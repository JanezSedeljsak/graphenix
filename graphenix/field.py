from datetime import datetime
from .mixins.mixin_model_base import T
from .mixins.mixin_field_order import FO, FieldOrderMixin

import graphenix_engine2 as ge2

class FieldTypeEnum:
    INT = 0
    STRING = 1
    BOOL = 2
    DATETIME = 3
    LINK = 4
    DOUBLE = 5
    VIRTUAL_LINK = 6

class FilterOperationEnum:
    EQUAL = 0
    NOTEQUAL = 1
    GREATER = 2
    GREATER_OR_EQUAL = 3
    LESS = 4
    LESS_OR_EQUAL = 5
    REGEX = 6

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
        
        @staticmethod
        def make_cond_object(field_name, operation, cmp_value):
            cond_obj = ge2.cond_object()
            cond_obj.field_name = field_name
            cond_obj.operation_index = operation
            cond_obj.value = cmp_value
            return cond_obj
        
        def __eq__(self, val):
            return self.make_cond_object(self.name, FilterOperationEnum.EQUAL, val)

        def __ne__(self, val):
            return self.make_cond_object(self.name, FilterOperationEnum.NOTEQUAL, val)

        def __gt__(self, val):
            return self.make_cond_object(self.name, FilterOperationEnum.GREATER, val)

        def __ge__(self, val):
            return self.make_cond_object(self.name, FilterOperationEnum.GREATER_OR_EQUAL, val)

        def __lt__(self, val):
            return self.make_cond_object(self.name, FilterOperationEnum.LESS, val)

        def __le__(self, val):
            return self.make_cond_object(self.name, FilterOperationEnum.LESS_OR_EQUAL, val)
        
        def regex(self, val):
            raise Exception("Regex is only allowed on type String")


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
            return BaseType.make_cond_object(self.index, FilterOperationEnum.REGEX, val)

    class Bool(BaseType):
        def __init__(self, default: bool = False):
            self.default = default
            self.size = 1

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
            
            diff: int = getattr(instance, '_' + self.name, self.default)
            return datetime.fromtimestamp(diff)
        
        def __set__(self, instance, value: datetime | int):
            if isinstance(value, int):
                setattr(instance, '_' + self.name, int(value))
                return

            diff = int(value.timestamp())
            setattr(instance, '_' + self.name, diff)

        def __eq__(self, val):
            posix = int(val.timestamp())
            return self.make_cond_object(self.name, FilterOperationEnum.EQUAL, posix)

        def __ne__(self, val):
            posix = int(val.timestamp())
            return self.make_cond_object(self.name, FilterOperationEnum.NOTEQUAL, posix)

        def __gt__(self, val):
            posix = int(val.timestamp())
            return self.make_cond_object(self.name, FilterOperationEnum.GREATER, posix)

        def __ge__(self, val):
            posix = int(val.timestamp())
            return self.make_cond_object(self.name, FilterOperationEnum.GREATER_OR_EQUAL, posix)

        def __lt__(self, val):
            posix = int(val.timestamp())
            return self.make_cond_object(self.name, FilterOperationEnum.LESS, posix)

        def __le__(self, val):
            posix = int(val.timestamp())
            return self.make_cond_object(self.name, FilterOperationEnum.LESS_OR_EQUAL, posix)
    
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
            
            return getattr(instance, '_' + self.name, self.default)

        def __set__(self, instance, value: list) -> None:
            """
            This should only be assigned when creating a class instance from view
            """
            setattr(instance, '_' + self.name, value)