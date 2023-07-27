from datetime import datetime

class ViewSearilizer:

    fields = ()

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
