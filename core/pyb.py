from enum import Enum
from collections.abc import Iterator, Callable
from datetime import datetime
from typing import Any
import pybase_core as PYB_Core

class _Type(Enum):
    UNKNOWN = 0
    BOOL = 1
    INT = 2
    STRING = 3
    TEXT = 4
    DATETIME = 5
    FILE = 6
    LINK = 7

class Type:
    type = _Type.UNKNOWN
    index = False
    multi = False # only used for Link
    default = lambda : None

    def as_index(self):
        """
        Creates internal search index for column (seach will be O(log(n)) instead of O(n), but it will take more space)
        """
        if type == _Type.LINK:
            raise ValueError("LINK type cannot be indexed")

        self.index = True
        return self

    def set_default(self, func: Callable[[], Any]):
        """
        Set value of default (has to be callable -> function is called when we create instance)
        """
        self.default = func
        return self

    @property
    def compressed_(self):
        """
        Unique value for field type 0-31
        """
        return self.index * 16 + self.multi * 8 + self.type.value

class Field:

    class Bool(Type):
        """
        Stores true/false
        """
        type = _Type.BOOL
        default = lambda : True
        size = 1

    class Int(Type):
        """
        Stores 32-bit integer
        """
        default = lambda : 0
        type = _Type.INT
        size = 4

    class String(Type):
        """
        Stores C string of length
        """
        type = _Type.STRING
        size: int
        default = lambda : ""

        def __init__(self, size: int = 255):
            self.size = size

    class Text(Type):
        """
        Creates txt file and stores uuid4
        """
        type = _Type.TEXT
        size = 37 # uuid for filename
        default = lambda : ""

    class DateTime(Type):
        """
        Datetime object stored as int-timestamp
        """
        default = lambda : int(round(datetime.now().timestamp()))
        type = _Type.DATETIME
        size = 8
    
    class File(Type):
        """
        Creates file and links via uuid4 name
        """
        size = 37 # uuid for filename
        type = _Type.FILE

    class Link(Type):
        """
        Creates internal collection of references to model
        """
        default = lambda : []
        type = _Type.LINK

        def __init__(self, model: str, multi=False):
            self.model = model
            self.multi = multi

class Model:
    slots: dict[str, Field]
    lazy_delete: bool
    _name: str

    @property
    def slots_(self):
        """
        Returns compressed slots of model + their names
        """
        return [(sname, slot.compressed_) for sname, slot in self.slots.items()]

    def __init__(self, slots: dict[str, Field], lazy_delete: bool = False):
        """
        Creates instance of Model
        """
        self.slots = slots if slots is not None else {}
        self.lazy_delete = lazy_delete


class Schema:
    name: str
    _models: dict[str, Model]

    @property
    def parsed_(self):
        """
        Returns parsed schema for C API
        eg. [(mname, lazy_delete, [(sname, _compressed_column), ]), ]
        """
        return [(mdef._name, int(mdef.lazy_delete), mdef.slots_) for mdef in self._models.values()]

    def __init__(self, name: str, models: dict[str, Model] = None) -> None:
        """
        Creates instance of Schema
        """
        self.name = name
        self._models = models if models is not None else {}
        for mname, mdef in self._models.items():
            mdef._name = mname


    def __getitem__(self, mname: str) -> Model:
        """
        Gets schema model by name
        """
        res = self._models.get(mname)
        if res is None:
            raise KeyError(f"\"{mname}\" model not found in \"{self.name}\" - schema")

        return res

    def __setitem__(self, mname: str, mdef: Model) -> None:
        """
        Sets schema model by key
        """
        self._models[mname] = mdef
        mdef._name = mname

    def __delitem__(self, mname: str) -> None:
        """
        Deletes model by name
        """
        if mname not in self._models:
            raise KeyError(f"\"{mname}\" model not found in \"{self.name}\" - schema")

        del self._models[mname]

    def __iter__(self) -> Iterator[tuple[str, Model]]:
        for pair in self._models.items():
            yield pair

    def create(self) -> None:
        """
        Creates Schema on system. (if it already exists -> error)
        """
        return PYB_Core.create_schema(self.name, self.parsed_)


    def migrate(self) -> bool:
        """
        Updates schema definition and all models (if schema doesn't exist -> error)
        """
        return PYB_Core.migrate_schema(self.name, self.parsed_)

    def delete(self) -> bool:
        """
        Deletes schema from filesystem
        """
        return PYB_Core.delete_schema(self.name)

    def print(self) -> None:
        """
        Prints schema via C API
        """
        PYB_Core.print_schema(self.name, self.parsed_)

    @staticmethod
    def load(file_name: str) -> tuple[bool, 'Schema']:
        """
        Reads schema from a file
        """
        return True, Schema(file_name, models={})
