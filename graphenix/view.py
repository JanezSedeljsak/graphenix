class QueryView(list):
    
    def __init__(self, view_obj):
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