import graphenix_engine2

from typing import TypeVar, cast, Union
from datetime import datetime, timedelta
from .query import ModelQueryMixin

T = TypeVar('T', bound='Model')

class Model(ModelQueryMixin):
    __db__ = None
    __lazy_delete__ = False

    # cached values
    __cache_init: bool = False
    __model_fields: tuple = tuple()
    __total_size: int = 0
    __field_sizes: dict[str, int] = {}
    __field_types: dict[str, type] = {}
    __field_types_raw: dict[str, int] = {} # for parsing in c++
    __field_defaults: dict = {} # can't add type hints since there are different types for default (base of field type)

    def __init__(self, **fields):
        self._id = -1
        self.__name__ = self.__class__.__name__
        for key, value in fields.items():
            setattr(self, key, value)

    def __str__(self):
        fields = ['id', *self.__get_fields()]
        attrs = ', '.join([f"{k}={getattr(self, k)}" for k in fields])
        return f"{self.__name__}({attrs})"
    
    def __setattr__(self, name, value):
        fields = self.__get_fields()
        if name not in fields and not name.startswith("_"):
            raise AttributeError(f"Cannot assign member '{name}' for type '{self.__name__}'")
        
        super().__setattr__(name, value)

    @property
    def id(self):
        return self._id
    
    @property
    def is_new(self):
        return self.id == -1
    
    @classmethod
    def __make_cache(cls) -> None:
        if cls.__cache_init:
            return
        
        field_sizes = {}
        for field_name, field in cls.__dict__.items():
            if isinstance(field, Field.BaseType):
                if not field.size:
                    raise AttributeError(f'Size for field {field_name} is not defined or is not a positive integer!')

                field_sizes[field_name] = field.size
                actual_type = type(field)
                cls.__field_types[field_name] = actual_type
                cls.__field_defaults[field_name] = field.default

                match actual_type:
                    case Field.Int:
                        raw_type_index = FieldTypeEnum.INT
                    case Field.String:
                        raw_type_index = FieldTypeEnum.STRING
                    case Field.Bool:
                        raw_type_index = FieldTypeEnum.BOOL
                    case Field.DateTime:
                        raw_type_index = FieldTypeEnum.DATETIME
                    case Field.Link:
                        raw_type_index = FieldTypeEnum.LINK_SINGLE
                    case Field.LinkMultiple:
                        raw_type_index = FieldTypeEnum.LINK_MULTIPLE
                    case _:
                        raise AttributeError("Field type is not valid!")
                

                cls.__field_types_raw[field_name] = raw_type_index
        
        cls.__field_sizes = field_sizes
        cls.__total_size = sum(field_sizes.values())
        cls.__cache_init = True
    
    @classmethod
    def __get_fields(cls):
        if not cls.__model_fields:
            cls.__model_fields = tuple(attr for attr, val in cls.__dict__.items() if isinstance(val, Field.BaseType))

        return cls.__model_fields
    
    @classmethod
    def get(cls, record_id):
        fields = cls.__get_fields()
        cls.__make_cache()

        field_sizes = cls.__field_sizes
        sizes_as_list = [field_sizes[field] for field in fields]
        raw_type_as_list = [cls.__field_types_raw[field] for field in fields]

        record = graphenix_engine2.schema_get_record(cls.__db__, cls.__name__, # type: ignore
                                                     record_id, sizes_as_list,
                                                     raw_type_as_list, cls.__total_size)
        
        record_as_dict = {field: record[idx] for idx, field in enumerate(fields)}
        instance = cls(**record_as_dict)
        instance._id = record_id
        return instance
    
    def __get_values(self, fields) -> list:
        result: list = []
        for field in fields:
            match self.__field_types[field]:
                case Field.Link:
                    result.append(getattr(self, '_' + field + '_id', self.__field_defaults[field]))
                case _:
                    result.append(getattr(self, '_' + field, self.__field_defaults[field]))
        
        return result
    
    def delete(self, lazy=None):
        if self.is_new:
            raise Exception("Record doesn't exist in the db!")

        if lazy is None:
            lazy = self.__lazy_delete__

        graphenix_engine2.schema_delete_record(self.__db__, self.__name__, # type: ignore
                                               self.id, lazy, self.__total_size)
        self._id = -1 # set flag to is_new again so you don't update an inactive record

    def make(self: T) -> T:
        self.save()
        return self
    
    def save(self):
        fields = self.__get_fields()
        self.__make_cache()

        field_sizes = self.__field_sizes
        sizes_as_list = [field_sizes[field] for field in fields]
        values_as_list = self.__get_values(fields)
        raw_type_as_list = [self.__field_types_raw[field] for field in fields]

        if self.is_new:
            self._id = graphenix_engine2.schema_add_record(self.__db__, self.__name__, # type: ignore
                                                            values_as_list, sizes_as_list,
                                                            self.__total_size,
                                                            raw_type_as_list)
        else:
            graphenix_engine2.schema_update_record(self.__db__, self.__name__, self.id,  # type: ignore
                                                   values_as_list, sizes_as_list,
                                                   self.__total_size,
                                                   raw_type_as_list)


