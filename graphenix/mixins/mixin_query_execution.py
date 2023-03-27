from .mixin_model_base import ModelBaseMixin, T

class QueryExecutionMixin:
    base_model : ModelBaseMixin

    def all(self) -> list[T]:
        data: list[T] = []
        for i in range(2):
            data.append(self.base_model.get(i))
            
        return data

    def first(self) -> T:
        ... # this exectures the actual query to the schema engine