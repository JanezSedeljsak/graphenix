from graphenix import Field, Schema, Model

class User(Model):
    first_name = Field.String(size=15)
    last_name = Field.String(size=15)
    email = Field.String(size=50)
    age = Field.Int()
    is_admin = Field.Bool()
    created_at = Field.DateTime()

class City(Model):
    name = Field.String(size=50)
    country = Field.String(size=50)
    population_thousands = Field.Int()

mock_schema = Schema('test_school', models=[User, City])