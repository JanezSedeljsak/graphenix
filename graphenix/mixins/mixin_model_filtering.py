import graphenix_engine2 as ge2
from .mixin_field_order import FilterOperationEnum

def _make_cond_object(operation, cmp_value):
        cond_obj = ge2.cond_object()
        cond_obj.field_name = '' # empty string marks PK
        cond_obj.operation_index = operation
        cond_obj.value = cmp_value
        return cond_obj

class ModelFilteringMixin:    
    @classmethod
    def __eq__(cls, val):
        return _make_cond_object(FilterOperationEnum.EQUAL, val)
    
    @classmethod
    def __ne__(cls, val):
        return _make_cond_object(FilterOperationEnum.NOTEQUAL, val)

    @classmethod
    def __gt__(cls, val):
        return _make_cond_object(FilterOperationEnum.GREATER, val)

    @classmethod
    def __ge__(cls, val):
        return _make_cond_object(FilterOperationEnum.GREATER_OR_EQUAL, val)

    @classmethod
    def __lt__(cls, val):
        return _make_cond_object(FilterOperationEnum.LESS, val)
    
    @classmethod
    def __le__(cls, val):
        return _make_cond_object(FilterOperationEnum.LESS_OR_EQUAL, val)
    
    @classmethod
    def regex(cls, val):
        raise Exception("Regex is only allowed on type String")
    
    @classmethod
    def is_in(cls, values):
        return _make_cond_object(FilterOperationEnum.IS_IN, values)
    
    @classmethod
    def not_in(cls, values):
        return _make_cond_object(FilterOperationEnum.NOT_IN, values)