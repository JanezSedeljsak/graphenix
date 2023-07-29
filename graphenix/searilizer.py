from datetime import datetime
import types

class ViewSearilizer:

    fields = ()
    required = ()

    @classmethod
    def is_all(cls):
        return isinstance(cls.fields, str) and cls.fields == '*'

    @classmethod
    def validate(cls, data):
        required = cls.required if not isinstance(cls.required, str) else (cls.required,)

        if isinstance(data, types.GeneratorType):
            data = list(data)
            
        if isinstance(data, list):
            return all(cls.validate(row) for row in data)

        if isinstance(data, dict):
            keys_with_values = set(key for key, val in data.items() if (val or val == 0))
            diff = set(required).difference(keys_with_values)
            if diff:
                return False
            
            for field in required:
                current_value = data[field]
                if isinstance(current_value, list) or isinstance(current_value, dict):
                    if not hasattr(cls, field):
                         raise AttributeError(f'Field - "{field}" should have a searilizer!')
                    
                    searilizer = getattr(cls, field)
                    if not isinstance(searilizer, type) or not issubclass(searilizer, ViewSearilizer):
                        raise AttributeError(f'Field - "{field}" is not as a searilizer!')

                    if not searilizer.validate(current_value):
                        return False

            return True

        return False

    @classmethod
    def jsonify(cls, data):
        if not cls.is_all():
            fields = cls.fields if not isinstance(cls.fields, str) else (cls.fields,)

        if isinstance(data, types.GeneratorType):
            data = list(data)

        if isinstance(data, list):
            return [cls.jsonify(row) for row in data]
        
        elif isinstance(data, datetime):
            return data.isoformat()
        
        elif isinstance(data, tuple) and hasattr(data, '_asdict'):
            tuple_as_dict = data._asdict() # type: ignore
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

        return data
