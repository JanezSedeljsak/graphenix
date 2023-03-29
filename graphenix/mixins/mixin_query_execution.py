import graphenix_engine2

from typing import Type
from .mixin_model_base import ModelBaseMixin, T

class QueryExecutionMixin:

    _base_model : Type[T]


    def all(self) -> list[T]:
        return []
        fields = self._base_model._model_fields
        self._base_model._make_cache()

        field_sizes = self._base_model._field_sizes
        sizes_as_list = [field_sizes[field] for field in fields]
        raw_type_as_list = [self._base_model._field_types_raw[field] for field in fields]

        data : list[T] = graphenix_engine2.query(
            self._base_model._db,
            self._base_model.__name__,
            sizes_as_list,
            raw_type_as_list,
            self._base_model._total_size
        )

        return data

    def first(self) -> T:
        ... # this exectures the actual query to the schema engine