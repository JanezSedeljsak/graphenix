import graphenix_engine2

from graphenix.internal.query import Query
from graphenix.field import Field

class Model:
    __db__ = None
    __lazy_delete__ = False

    # cached values
    __qfields__: tuple = tuple()
    __total_size__: int = 0
    __field_sizes__: dict[str, int] = {}
    __field_types__: dict[str, type] = {}

    def __init__(self, **fields):
        self._id = -1
        self.__name__ = self.__class__.__name__
        for key, value in fields.items():
            setattr(self, key, value)

    def __str__(self):
        fields = ['id', *self.get_fields()]
        attrs = ', '.join([f"{k}={getattr(self, k)}" for k in fields])
        return f"{self.__name__}({attrs})"
    
    def __setattr__(self, name, value):
        fields = self.get_fields()
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
    def get_field_sizes(cls) -> dict[str, int]:
        if cls.__field_sizes__:
            return cls.__field_sizes__
        
        field_sizes = {}
        for field_name, field in cls.__dict__.items():
            if isinstance(field, Field.BaseType):
                if not field.size:
                    raise AttributeError(f'Size for field {field_name} is not defined or is not a positive integer!')

                field_sizes[field_name] = field.size
                cls.__field_types__[field_name] = type(field)
        
        cls.__field_sizes__ = field_sizes
        cls.__total_size__ = sum(field_sizes.values())
        return field_sizes
    
    @classmethod
    def get_fields(cls):
        if not cls.__qfields__:
            cls.__qfields__ = tuple(attr for attr, val in cls.__dict__.items() if isinstance(val, Field.BaseType))

        return cls.__qfields__
    
    @classmethod
    def all(cls):
        return Query(cls).all()

    @classmethod
    def first(cls):
        return Query(cls).first()
    
    @classmethod
    def filter(cls, **filters) -> Query:
        return Query(cls, **filters)
    
    @classmethod
    def get(cls, record_id):
        fields = cls.get_fields()
        field_sizes = cls.get_field_sizes()
        sizes_as_list = [field_sizes[field] for field in fields]

        record = graphenix_engine2.schema_get_record(cls.__db__, cls.__name__, 
                                                     record_id, sizes_as_list)
        
        record_as_dict = {field: record[idx] for idx, field in enumerate(fields)}
        instance = cls(**record_as_dict)
        instance._id = record_id
        return instance
    
    def get_values(self, fields) -> list:
        values = []
        for field in fields:
            match self.__field_types__[field]:
                case Field.String:
                    values.append(str(getattr(self, field)))
                case Field.Int:
                    values.append(str(getattr(self, field)))
                case Field.Bool:
                    values.append(str(getattr(self, field)))
                case Field.DateTime:
                    values.append(str(getattr(self, field)))
        
        return values
    
    def delete(self, lazy=None):
        if self.is_new:
            raise Exception("Record doesn't exist in the db!")

        if lazy is None:
            lazy = self.__lazy_delete__

        graphenix_engine2.schema_delete_record(self.__db__, self.__name__, 
                                               self.id, lazy, self.__total_size__)
        self._id = -1 # set flag to is_new again so you don't update an inactive record
    
    def save(self):
        fields = self.get_fields()

        field_sizes = self.get_field_sizes()
        sizes_as_list = [field_sizes[field] for field in fields]
        values_as_list = self.get_values(fields)

        if self.is_new:
            self._id = graphenix_engine2.schema_add_record(self.__db__, self.__name__, 
                                                            values_as_list, sizes_as_list,
                                                            self.__total_size__)
        else:
            graphenix_engine2.schema_update_record(self.__db__, self.__name__, self.id, 
                                                   values_as_list, sizes_as_list,
                                                   self.__total_size__)

