import graphenix_engine

from graphenix.internal.query import Query

class Model:
    __db__ = None
    __lazy_delete__ = False

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
    
    @property
    def is_new(self):
        return self.id == -1
    
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
        record = graphenix_engine.schema_get_record(cls.__db__, cls.__name__, record_id, [100] * len(fields))
        record_as_dict = {field: record[idx] for idx, field in enumerate(fields)}
        instance = cls(**record_as_dict)
        instance._id = record_id
        return instance
    
    def delete(self):
        return Query(self, id=self.id).delete()
    
    def save(self):
        values_as_list = [getattr(self, field) for field in self.get_fields_instance()]

        if self.is_new:
            record_id = graphenix_engine.schema_add_record(self.__db__, self.__name__, values_as_list)
            self._id = record_id
        else:
            graphenix_engine.schema_update_record(self.__db__, self.__name__, self.id, values_as_list)

