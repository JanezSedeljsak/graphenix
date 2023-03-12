class Query:
    def __init__(self, db, model, **query):
        self.db = db
        self.model = model
        self.query = query

    def all():
        ... # this executes the actual query to the schema engine

    def first():
        ... # this exectures the actual query to the schema engine

    def delete():
        ... # this will delete all items in the db that fit the query
