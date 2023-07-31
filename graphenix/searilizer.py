from datetime import datetime
from .mixins.mixin_model_base import ModelBaseMixin
from .view import QueryView
import graphenix_engine2 as ge2
import csv

class ViewSearilizer:

    fields = ()
    required = ()
    instance = None
    model: ModelBaseMixin | None = None

    @classmethod
    def is_all(cls):
        return isinstance(cls.fields, str) and cls.fields == '*'
    
    @classmethod
    def default(cls):
        if cls.instance is not None:
            return cls.instance
        
        class AllSearilizer(ViewSearilizer):
            fields = '*'
            
        cls.instance = AllSearilizer
        return cls.instance
    
    @classmethod
    def dump2csv(cls, data: list, filename: str) -> bool:
        rows = cls.jsonify(data)
        if not rows:
            return False

        keys = rows[0].keys()
        try:
            with open(filename, 'w', newline='', encoding='utf-8') as csvfile:
                writer = csv.DictWriter(csvfile, fieldnames=keys, delimiter=';')
                
                writer.writeheader()
                writer.writerows(rows)
            
            return True
        
        except Exception as err:
            print(f'Error writting to csv {err}')
            return False

    @classmethod
    def jsonify(cls, data):
        if not cls.is_all():
            fields = cls.fields if not isinstance(cls.fields, str) else (cls.fields,)

        if isinstance(data, ge2.View):
            data = QueryView(data)
        
        if isinstance(data, list):
            return [cls.jsonify(row) for row in data]
        
        if isinstance(data, datetime):
            return data.isoformat()        

        if isinstance(data, ge2.Record) or isinstance(data, ge2.RecordView):
            tuple_as_dict = data.as_dict() # type: ignore
            res_dict = {}
            fields_list = fields if not cls.is_all() else list(tuple_as_dict.keys())
            for field in fields_list:
                if field not in tuple_as_dict:
                    res_dict[field] = None
                    continue
                
                tvalue = tuple_as_dict[field]
                if not hasattr(cls, field):
                    if isinstance(tvalue, tuple) or isinstance(tvalue, list):
                        raise AttributeError(f'Field - "{field}" should have a searilizer!')
                    
                    res_dict[field] = cls.jsonify(tvalue)
                    continue

                searilizer = getattr(cls, field)
                if not isinstance(searilizer, type) or not issubclass(searilizer, ViewSearilizer):
                    raise AttributeError(f'Field - "{field}" is not as a searilizer!')
                
                res_dict[field] = searilizer.jsonify(tvalue)

            return res_dict
        
        if isinstance(data, ModelBaseMixin):
            fields_list = fields if not cls.is_all() else ['id', *data._model_fields]
            res_dict = {field: getattr(data, field) for field in fields_list}
            return res_dict

        return data
