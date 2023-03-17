import graphenix_engine

class Schema:
    def __init__(self, name: str, models=None):
        self.name = name
        self.models = {}
        for model_class in (models or []):
            model_class.__db__ = self.name
            setattr(self, name, model_class)
            self.models[model_class.__name__] = model_class
    
    def __getitem__(self, key: str):
        return self.models[key]

    def create(self, delete_old=False):
        model_names = [m_name for m_name in self.models.keys()]
        graphenix_engine.create_schema(self.name, model_names, delete_old)

    def delete(self):
        graphenix_engine.delete_schema(self.name)

    def migrate(self):
        # Call the engine function to migrate the database schema
        pass

    def exists(self) -> bool:
        return graphenix_engine.schema_exists(self.name)

