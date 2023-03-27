from typing import TypeVar
import abc

T = TypeVar('T', bound='ModelBaseMixin')

class ModelBaseMixin(metaclass=abc.ABCMeta):

    _id = -1
    
    @abc.abstractmethod
    def get(self, id: int) -> T:
        ...

    @abc.abstractmethod
    def save(self) -> None:
        ...

    