class FieldTypeEnum:
    INT = 0
    STRING = 1
    BOOL = 2
    DATETIME = 3
    LINK_SINGLE = 4
    LINK_MULTIPLE = 5

class Field:

    class BaseType:
        size = None
        default = None

        def __get__(self, instance, owner):
            return getattr(instance, '_' + self.name, self.default)
        
        def __set__(self, instance, value):
            setattr(instance, '_' + self.name, value)
        
        def __set_name__(self, owner, name):
            self.name = name

    class Int(BaseType):
        def __init__(self, default: int = 0):
            self.default = default
            self.size = 8

        def __get__(self, instance, owner) -> int:
            return getattr(instance, '_' + self.name, self.default)

        def __set__(self, instance, value):
            setattr(instance, '_' + self.name, value)

    class String(BaseType):
        def __init__(self, size=255, default: str = ''):
            self.size = size
            self.default = default

    class Bool(Int):
        def __init__(self, default: bool = False):
            self.default = default
            self.size = 1

        def __get__(self, instance, owner) -> bool:
            return getattr(instance, '_' + self.name, self.default)

        def __set__(self, instance, value):
            setattr(instance, '_' + self.name, int(value))

    class DateTime(BaseType):
        """ Datetime field is stored as ammount of seconds from 1.1.1970 (POSIX) format """
        epoch = datetime.utcfromtimestamp(0)

        def __init__(self, default = 0):
            self.default = default
            self.size = 8

        def __get__(self, instance, owner) -> datetime:
            diff: int = getattr(instance, '_' + self.name, self.default)
            dt = self.epoch + timedelta(seconds=diff)
            return dt
        
        def __set__(self, instance, value: datetime | int):
            if isinstance(value, int):
                setattr(instance, '_' + self.name, int(value))
                return

            diff = int((value - self.epoch).total_seconds())
            setattr(instance, '_' + self.name, diff)
    
    class Link(BaseType):
        def __init__(self, default = -1):
            self.default = default
            self.size = 8

        def __set__(self, instance, value: T | int | None) -> None:
            if value is None:
                setattr(instance, '_' + self.name + '_id', -1)
                setattr(instance, '_' + self.name, None)
                return

            if isinstance(value, int):
                setattr(instance, '_' + self.name + '_id', value)
                return

            # set both the id stored id _{field_name}_id & the actual model in  _{field_name}
            setattr(instance, '_' + self.name + '_id', int(value.id)) # type: ignore
            setattr(instance, '_' + self.name, value)
        
        def __get__(self, instance, owner) -> T | None:
            linked : T | None = getattr(instance, '_' + self.name, None)
            return linked


    class LinkMultiple(Int):
        ...
