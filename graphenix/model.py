import graphenix_engine2

from typing import Type
from .mixins.mixin_model_base import ModelBaseMixin, T
from .query import ModelQueryMixin
from .field import Field, FieldTypeEnum

class Model(ModelBaseMixin, ModelQueryMixin):
    _db = None
    __lazy_delete__ = False

    def __init__(self, **fields):
        self._id = -1
        self.__name__ = self.__class__.__name__
        for key, value in fields.items():
            setattr(self, key, value)

    def __str__(self):
        self._make_cache()
        fields = ['id', *self._model_fields]
        attrs = ', '.join([f"{k}={getattr(self, k)}" for k in fields])
        return f"{self.__name__}({attrs})"
    
    def __setattr__(self, name, value):
        self._make_cache()
        if name not in self._model_fields and not name.startswith("_"):
            raise AttributeError(f"Cannot assign member '{name}' for type '{self.__name__}'")
        
        super().__setattr__(name, value)

    @property
    def id(self):
        return self._id
    
    @property
    def is_new(self):
        return self.id == -1
    
    @classmethod
    def _make_cache(cls: Type[T]) -> None:
        if cls._cache_init:
            return
        
        cls._model_fields = tuple(attr for attr, val in cls.__dict__.items() if isinstance(val, Field.BaseType))
        for field_name, field in cls.__dict__.items():
            if isinstance(field, Field.BaseType):
                if not field.size:
                    raise AttributeError(f'Size for field {field_name} is not defined or is not a positive integer!')

                cls._field_sizes[field_name] = field.size
                actual_type = type(field)
                cls._field_types[field_name] = actual_type
                cls._field_defaults[field_name] = field.default

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

                cls._field_types_raw[field_name] = raw_type_index
        
        cls._total_size = sum(cls._field_sizes.values())
        cls._cache_init = True
    
    @classmethod
    def get(cls: Type[T], record_id):
        cls._make_cache()

        field_sizes = cls._field_sizes
        sizes_as_list = [field_sizes[field] for field in cls._model_fields]
        raw_type_as_list = [cls._field_types_raw[field] for field in cls._model_fields]

        record = graphenix_engine2.schema_get_record(cls._db, cls.__name__, # type: ignore
                                                     record_id, sizes_as_list,
                                                     raw_type_as_list, cls._total_size)
        
        record_as_dict = {field: record[idx] for idx, field in enumerate(cls._model_fields)}
        instance = cls(**record_as_dict)
        instance._id = record_id
        return instance
    
    def __get_values(self, fields) -> list:
        result: list = []
        for field in fields:
            match self._field_types[field]:
                case Field.Link:
                    result.append(getattr(self, '_' + field + '_id', self._field_defaults[field]))
                case _:
                    result.append(getattr(self, '_' + field, self._field_defaults[field]))
        
        return result
    
    def delete(self, lazy=None):
        if self.is_new:
            raise Exception("Record doesn't exist in the db!")

        if lazy is None:
            lazy = self.__lazy_delete__

        graphenix_engine2.schema_delete_record(self._db, self.__name__, # type: ignore
                                               self.id, lazy, self._total_size)
        self._id = -1 # set flag to is_new again so you don't update an inactive record

    def make(self: T) -> T:
        self.save()
        return self
    
    def save(self):
        self._make_cache()

        field_sizes = self._field_sizes
        sizes_as_list = [field_sizes[field] for field in self._model_fields]
        values_as_list = self.__get_values(self._model_fields)
        raw_type_as_list = [self._field_types_raw[field] for field in self._model_fields]

        if self.is_new:
            self._id = graphenix_engine2.schema_add_record(self._db, self.__name__, # type: ignore
                                                            values_as_list, sizes_as_list,
                                                            self._total_size,
                                                            raw_type_as_list)
        else:
            graphenix_engine2.schema_update_record(self._db, self.__name__, self.id,  # type: ignore
                                                   values_as_list, sizes_as_list,
                                                   self._total_size,
                                                   raw_type_as_list)


