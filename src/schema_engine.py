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
    BLOB = 6
    LINK = 7

class Type:
    type = _Type.UNKNOWN
    index: bool = False
    multi: bool = False # only used for Link
    size: int = 0
    default: Callable = lambda : None

    def as_index(self):
        """
        Creates internal search index for column (seach will be O(log(n)) instead of O(n), but it will take more space)
        """
        if type == _Type.LINK:
            raise TypeError("LINK type cannot be indexed")

        self.index = True
        return self

    def set_default(self, func: Callable):
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
    
    class Blob(Type):
        """
        Creates file and links via uuid4 name
        """
        size = 37 # uuid for filename
        type = _Type.BLOB

    class Link(Type):
        """
        Creates internal collection of references to model
        """
        default = lambda : []
        type = _Type.LINK
        
        @property
        def size(self):
            # size of links depends on multi value
            return 0 if self.multi else 4

        def __init__(self, model: str, multi=True):
            self.model = model
            self.multi = multi

class Model:
    slots: dict
    lazy_delete: bool
    _name: str

    @property
    def slots_(self):
        """
        Returns compressed slots of model + their names
        """
        return [(sname, slot.compressed_, slot.size) for sname, slot in self.slots.items()]

    def __init__(self, slots: dict, lazy_delete: bool = False):
        """
        Creates instance of Model
        """
        self.slots = slots if slots is not None else {}
        self.lazy_delete = lazy_delete

    def insert(self) -> bool:
        ...

    def query(self) -> Any:
        ...

    def edit(self) -> bool:
        ...

    def delete(self) -> bool:
        ...


class Schema:
    version: int
    is_synced: bool
    name: str
    _models: dict

    @property
    def parsed_(self):
        """
        Returns parsed schema for C API
        eg. [(mname, lazy_delete, [(sname, _compressed_column), ]), ]
        """
        return [(mdef._name, int(mdef.lazy_delete), mdef.slots_) for mdef in self._models.values()]

    def __init__(self, name: str, models = None) -> None:
        """
        Creates instance of Schema
        """
        self.name = name
        self._models = models if models is not None else {}
        for mname, mdef in self._models.items():
            mdef._name = mname

    def __str__(self) -> str:
        """
        Schema instance to string
        """
        return PYB_Core.stringify_schema(self.name, self.parsed_)

    def __getitem__(self, key: str):
        """
        Gets schema model by name
        """
        res = self._models.get(key)
        if res is None:
            raise KeyError(f"\"{key}\" model not found in \"{self.name}\" - schema")

        return res

    def __setitem__(self, mname: str, mdef: Model) -> None:
        """
        Sets schema model by key
        """
        self.is_synced = False
        self._models[mname] = mdef
        mdef._name = mname

    def __delitem__(self, mname: str) -> None:
        """
        Deletes model by name
        """
        if mname not in self._models:
            raise KeyError(f"\"{mname}\" model not found in \"{self.name}\" - schema")

        del self._models[mname]
        self.is_synced = False

    def __iter__(self) -> Iterator:
        for pair in self._models.items():
            yield pair

    def create(self) -> bool:
        """
        Creates Schema on system. (if it already exists -> error)
        """
        success = PYB_Core.create_schema(self.name, self.parsed_)
        if success:
            self.is_synced = True
        
        return success


    def migrate(self) -> bool:
        """
        Updates schema definition and all models (if schema doesn't exist -> error)
        """
        success = PYB_Core.migrate_schema(self.name, self.parsed_)
        if success:
            self.is_synced = True

        return success

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
    def load(file_name: str) -> tuple:
        """
        Reads schema from a file
        """
        return True, PYB_Core.load_schema(file_name)
