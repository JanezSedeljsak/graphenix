import graphenix_engine

class Schema:
    def __init__(self, name: str, *models):
        self.name = name
        self.models = {}
        for model_class in models:
            model_class.__db__ = self.name
            setattr(self, name, model_class)
            self.models[model_class.__name__] = model_class
    
    def __getitem__(self, key: str):
        return self.models[key]

    def create(self):
        model_names = [m_name for m_name in self.models.keys()]
        graphenix_engine.create_schema(self.name, model_names)

    def delete(self):
        graphenix_engine.delete_schema(self.name)

    def migrate(self):
        # Call the engine function to migrate the database schema
        pass

    def exists(self) -> bool:
        return graphenix_engine.schema_exists(self.name)