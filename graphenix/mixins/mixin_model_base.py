from typing import TypeVar
import abc

T = TypeVar('T', bound='ModelBaseMixin')

class ModelBaseMixin(metaclass=abc.ABCMeta):

    _id: int = -1
    _db: None | str

    # cached values
    _mdef: object
    _cache_init: bool = False
    _model_fields: list = []
    #_total_size: int = 0
    #_field_sizes: dict[str, int] = {}
    #_field_types_raw: dict[str, int] = {} # for parsing in c++
    _field_types: dict[str, type] = {}
    _field_defaults: dict = {} # can't add type hints since there are different types for default (base of field type)

    @abc.abstractmethod
    def get_values(self, fields) -> list:
        raise NotImplementedError

    @abc.abstractmethod
    def save(self) -> None:
        raise NotImplementedError
    
    @abc.abstractclassmethod
    def get(cls, id: int):
        raise NotImplementedError

    @abc.abstractclassmethod
    def make_cache(cls):
        raise NotImplementedError
    
    @abc.abstractclassmethod
    def get_mdef(cls):
        raise NotImplementedError
    

    