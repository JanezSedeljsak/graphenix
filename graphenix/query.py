from .mixins.mixin_model_base import T, ModelBaseMixin
from .mixins.mixin_field_order import FieldOrderMixin
from .view import QueryView
from typing import Type, Generator
from collections import namedtuple
import graphenix_engine2 as ge2

def cond_node_from_conditions(*conditions) -> ge2.cond_node:
    condition_node = ge2.cond_node()
    f_children, f_conditions = [], []
    btree_conditions = []

    for cond in conditions:
        if isinstance(cond, ge2.cond_node):
            f_children.append(cond)
            continue

        indexed, current_cond = cond
        if indexed:
            btree_conditions.append(current_cond)
        else:
            f_conditions.append(current_cond)

    condition_node.btree_conditions = btree_conditions
    condition_node.conditions = f_conditions
    condition_node.children = f_children
    return condition_node

def every(*conditions):
    condition_node = cond_node_from_conditions(*conditions)
    condition_node.is_and = True
    return condition_node

def some(*conditions):
    condition_node = cond_node_from_conditions(*conditions)
    condition_node.is_and = False
    return condition_node

class Query:
    base_model: Type[T]
    query_object: object
    
    def _get_virtual_link_field(self, key):
        if not hasattr(self.base_model, key):
            raise AttributeError(f"Invalid key for linking {key}")

        field = getattr(self.base_model, key)
        return field.link_field

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

        # single field select (-1 -> PK, -2 -> IGNORE)
        self.query_object.picked_index = -2

    def filter(self, *conditions) -> "Query":
        condition_node = cond_node_from_conditions(*conditions)
        condition_node.is_and = True
        self.query_object.filter_root = condition_node
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
        view = ge2.execute_query(self.query_object, 0)
        return view.size(), QueryView(self.base_model, view)

    def first(self, as_view=False) -> T | None:
        self.query_object.limit = 1
        self.base_model.make_cache()
        data = QueryView(self.base_model, ge2.execute_query(self.query_object, 0))
        if not data:
            return None
        
        if as_view:
            return data[0]
        
        return self.base_model.from_view(data[0])

    
    def pick_id(self) -> list:
        return self.pick(None)
    
    def pick(self, field) -> list:
        field_index = -1
        if field is not None:
            field_index = self.base_model._model_fields.index(field.name)

        self.query_object.picked_index = field_index
        data = ge2.execute_entity_query(self.query_object)
        return [picked for picked, *_ in data]
    
    def agg(self, by = None, **aggregations) -> list:
        res_keys = list(aggregations.keys())
        if by is not None:
            res_keys = [by.name, *res_keys]
            self.query_object.agg_field_index = self.base_model._model_fields.index(by.name)
        
        aggs = []
        for [field, func] in aggregations.values():
            agg_obj = ge2.aggregate_object()
            agg_obj.option = func
            if func != AGG._Options.COUNT:
                agg_obj.field_index = self.base_model._model_fields.index(field.name)
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
    def first(cls, as_view=False):
        return Query(cls).first(as_view=as_view)
    
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
    def pick(cls, field) -> Query:
        return Query(cls).pick(field)
    
    @classmethod
    def pick_id(cls) -> Query:
        return Query(cls).pick_id()
    
    @classmethod
    def agg(cls, by = None, **aggregations) -> list:
        return Query(cls).agg(by=by, **aggregations)
