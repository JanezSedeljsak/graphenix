import graphenix_engine2 as ge2
from .enums import FilterOperationEnum
from .mixin_model_base import ModelBaseMixin

def _make_cond_object(model, operation, cmp_value):
    cond_obj = ge2.cond_object()
    cond_obj.field_name = '' # empty string marks PK
    cond_obj.operation_index = operation
    cond_obj.value = cmp_value if not isinstance(cmp_value, ModelBaseMixin) else cmp_value.id
    return False, cond_obj

class ModelFilteringMixin:

    @classmethod
    def equals(cls, val):
        return _make_cond_object(cls, FilterOperationEnum.EQUAL, val)
    
    @classmethod
    def is_not(cls, val):
        return _make_cond_object(cls, FilterOperationEnum.NOTEQUAL, val)

    @classmethod
    def greater(cls, val):
        return _make_cond_object(cls, FilterOperationEnum.GREATER, val)

    @classmethod
    def greater_or_equal(cls, val):
        return _make_cond_object(cls, FilterOperationEnum.GREATER_OR_EQUAL, val)

    @classmethod
    def less(cls, val):
        return _make_cond_object(cls, FilterOperationEnum.LESS, val)
    
    @classmethod
    def less_or_equal(cls, val):
        return _make_cond_object(cls, FilterOperationEnum.LESS_OR_EQUAL, val)
    
    @classmethod
    def regex(cls, val):
        raise Exception("Regex is only allowed on type String")
    
    @classmethod
    def is_in(cls, values):
        return _make_cond_object(cls, FilterOperationEnum.IS_IN, values)
    
    @classmethod
    def not_in(cls, values):
        return _make_cond_object(cls, FilterOperationEnum.NOT_IN, values)
    
    @classmethod
    def between(cls, low, high):
        return _make_cond_object(cls, FilterOperationEnum.BETWEEN, (low, high))