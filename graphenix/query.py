
class Query:
    def __init__(self, model):
        self.base_model = model

    def filter(self) -> "Query":
        ...

    def limit(self, count) -> "Query":
        ...

    def order(self, attr, asc: bool = True) -> "Query":
        ...

    def all(self) -> list[object]:
        data = []
        for i in range(2):
            data.append(self.base_model.get(i))

        return data

    def first(self) -> object:
        ... # this exectures the actual query to the schema engine

    def link(self, link_map: dict) -> "Query":
        ...
