from .mixins.mixin_model_base import T, ModelBaseMixin
from .mixins.mixin_field_order import FieldOrderMixin
from typing import Type, Generator
from collections import namedtuple
from datetime import datetime
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

    def _handle_link_index(self, link):
        return link.link_field_index + 1
    
    def _get_virtual_link_field(self, key):
        if not hasattr(self.base_model, key):
            raise AttributeError(f"Invalid key for linking {key}")

        field = getattr(self.base_model, key)
        return field.link_field
    
    def _is_date_field(self, idx: int) -> bool:
        return self.query_object.mdef.field_date_indexes[idx - 1]

    def _handle_tuple_field(self, field, idx: int, link_map: dict):
        if self._is_date_field(idx):
            return datetime.fromtimestamp(field)
        
        link_index = link_map.get(idx)
        if not link_index and link_index != 0:                
            return field
        
        subquery = self.subqueries[link_index]
        sublink = self.query_object.links[link_index]
        sublink_map = {
            self._handle_link_index(link): idx
            for idx, link in enumerate(subquery.query_object.links)
        }

        if sublink.is_direct_link:
            return subquery._make_namedtuple(field, sublink_map) if field != -1 else None
        
        if not isinstance(field, list):
            raise ValueError('Link data should be a list!')

        return [subquery._make_namedtuple(row, sublink_map) for row in field]   


    def _make_namedtuple(self, tuple_record: tuple, link_map: dict) -> tuple:
        if not isinstance(tuple_record, tuple):
            raise ValueError(f'Expected tuple got {type(tuple_record)} for record')
        
        has_date_fields = any(self.query_object.mdef.field_date_indexes)
        if not link_map and not has_date_fields:
            return self.base_model._view_tuple._make(tuple_record)
        
        new_record = tuple(
            self._handle_tuple_field(field, idx, link_map)
            for idx, field in enumerate(tuple_record)
        )
        rec = self.base_model._view_tuple._make(new_record)
        return rec

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

        # linking
        self.query_object.links = []
        self.query_object.has_ix_constraints = False
        self.query_object.is_subquery = True
        self.subqueries = []

        # single field select
        self.query_object.picked_index = -1

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
        links, link_vector = [], []
        self.subqueries = []

        for link_key, link in link_map.items():
            link_obj = ge2.link_object()
            link_obj.link_field_index = self.base_model._model_fields.index(link_key)
            vlink_field = self._get_virtual_link_field(link_key)
            if vlink_field is None:
                # direct link
                link_obj.child_link_field_index = -1
                link_obj.is_direct_link = True
                link_obj.limit = 0
                link_obj.offset = 0

                subquery = link
                if isinstance(link, type) and issubclass(link, ModelBaseMixin):
                    subquery = Query(link)

                subquery.query_object.has_ix_constraints = True
                subquery.query_object.is_subquery = True
                self.subqueries.append(subquery)
                link_vector.append(subquery.query_object)
            else:
                # virtual link
                subquery = link
                if isinstance(link, type) and issubclass(link, ModelBaseMixin):
                    subquery = Query(link)
                
                if not isinstance(subquery, Query):
                    raise ValueError("Virtual link can't be resolved to query type!")

                link_obj.child_link_field_index = subquery.base_model._model_fields.index(vlink_field) 
                link_obj.is_direct_link = False

                # original limit and offset get moved from original subquery to link object
                link_obj.limit = subquery.query_object.limit
                link_obj.offset = subquery.query_object.offset
                subquery.query_object.limit = 0
                subquery.query_object.offset = 0

                # here we will have to apppend the foreign keys as a filter condition
                subquery.query_object.has_ix_constraints = False
                subquery.query_object.is_subquery = True
                self.subqueries.append(subquery)
                link_vector.append(subquery.query_object)

            links.append(link_obj)

        self.query_object.links = links
        self.query_object.link_vector = link_vector
        return self

    def all(self) -> tuple[int, Generator[T, None, None]]:
        self.base_model.make_cache()
        tuple_records = ge2.execute_query(self.query_object, 0)
        link_map = {
            self._handle_link_index(link): idx
            for idx, link in enumerate(self.query_object.links)
        }
    
        def generator_func():
            for trec in tuple_records:
                ntuple_res = self._make_namedtuple(trec, link_map)
                yield ntuple_res

        return len(tuple_records), generator_func()

    def first(self) -> T | None:
        self.query_object.limit = 1
        self.base_model.make_cache()
        data = ge2.execute_query(self.query_object, 0)
        if len(data) != 1:
            return None

        link_map = {
            self._handle_link_index(link): idx
            for idx, link in enumerate(self.query_object.links)
        }

        ntuple_res = self._make_namedtuple(data[0], link_map)
        return ntuple_res
    
    def pick_id(self) -> list:
        return self.pick(None)
    
    def pick(self, field) -> list:
        field_index = -1
        if field is not None:
            field_index = self.base_model._model_fields.index(field.name)

        self.query_object.picked_index = field_index
        data = ge2.execute_query(self.query_object, 0)
        return [picked for picked, *_ in data]
    
    def agg(self, by = None, **aggregations) -> list:
        res_keys = list(aggregations.keys())
        if by is not None:
            res_keys = [by.name, *res_keys]
            self.query_object.agg_field_index = self.base_model._model_fields.index(by.name)
        
        aggs = []
        for [field_name, func] in aggregations.values():
            agg_obj = ge2.aggregate_object()
            agg_obj.option = func
            if func != AGG._Options.COUNT:
                agg_obj.field_index = self.base_model._model_fields.index(field_name)
            else:
                agg_obj.field_index = -1

            aggs.append(agg_obj)

        self.query_object.agg_vector = aggs
        data = ge2.execute_agg_query(self.query_object)
        agg_view_tuple = namedtuple("AggView", res_keys)
        result = [agg_view_tuple._make(row) for row in data]
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
    def count():
        return [None, AGG._Options.COUNT]


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
