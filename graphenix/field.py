from datetime import datetime, timedelta

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
        def __init__(self, default=0):
            self.default = default
            self.size = 8

        def __set__(self, instance, value):
            setattr(instance, '_' + self.name, int(value))

    class String(BaseType):
        def __init__(self, size=255, default=''):
            self.size = size
            self.default = default

    class Bool(Int):
        def __init__(self, default=0):
            self.default = default
            self.size = 1

        def __get__(self, instance, owner):
            return int(getattr(instance, '_' + self.name, self.default))

        def __set__(self, instance, value):
            setattr(instance, '_' + self.name, int(value) != 0)

    class DateTime(BaseType):
        """ Datetime field is stored as ammount of seconds from 1.1.1970 (POSIX) format """
        epoch = datetime.utcfromtimestamp(0)

        def __init__(self, default=0):
            self.default = default
            self.size = 20

        def __get__(self, instance, owner):
            diff: int = getattr(instance, '_' + self.name, self.default)
            dt = self.epoch + timedelta(seconds=diff)
            return dt
        
        def __set__(self, instance, value):
            if isinstance(value, str):
                value = datetime.strptime(value, '%Y-%m-%d %H:%M:%S')

            diff = int((value - self.epoch).total_seconds())
            setattr(instance, '_' + self.name, diff)
            



