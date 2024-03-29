from datetime import datetime
from .mixins.mixin_model_base import T, ModelBaseMixin
from .mixins.mixin_field_order import FO, FieldOrderMixin
from .mixins.enums import FilterOperationEnum

import graphenix_engine2 as ge2

class Field:

    class BaseType(FieldOrderMixin):
        size: int = 8
        default = None
        index: bool = False
        name: str
        
        def __set_name__(self, owner, name) -> None:
            self.name = name

        def as_index(self: FO) -> FO:
            self.index = True
            return self
        
        def desc(self: FO) -> str:
            return self.name
        
        def make_cond_object(self, operation, cmp_value):
            cond_obj = ge2.cond_object()
            cond_obj.field_name = self.name
            cond_obj.operation_index = operation
            cond_obj.value = cmp_value if not isinstance(cmp_value, ModelBaseMixin) else cmp_value.id
            is_indexed = self.index and FilterOperationEnum.supports_index(operation)
            return is_indexed, cond_obj
        
        def equals(self, val):
            return self.make_cond_object(FilterOperationEnum.EQUAL, val)

        def is_not(self, val):
            return self.make_cond_object(FilterOperationEnum.NOTEQUAL, val)

        def greater(self, val):
            return self.make_cond_object(FilterOperationEnum.GREATER, val)

        def greater_or_equal(self, val):
            return self.make_cond_object(FilterOperationEnum.GREATER_OR_EQUAL, val)

        def less(self, val):
            return self.make_cond_object(FilterOperationEnum.LESS, val)

        def less_or_equal(self, val):
            return self.make_cond_object(FilterOperationEnum.LESS_OR_EQUAL, val)
        
        def regex(self, val):
            raise Exception("Regex is only allowed on type String")
        
        def iregex(self, val):
            raise Exception("Regex is only allowed on type String")
        
        def is_in(self, values):
            return self.make_cond_object(FilterOperationEnum.IS_IN, values)
        
        def not_in(self, values):
            return self.make_cond_object(FilterOperationEnum.NOT_IN, values)
        
        def between(self, low, high):
            return self.make_cond_object(FilterOperationEnum.BETWEEN, (low, high))


    class Int(BaseType):
        def __init__(self, default: int = 0):
            self.default = default
            self.size = 8

        def __get__(self, instance, owner) -> int:
            if not instance:
                return self
            
            return getattr(instance, '_' + self.name, self.default)

        def __set__(self, instance, value):
            setattr(instance, '_' + self.name, value)

    class String(BaseType):
        def __init__(self, size=255, default: str = ''):
            self.size = size
            self.default = default

        def __get__(self, instance, owner) -> str:
            if not instance:
                return self
            
            return getattr(instance, '_' + self.name, self.default)
        
        def __set__(self, instance, value: str) -> None:
            setattr(instance, '_' + self.name, value)
        
        def regex(self, val):
            return self.make_cond_object(FilterOperationEnum.REGEX, val)
        
        def iregex(self, val):
            return self.make_cond_object(FilterOperationEnum.IREGEX, val)

    class Bool(BaseType):
        def __init__(self, default: bool = False):
            self.default = default
            self.size = 1

        def as_index(self: FO):
            raise AttributeError("Bool attributes can't be indexed!")

        def __get__(self, instance, owner) -> bool:
            if not instance:
                return self
            
            return getattr(instance, '_' + self.name, self.default)

        def __set__(self, instance, value):
            setattr(instance, '_' + self.name, value)

    class DateTime(BaseType):
        """ Datetime field is stored as ammount of seconds from 1.1.1970 (POSIX) format """

        def __init__(self, default = 0):
            self.default = default
            self.size = 8

        def __get__(self, instance, owner) -> datetime:
            if not instance:
                return self
            
            epoch: int = getattr(instance, '_' + self.name, self.default)
            return datetime.fromtimestamp(epoch)
        
        def __set__(self, instance, value: datetime | int):
            if isinstance(value, int):
                setattr(instance, '_' + self.name, int(value))
                return

            diff = int(value.timestamp())
            setattr(instance, '_' + self.name, diff)

        def equals(self, val):
            posix = int(val.timestamp())
            return self.make_cond_object(FilterOperationEnum.EQUAL, posix)

        def is_not(self, val):
            posix = int(val.timestamp())
            return self.make_cond_object(FilterOperationEnum.NOTEQUAL, posix)

        def greater(self, val):
            posix = int(val.timestamp())
            return self.make_cond_object(FilterOperationEnum.GREATER, posix)

        def greater_or_equal(self, val):
            posix = int(val.timestamp())
            return self.make_cond_object(FilterOperationEnum.GREATER_OR_EQUAL, posix)

        def less(self, val):
            posix = int(val.timestamp())
            return self.make_cond_object(FilterOperationEnum.LESS, posix)

        def less_or_equal(self, val):
            posix = int(val.timestamp())
            return self.make_cond_object(FilterOperationEnum.LESS_OR_EQUAL, posix)
        
        def is_in(self, values):
            posix_values = [int(val.timestamp()) for val in values]
            return self.make_cond_object(FilterOperationEnum.IS_IN, posix_values)
    
        def not_in(self, values):
            posix_values = [int(val.timestamp()) for val in values]
            return self.make_cond_object(FilterOperationEnum.IS_IN, posix_values)
        
        def between(self, low, high):
            return self.make_cond_object(FilterOperationEnum.BETWEEN,
                                         (int(low.timestamp()), int(high.timestamp())))
    
    class Link(BaseType):
        def __init__(self, default = -1):
            self.default = default
            self.size = 8

        def __set__(self, instance, value: T) -> None:
            if value is None:
                setattr(instance, '_' + self.name + '_id', -1)
                setattr(instance, '_' + self.name, None)
                return

            if isinstance(value, int):
                setattr(instance, '_' + self.name + '_id', value)
                return

            if not any(isinstance(value, typ) for typ in (ModelBaseMixin, ge2.Record, ge2.RecordView)):
                raise ValueError('Link can be None | int(PK) | instance of an object!')
            
            if not any(isinstance(value, typ) for typ in (ge2.Record, ge2.RecordView)) and value.is_new:
                raise ValueError('Record must be saved so it can be assigned to a parent!')

            # set both the id stored id _{field_name}_id & the actual model in  _{field_name}
            setattr(instance, '_' + self.name + '_id', int(value.id)) # type: ignore
            setattr(instance, '_' + self.name, value)
        
        def __get__(self, instance, owner) -> T:
            if not instance:
                return self
            
            linked : T = getattr(instance, '_' + self.name, None)
            return linked

    class Double(BaseType):
        def __init__(self, default: float = 0.0):
            self.default = default
            self.size = 8

        def __get__(self, instance, owner) -> float:
            if not instance:
                return self
            
            return getattr(instance, '_' + self.name, self.default)

        def __set__(self, instance, value: float):
            setattr(instance, '_' + self.name, value)

    class VirtualLink(BaseType):
        size: int = 0
        default = []
        name: str
        link_field: str

        def __init__(self, link_field: str):
            self.link_field = link_field

        def __get__(self, instance, owner) -> list:
            if not instance:
                return self
            
            return []

        def __set__(self, instance, value: list) -> None:
            # links should never be set on class instances
            return None
        
        def as_index(self: FO):
            raise AttributeError("VirtualLink attributes can't be indexed!")