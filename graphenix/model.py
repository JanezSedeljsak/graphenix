import graphenix_engine2 as ge2

from .mixins.mixin_model_base import ModelBaseMixin, T
from .mixins.mixin_model_filtering import ModelFilteringMixin
from .mixins.enums import FieldTypeEnum
from .query import ModelQueryMixin
from .field import Field
from typing import Type

class Model(ModelBaseMixin, ModelQueryMixin, ModelFilteringMixin):
    _db = None

    def __init__(self, **fields):
        self._id = -1
        self.__name__ = self.__class__.__name__
        for key, value in fields.items():
            if key in self._model_fields or key == '_id':
                setattr(self, key, value)


    def __str__(self):
        self.make_cache()
        fields = ['id', *self._model_fields]
        attrs = ', '.join([f"{k}={getattr(self, k)}" for k in fields if not self._field_types.get(k) == Field.VirtualLink])
        return f"{self.__name__}({attrs})"
    
    def __setattr__(self, name, value):
        self.make_cache()
        if name not in self._model_fields and not name.startswith("_"):
            raise AttributeError(f"Cannot assign member '{name}' for type '{self.__name__}'")
        
        super().__setattr__(name, value)

    @property
    def id(self):
        return self._id
    
    @property
    def is_new(self):
        return self.id == -1
    
    @classmethod
    def desc(cls: Type[T]) -> str:
        return 'id'
    
    @classmethod
    def from_view(cls: Type[T], record_view) -> T:
        if not isinstance(record_view, ge2.Record):
            raise TypeError(f"Record should be of type Record - got {type(record_view)}")
        
        view_dict = record_view.as_dict()
        record_id = view_dict.pop('id')
        rec = cls(**view_dict)
        rec._id = record_id
        return rec

    @classmethod
    def make_cache(cls: Type[T]) -> None:
        if cls._cache_init:
            return
        
        if not cls._db:
            raise AttributeError("Model is missing a _db property!")
        
        mdef = ge2.model_def()
        mdef.db_name = cls._db
        mdef.model_name = cls.__name__

        cls._model_fields = [attr for attr, val in cls.__dict__.items() if isinstance(val, Field.BaseType)]
        mdef.field_names = cls._model_fields

        field_sizes_dict = {}
        field_types_raw_dict = {}
        cls._field_types = {}
        field_offsets: list[int] = []
        field_indexes: list[bool] = []
        current_offset: int = 0
        field_date_indexes: list[bool] = []

        for field_name, field in cls.__dict__.items():
            if isinstance(field, Field.BaseType):
                if not field.size and not isinstance(field, Field.VirtualLink):
                    raise AttributeError(f'Size for field {field_name} is not defined or is not a positive integer!')

                field_sizes_dict[field_name] = field.size
                field_offsets.append(current_offset)
                field_indexes.append(field.index)
                current_offset += field.size
                actual_type = type(field)
                cls._field_types[field_name] = actual_type
                cls._field_defaults[field_name] = field.default
                is_date = False

                match actual_type:
                    case Field.Int:
                        raw_type_index = FieldTypeEnum.INT
                    case Field.String:
                        raw_type_index = FieldTypeEnum.STRING
                    case Field.Bool:
                        raw_type_index = FieldTypeEnum.BOOL
                    case Field.DateTime:
                        raw_type_index = FieldTypeEnum.DATETIME
                        is_date = True
                    case Field.Link:
                        raw_type_index = FieldTypeEnum.LINK
                    case Field.Double:
                        raw_type_index = FieldTypeEnum.DOUBLE
                    case Field.VirtualLink:
                        raw_type_index = FieldTypeEnum.VIRTUAL_LINK
                    case _:
                        raise AttributeError("Field type is not valid!")

                field_types_raw_dict[field_name] = raw_type_index
                field_date_indexes.append(is_date)
        
        mdef.record_size = sum(field_sizes_dict.values())
        mdef.field_sizes = [field_sizes_dict[field] for field in cls._model_fields]
        mdef.field_types = [field_types_raw_dict[field] for field in cls._model_fields]
        mdef.field_offsets = field_offsets
        mdef.field_indexes = field_indexes
        mdef.field_date_indexes = [dindex for dindex, is_date in enumerate(field_date_indexes) if is_date]

        cls._mdef = mdef
        cls._cache_init = True

    @classmethod
    def get_mdef(cls: Type[T]) -> object:
        cls.make_cache()
        return cls._mdef
    
    @classmethod
    def bulkcreate(cls: Type[T], rows: list) -> None:
        cls.make_cache()
        for row in rows:
            ge2.model_add_record(cls._mdef, row)

    @classmethod
    def bulkdelete(cls: Type[T], ids:  list[int]) -> None:
        cls.make_cache()
        for rec_id in ids:
            ge2.model_delete_record(cls._mdef, rec_id)

    @classmethod
    def get(cls: Type[T], record_id: int):
        cls.make_cache()

        record = ge2.model_get_record(cls._mdef, record_id) 
        record_as_dict = {field: record[idx] for idx, field in enumerate(cls._model_fields)}
        instance = cls(**record_as_dict)
        instance._id = record_id
        return instance
    
    def get_values(self, fields) -> list:
        result: list = []
        for field in fields:
            match self._field_types[field]:
                case Field.Link:
                    result.append(getattr(self, '_' + field + '_id', self._field_defaults[field]))
                case _:
                    result.append(getattr(self, '_' + field, self._field_defaults[field]))
        
        return result
    
    def delete(self):
        if self.is_new:
            raise Exception("Record doesn't exist in the db!")

        ge2.model_delete_record(self._mdef, self.id)
        self._id = -1 # set flag to is_new again so you don't update an inactive record

    def make(self: T) -> T:
        self.save()
        return self
    
    def connect_child(self: T, child_model: type, field_name: str) -> None:
        if getattr(self, field_name) is not None:
            return
        
        if not hasattr(self, f'_{field_name}_id'):
            return
        
        child_id = getattr(self, f'_{field_name}_id')
        if (not child_id and child_id != 0)  or child_id == -1:
            return
        
        child = child_model.get(child_id)
        setattr(self, field_name, child)

    
    def save(self):
        self.make_cache()
        values_as_list = self.get_values(self._model_fields)

        if self.is_new:
            self._id = ge2.model_add_record(self._mdef, values_as_list)
        else:
            ge2.model_update_record(self._mdef, values_as_list, self.id)



class IXModel(Model):
    """
    Subclass of Model with B-tree index on primary key (PK).

    This class represents a specialized graphenix Model that inherits from the base Model class.
    It extends the functionality by adding a B-tree index on the primary key for efficient lookups.
    """

    pk_index = True