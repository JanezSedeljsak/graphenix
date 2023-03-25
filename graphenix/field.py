from datetime import datetime, timedelta

class FieldTypeEnum:
    INT = 0
    STRING = 1
    BOOL = 2
    DATETIME = 3
    LINK_SINGLE = 4
    LINK_MULTIPLE = 5

class Field:

    class BaseType:
        size = None
        default = None

        def __get__(self, instance, owner):
            return getattr(instance, '_' + self.name, self.default)
        
        def __set__(self, instance, value):
            setattr(instance, '_' + self.name, value)
        
        def __set_name__(self, owner, name):
            self.name = name

    class Int(BaseType):
        def __init__(self, default: int = 0):
            self.default = default
            self.size = 8

        def __get__(self, instance, owner) -> int:
            return getattr(instance, '_' + self.name, self.default)

        def __set__(self, instance, value):
            setattr(instance, '_' + self.name, value)

    class String(BaseType):
        def __init__(self, size=255, default: str = ''):
            self.size = size
            self.default = default

    class Bool(Int):
        def __init__(self, default: bool = False):
            self.default = default
            self.size = 1

        def __get__(self, instance, owner) -> bool:
            return getattr(instance, '_' + self.name, self.default)

        def __set__(self, instance, value):
            setattr(instance, '_' + self.name, int(value))

    class DateTime(BaseType):
        """ Datetime field is stored as ammount of seconds from 1.1.1970 (POSIX) format """
        epoch = datetime.utcfromtimestamp(0)

        def __init__(self, default = 0):
            self.default = default
            self.size = 8

        def __get__(self, instance, owner) -> datetime:
            diff: int = getattr(instance, '_' + self.name, self.default)
            dt = self.epoch + timedelta(seconds=diff)
            return dt
        
        def __set__(self, instance, value: datetime | int):
            if isinstance(value, int):
                setattr(instance, '_' + self.name, int(value))
                return

            diff = int((value - self.epoch).total_seconds())
            setattr(instance, '_' + self.name, diff)
            



