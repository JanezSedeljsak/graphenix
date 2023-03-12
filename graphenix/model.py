import graphenix_engine

from graphenix.internal.query import Query

class Model:
    __db__ = None

    def __init__(self, **fields):
        self._id = -1
        self.__name__ = self.__class__.__name__
        for key, value in fields.items():
            setattr(self, key, value)

    def __str__(self):
        fields = ['id', *self.get_fields_instance()]
        attrs = ', '.join([f"{k}={getattr(self, k)}" for k in fields])
        return f"{self.__name__}({attrs})"

    @property
    def id(self):
        return self._id
    
    @staticmethod
    def filter_attributes(attrs):
        return [attr for attr in attrs if not attr.startswith('_')]

    
    @classmethod
    def get_fields(cls):
        return Model.filter_attributes(cls.__dict__['__annotations__'])
    
    def get_fields_instance(self):
        return Model.filter_attributes(self.__dict__)
    
    @classmethod
    def all(cls):
        return Query(cls.__db__, cls.__name__).all()

    @classmethod
    def first(cls):
        return Query(cls.__db__, cls.__name__).first()
    
    @classmethod
    def filter(cls, **filters):
        return Query(cls.__db__, cls.__name__, **filters)
    
    @classmethod
    def get_by_id(cls, record_id):
        fields = cls.get_fields()
        record = graphenix_engine.schema_get_record(cls.__db__, cls.__name__, record_id, [100] * len(fields))
        record_as_dict = {field: record[idx] for idx, field in enumerate(fields)}
        return cls(**record_as_dict)
    
    def __getitem__(self, record_id):
        return self.get_by_id(record_id)
    
    def delete(self):
        pass
    
    def save(self):
        values_as_list = [getattr(self, field) for field in self.get_fields_instance()]
        graphenix_engine.schema_add_record(self.__db__, self.__name__, values_as_list)