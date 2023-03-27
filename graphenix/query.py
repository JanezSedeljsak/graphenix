from .mixins.mixin_query_execution import QueryExecutionMixin

class Query(QueryExecutionMixin):
    def __init__(self, model):
        self.base_model = model

    # this method needs to be called on root so it then generates the sub queries accordingly
    def __build_query():
        ...

    def filter(self, *conditions) -> "Query":
        ...

    def limit(self, count) -> "Query":
        ...

    def order(self, attr, asc: bool = True) -> "Query":
        ...

    def link(self, **link_map) -> "Query":
        ...

class ModelQueryMixin:
    
    @classmethod
    def all(cls):
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
