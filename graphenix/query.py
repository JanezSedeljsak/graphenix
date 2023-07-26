from .mixins.mixin_model_base import T, ModelBaseMixin
from .mixins.mixin_field_order import FieldOrderMixin
from typing import Type, Generator
from collections import namedtuple
import graphenix_engine2 as ge2

def every(*conditions):
    condition_node = ge2.cond_node()
    f_children, f_conditions = [], []
    for cond in conditions:
        if isinstance(cond, ge2.cond_node):
            f_children.append(cond)
        else:
            f_conditions.append(cond)

    condition_node.conditions = f_conditions
    condition_node.children = f_children
    condition_node.is_and = False
    return condition_node

def some(*conditions):
    condition_node = ge2.cond_node()
    f_children, f_conditions = [], []
    for cond in conditions:
        if isinstance(cond, ge2.cond_node):
            f_children.append(cond)
        else:
            f_conditions.append(cond)

    condition_node.conditions = f_conditions
    condition_node.children = f_children
    condition_node.is_and = False
    return condition_node

class Query:
    base_model: Type[T]
    query_object: object

    def __init__(self, model: Type[T]):
        self.base_model = model
        self.base_model.make_cache()
        self.query_object = ge2.query_object()
        self.query_object.mdef = self.base_model._mdef

        # order
        self.query_object.field_indexes = []
        self.query_object.order_asc = []

        # limit
        self.query_object.limit = 0
        self.query_object.offset = 0

        # filtering
        filter_root = ge2.cond_node()
        filter_root.conditions = []
        filter_root.children = []
        filter_root.is_and = True
        self.query_object.filter_root = filter_root

        # aggregation
        self.query_object.agg_field_index = -1
        self.query_object.agg_vector = []

    def filter(self, *conditions) -> "Query":
        f_children, f_conditions = [], []
        for cond in conditions:
            if isinstance(cond, ge2.cond_node):
                f_children.append(cond)
            else:
                f_conditions.append(cond)

        self.query_object.filter_root.conditions = f_conditions
        self.query_object.filter_root.children = f_children
        return self

    def limit(self, amount: int) -> "Query":
        self.query_object.limit = amount
        return self
    
    def offset(self, amount: int) -> "Query":
        self.query_object.offset = amount
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
        tuple_records = ge2.execute_query(self.query_object)
    
        def generator_func():
            for trec in tuple_records:
                ntuple_res = self.base_model.view_tuple._make(trec)
                yield ntuple_res

        return len(tuple_records), generator_func()

    def first(self) -> T | None:
        self.query_object.limit = 1
        self.base_model.make_cache()
        data = ge2.execute_query(self.query_object)
        if len(data) != 1:
            return None
        
        record_dict = ge2.build_record(self.base_model._mdef, data[0])
        record : T = self.base_model(**record_dict)
        return record
    
    def agg(self, by = None, **aggregations) -> list:
        res_keys = list(aggregations.keys())
        if by is not None:
            res_keys = [by.name, *res_keys]
            self.query_object.agg_field_index = self.base_model._model_fields.index(by.name)
        
        aggs = []
        for [field_name, func] in aggregations.values():
            agg_obj = ge2.aggregate_object()
            agg_obj.option = func
            agg_obj.field_index = self.base_model._model_fields.index(field_name)
            aggs.append(agg_obj)

        self.query_object.agg_vector = aggs
        data = ge2.execute_agg_query(self.query_object)
        view_tuple = namedtuple("AggView", res_keys)
        result = [view_tuple._make(row) for row in data]
        return result


class AGG:
    
    class _Options:
        SUM = 0
        MIN = 1
        MAX = 2
        COUNT = 3

    @staticmethod
    def min(field):
        return [field, AGG._Options.MIN]
    
    @staticmethod
    def max(field):
        return [field, AGG._Options.MAX]
    
    @staticmethod
    def sum(field):
        return [field, AGG._Options.SUM]
    
    @staticmethod
    def count(field):
        return [field, AGG._Options.COUNT]


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
    def offset(cls, amount: int) -> Query:
        return Query(cls).offset(amount)

    @classmethod
    def order(cls, *fields) -> Query:
        return Query(cls).order(*fields)
    
    @classmethod
    def agg(cls, by = None, **aggregations) -> list:
        return Query(cls).agg(by=by, **aggregations)
