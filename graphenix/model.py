import graphenix_engine2

from graphenix.internal.query import Query
from graphenix.field import Field

class Model:
    __db__ = None
    __lazy_delete__ = False
    qfields = None # if not defined take all the fields

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
        if name not in self.get_fields() and not name.startswith("_"):
            raise AttributeError(f"Cannot assign member '{name}' for type '{self.__name__}'")
        
        super().__setattr__(name, value)

    @property
    def id(self):
        return self._id
    
    @property
    def is_new(self):
        return self.id == -1
    
    @staticmethod
    def filter_attributes(attrs):
        return [attr for attr, val in attrs.items() if isinstance(val, Field.BaseType)]
    
    @classmethod
    def get_field_sizes(cls):
        field_sizes = {}
        for field_name, field_type in cls.__dict__.items():
            if isinstance(field_type, Field.BaseType):
                if not field_type.size:
                    raise AttributeError(f'Size for field {field_name} is not defined or is not a positive integer!')
                
                field_sizes[field_name] = field_type.size
        return field_sizes
    
    @classmethod
    def get_fields(cls):
        return Model.filter_attributes(cls.__dict__)
    
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
        fields_sizes_dict = cls.get_field_sizes()
        sizes_as_list = [fields_sizes_dict[field] for field in fields]

        record = graphenix_engine2.schema_get_record(cls.__db__, cls.__name__, record_id, sizes_as_list)
        record_as_dict = {field: record[idx] for idx, field in enumerate(fields)}
        instance = cls(**record_as_dict)
        instance._id = record_id
        return instance
    
    def delete(self, lazy=None):
        if lazy is None:
            lazy = self.__lazy_delete__

        fields = self.get_fields()
        fields_sizes_dict = self.get_field_sizes()
        sizes_as_list = [fields_sizes_dict[field] for field in fields]

        graphenix_engine2.schema_delete_record(self.__db__, self.__name__, self.id, lazy, sizes_as_list)
        self._id = -1 # set flag to is_new again so you don't update an inactive record
    
    def save(self):
        fields = self.get_fields()

        # TODO - each field is currently just a string - we need actual types
        values_as_list = [str(getattr(self, field)) for field in fields]
        fields_sizes_dict = self.get_field_sizes()
        sizes_as_list = [fields_sizes_dict[field] for field in fields]

        if self.is_new:
            record_id = graphenix_engine2.schema_add_record(self.__db__, self.__name__, values_as_list, sizes_as_list)
            self._id = record_id
        else:
            graphenix_engine2.schema_update_record(self.__db__, self.__name__, self.id, values_as_list, sizes_as_list)

