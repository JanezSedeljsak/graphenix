class Query:
    def __init__(self, model, **query):
        self.db = model.__db__
        self.model = model.__name__
        self.lazy_delete = model.__lazy_delete__
        self.query = query

    def all(self):
        ... # this executes the actual query to the schema engine

    def first(self):
        ... # this exectures the actual query to the schema engine

    def delete(self):
        if self.lazy_delete:
            ... # call lazy delete method under the hood
        else:
            ... # call full delete 
