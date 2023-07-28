from datetime import datetime

class ViewSearilizer:

    fields = ()
    required = ()

    @classmethod
    def validate(cls, data):
        if isinstance(data, list):
            return all(cls.validate(row) for row in data)

        if isinstance(data, dict):
            keys_with_values = set(key for key, val in data.items() if (val or val == 0))
            diff = set(cls.required).difference(keys_with_values)
            if diff:
                return False
            
            for field in cls.required:
                current_value = data[field]
                if isinstance(current_value, list) or isinstance(current_value, dict):
                    if not hasattr(cls, field):
                         raise AttributeError(f'Field - "{field}" should have a searilizer!')
                    
                    searilizer = getattr(cls, field)
                    if not issubclass(searilizer, ViewSearilizer):
                        raise AttributeError(f'Field - "{field}" is not as a searilizer!')

                    if not searilizer.validate(current_value):
                        return False

            return True

        return False

    @classmethod
    def jsonify(cls, data):
        if isinstance(data, list):
            return [cls.jsonify(row) for row in data]
        
        elif isinstance(data, datetime):
            return data.isoformat()
        
        elif isinstance(data, tuple) and hasattr(data, '_asdict'):
            tuple_as_dict = data._asdict() # type: ignore
            res_dict = {}
            for field in cls.fields:
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
                if not issubclass(searilizer, ViewSearilizer):
                    raise AttributeError(f'Field - "{field}" is not as a searilizer!')
                
                res_dict[field] = searilizer.jsonify(tvalue)

            return res_dict

        return data