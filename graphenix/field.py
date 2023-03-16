
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


