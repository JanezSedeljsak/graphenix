from datetime import datetime
from .mixins.mixin_model_base import T
from .mixins.mixin_field_order import FO, FieldOrderMixin

class FieldTypeEnum:
    INT = 0
    STRING = 1
    BOOL = 2
    DATETIME = 3
    LINK = 4

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
