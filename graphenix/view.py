from .field import Field

class QueryView(list):
    
    def __init__(self, model, view_obj):
        self.model = model
        self.view_obj = view_obj

    def __len__(self):
        return self.view_obj.size()

    def __getitem__(self, index):
        return self.view_obj.at(index)

    def __iter__(self):
        counter = 0
        while counter < len(self):
            yield self[counter]
            counter += 1

    def as_dict(self) -> list[dict]:
        return self.view_obj.as_dict()
    
    def as_tuple(self) -> list[tuple]:
        return self.view_obj.as_tuple()
    
    def __repr__(self):
        is_not_linkable = lambda field: not any(typ == self.model._field_types.get(field) 
                                                for typ in [Field.Link, Field.VirtualLink])

        field_names = ['id', *[fname for fname in self.model._model_fields if is_not_linkable(fname)]]
        rows = [self.view_obj.at(i).as_dict() for i in range(self.view_obj.size())]

        column_widths = [max(len(str(field)), max(len(str(row[field])) for row in rows)) for field in field_names]
        header = '  '.join(f"{field:{width}}" for field, width in zip(field_names, column_widths))
        str_builder = [header]
        for row in rows:
            formatted_row = '  '.join(f"{str(row.get(field, '')):{width}}" for field, width in zip(field_names, column_widths))
            str_builder.append(formatted_row)

        return "\n".join(str_builder)


    def __str__(self):
        return self.__repr__()
