from .mixins.mixin_model_base import T
from typing import Type, Generator
import graphenix_engine2 as ge2

class Query:
    base_model: Type[T]

    def __init__(self, model: Type[T]):
        self.base_model = model

    def filter(self, *conditions) -> "Query":
        ...

    def limit(self, count) -> "Query":
        ...

    def order(self, attr, asc: bool = True) -> "Query":
        ...

    def link(self, **link_map) -> "Query":
        ...
    
    def all(self) -> tuple[int, Generator[T, None, None]]:
        self.base_model.make_cache()
 
        sizes_as_list = [self.base_model._field_sizes[field] for field in self.base_model._model_fields]
        raw_type_as_list = [self.base_model._field_types_raw[field] for field in self.base_model._model_fields]

        data = ge2.execute_query(self.base_model._db, self.base_model.__name__,
                                 self.base_model._total_size)

        def generator_func():
            for row in data:
                record_dict = ge2.build_record(sizes_as_list, raw_type_as_list,
                                               self.base_model._model_fields, 
                                               self.base_model._total_size,
                                               row)
                
                yield self.base_model(**record_dict)

        return len(data), generator_func()
    

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
    def limit(cls, count) -> Query:
        return Query(cls).limit(count)

    @classmethod
    def order(cls, attr, asc: bool = True) -> Query:
        return Query(cls).order(attr, asc=asc)
