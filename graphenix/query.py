from .mixins.mixin_model_base import T, ModelBaseMixin
from .mixins.mixin_field_order import FieldOrderMixin
from typing import Type, Generator
import graphenix_engine2 as ge2

class Query:
    base_model: Type[T]
    query_object: object

    def __init__(self, model: Type[T]):
        self.base_model = model
        self.base_model.make_cache()
        self.query_object = ge2.query_object()
        self.query_object.mdef = self.base_model._mdef
        self.query_object.field_indexes = []
        self.query_object.order_asc = []
        self.query_object.limit = 0

    def filter(self, *conditions) -> "Query":
        ...

    def limit(self, amount: int) -> "Query":
        self.query_object.limit = amount
        return self

    def order(self, *fields) -> "Query":
        field_indexes = []
        order_asc = []

        for field in fields:
            if isinstance(field, FieldOrderMixin):
                order_asc.append(True)
                field_indexes.append(self.base_model._model_fields.index(field.name))

            elif isinstance(field, str):
                # if we recieve string it means the name of the field + desc
                order_asc.append(False)
                idx = self.base_model._model_fields.index(field) if field != 'id' else -1
                field_indexes.append(idx)
            
            elif isinstance(field, type) and issubclass(field, ModelBaseMixin):
                # if we pass the class directly it means we order by id asc
                order_asc.append(True)
                field_indexes.append(-1)


        self.query_object.field_indexes = field_indexes
        self.query_object.order_asc = order_asc
        return self
        

    def link(self, **link_map) -> "Query":
        ...
    
    def all(self) -> tuple[int, Generator[T, None, None]]:
        self.base_model.make_cache()
        data = ge2.execute_query(self.query_object)

        def generator_func():
            for row in data:
                record_dict = ge2.build_record(self.base_model._mdef, row)
                record : T = self.base_model(**record_dict)
                yield record

        return len(data), generator_func()

    def first(self) -> T | None:
        self.query_object.limit = 1
        self.base_model.make_cache()
        data = ge2.execute_query(self.query_object)
        if len(data) != 1:
            return None
        
        record_dict = ge2.build_record(self.base_model._mdef, data[0])
        record : T = self.base_model(**record_dict)
        return record

    

class ModelQueryMixin:
    
    @classmethod
    def all(cls) -> tuple[int, Generator[T, None, None]]:
        return Query(cls).all()

    @classmethod
    def first(cls):
        return Query(cls).first()
    
    @classmethod
    def link(cls, **link_map):
        return Query(cls).link(**link_map)
    
    @classmethod
    def filter(cls, *conditions) -> Query:
        return Query(cls).filter(*conditions)
    
    @classmethod
    def limit(cls, amount: int) -> Query:
        return Query(cls).limit(amount)

    @classmethod
    def order(cls, *fields) -> Query:
        return Query(cls).order(*fields)
