# Graphenix - Embedded Database System for Python

Graphenix is an embedded database system for Python that allows you to easily store and retrieve data within your Python application. The core of the Graphenix library is written in C++, making it a high-performance solution for managing data.

## Requirements

Graphenix requires Python 3.10 or higher to be installed.

## Usage

To use Graphenix, you can create models that inherit from the `Model` class provided by Graphenix. You can define the fields for your model using the various field types provided by Graphenix (e.g. `Field.String`, `Field.Int`, `Field.DateTime`, etc.).

Here is an example of how you might define two models, `User` and `City`, using Graphenix:
```py
from graphenix import Field, Schema, Model
from datetime import datetime, timedelta

class User(Model):
    first_name = Field.String(size=15)

class City(Model):
    name = Field.String(size=50)
    country = Field.String(size=50)
    population = Field.Int()
    created = Field.DateTime()
```

Once you have defined your models, you can create a schema and add your models to it:
```py
mock_schema = Schema('test_school', models=[User, City])
```

You can then create the schema and any necessary tables by calling the `create()` method on your schema:
```py
mock_schema.create(delete_old=True)
```

To save data to your database, you can create an instance of your model and call the `save()` method:
```py
london = City(name="London",
              country="UK",
              population=10,
              created=datetime.now())

london.save()
```

To retrieve data from your database, you can use the `get()` method:
```py
city_from_db = City.get(0)
```

You can then modify the retrieved object and call the `save()` method to update it in the database:
```py
city_from_db.created += timedelta(days=10)
city_from_db.save()
```

## License

Graphenix is released under the MIT license. See the `LICENSE` file for more details.

## Authors
- [Janez Sedeljšak](https://github.com/JanezSedeljsak)


