from typing import TypeVar
import abc

FO = TypeVar('FO', bound='FieldOrderMixin')

class FieldOrderMixin(metaclass=abc.ABCMeta):
    size: int
    index: bool
    name: str
    asc: bool = True
    link_field: str | None = None
        
    @abc.abstractmethod
    def __set_name__(self, owner, name) -> None:
        raise NotImplementedError

    @abc.abstractmethod
    def as_index(self: FO) -> FO:
        raise NotImplementedError
    
    @abc.abstractmethod
    def desc(self: FO) -> str:
        """ 
        If we do Field.desc() we only return the name of the field.
        Since we only returned a str we will know that that is a name of a field that 
        needs to be ordered desc.
        """
        raise NotImplementedError


