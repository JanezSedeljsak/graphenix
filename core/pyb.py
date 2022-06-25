import enum
from dataclasses import dataclass

class Type(enum.Enum):
    BOOL = 1
    INT = 2
    STR = 3
    STR_FIXED = 4
    DATETIME = 5
    FILE = 6
    LINK = 7

@dataclass
class Column:
    name: str
    size: int
    actualType: Type
    indexed: bool

    def __init__(self, name: str, actualType: Type, size: int = None, indexed: bool = False):
        self.name = name
        self.actualType = actualType
        self.size = size if size is not None else 8
        self.indexed = indexed

@dataclass
class Link(Column):
    def __init__(self, name: str):
        self.name = name
        self.actualType = Type.LINK
        self.size = 8
        self.indexed = True

@dataclass
class Model:
    name: str
    slots: list[Column]

    def __init__(self, name: str, slots: list[Column] = None):
        self.name = name
        self.slots = slots if slots is not None else []

@dataclass
class Schema:
    name: str
    models: list[Model]

    def __init__(self, name: str, models: list[Model] = None):
        self.name = name
        self.models = models if models is not None else []