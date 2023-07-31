# Graphenix - Embedded Database System for Python written in C++

Graphenix is an embedded database system for Python that allows you to easily store and retrieve data within your Python application. The core of the Graphenix library is written in C++, making it a high-performance solution for managing data.

## Requirements

Graphenix requires Python 3.10 or higher to be installed.

## Usage

To use Graphenix, you can create models that inherit from the `Model` class provided by Graphenix. You can define the fields for your model using the various field types provided by Graphenix (e.g. `gx.Field.String`, `gx.Field.Int`, `gx.Field.DateTime`, etc.).

### Schema definition
Here is an example of how you might define two models, `Teacher` and `Laboratory`, using Graphenix (each user can be a part of one laboratory):
```py
import graphenix as gx

class Teacher(gx.Model):
    full_name = gx.Field.String(size=256)
    email = gx.Field.String(size=128)
    laboratory = gx.Field.Link()

class Laboratory(gx.Model):
    name = gx.Field.String(size=64)
    room_number = gx.Field.Int()
    teachers = gx.Field.VirtualLink('laboratory')
```

Once you have defined your models, you can create a schema and add your models to it:
```py
my_schema = gx.Schema('my_schema', models=[Teacher, Laboratory])
```

You can then create the schema and any necessary tables by calling the `create()` method on your schema:
```py
my_schema.create(delete_old=True)
```
### Record operations
To save data to your database, you can create an instance of your model and call the `save()` method:
```py
biolab = Laboratory(name="Bioinformatics", room_number=44) 
biolab.save()
```

To retrieve data from your database, you can use the `get()` method (this get's the row by PK):
```py
lab = Laboratory.get(0)
```

You can then modify the retrieved object and call the `save()` method to update it in the database:
```py
lab.room_number = 190
lab.save()
```

### Querying

#### `.all()`
The .all() method retrieves all records that match the fields defined in the query object. This method is ideal for fetching multiple records that satisfy the query conditions. It returns a the count and a generator of records, where each record is represented as a `namedtuple` containing the fields retrieved from the database.

```python
_, labs_list = Laboratory.link(teachers = Teacher).all()
```

#### `.first()`
The .first() method retrieves the first record that matches the query object. It is commonly used when you need to get a single record that fulfills certain criteria. The method returns a single record as a `namedtuple`, or None if no matching record is found.

```python
lab = Laboratory.filter(Laboratory.name.equals('Bioinformatics')).first()
```
#### `.agg()`
The .agg() method is designed for performing aggregation operations on the data, such as counting the number of records, calculating the sum, finding the minimum or maximum values. It is useful when you need to gather summarized information from the dataset.

```sql
SELECT t.lab_id, COUNT(*)
FROM teachers AS t
GROUP BY t.lab_id
```
The SQL statement translates to:
```python
lab_stats = Teacher.agg(by=Teacher.laboratory, count=gx.AGG.count())
```

#### Query operations

#### `.link()`

Link is used to join data either by a direct or a virtual link:
- `VirtualLink` -> for a specific lab we can get a list of teachers that are in that lab (```query = Laboratory.link(teachers = Teacher)```)
- `Link` (direct link) -> for a specific teacher we get the lab he is in (```query = Teacher.link(laboratory = Laboratory)```)

#### `.filter()`

The method is meant to replace the `WHERE` operator in `SQL` it is a bit different it can only be used within the entity you are querying. It supports
9 operations (equals, is_not, greater, greater_or_equal, less, less_or_equal, regex(), is_in(), not_in()) + we can also wrap these methods into a condition tree like so:
```python
query = Teacher.filter(
    Teacher.full_name.is_not('John'),
    gx.some(Teacher.is_in([1,2,3]), Teacher.email.regex('.*@gmail.*'))
)
```

We have 2 options when we want to create a condition tree (`.gx.every(...)`, `.gx.some(...)`) which are a replacement for multiple AND, OR operators.

#### `.limit(), .offset()`

Limit and offset are usually used to implement pagging or when we just want a specific amount of records or skip a few.
The following query would retrieve the 3rd page of records, where each page is 15 records long:
```python
query = Teacher.offset(30).limit(15)
```

#### `.order()`

The order operator is quite straight forward we can sort records from one entity by multiple columns (ASC/DESC).
For example if we want to sort the Teachers by full_name and in case of the same name we do descending by email:
```python
query = Teacher.order(Teacher.full_name, Teacher.email.desc())
```

#### Data searilization

For returning data we usually want a `JSON` format. We took a similar approach to the `DJANGO` framework and used `searilizers`.
If we want to return teachers with keys `id`, `email` we can just do:
```python
class TeacherBasicSearilizer(gx.ViewSearilizer):
    fields = ('id', 'email')
```

Then we can also add nested searilizers when we have a more complex structure:
```python
class TeacherSearilizer(gx.ViewSearilizer):
    fields = ('id', 'full_name', 'email')

class LaboratorySearilizer(gx.ViewSearilizer):
    fields = '*'
    teachers = TeacherSearilizer
```

The query and response would then be something like this:
```python
_, labs_list = Laboratory.link(
    teachers = Teacher.filter(Teacher.email.regex('.*@gmail.*'))
).all()
parsed = LaboratorySearilizer.jsonify(labs_list)
```

```json
[
    {
        "name": "Computer Vision",
        "room_number": 123,
        "teachers": [
            {"id": 0, "full_name": "John Doe", "email": "john@gmail.com"},
        ]
    },
]
```

### An example of a more complex query object tree

Suppose we have the following database schema:
```python
class User(gx.Model):
    first_name = gx.Field.String(size=15)
    last_name = gx.Field.String(size=15)
    email = gx.Field.String(size=50)
    age = gx.Field.Int()
    is_admin = gx.Field.Bool()
    created_at = gx.Field.DateTime()
    tasks = gx.Field.VirtualLink("owner")
    laboratory = gx.Field.Link()

class Task(gx.Model):
    name = gx.Field.String(size=20)
    owner = gx.Field.Link()
    subtasks = gx.Field.VirtualLink('parent_task')

class SubTask(gx.Model):
    name = gx.Field.String(size=40)
    date_created = gx.Field.DateTime()
    parent_task = gx.Field.Link()

class Laboratory(gx.Model):
    name = gx.Field.String(size=50)
    room_number = gx.Field.Int()
    users = gx.Field.VirtualLink('laboratory')
```

The following query is just an example of what is possible, it is not meant to be anything logical:
```python
count, data = User\
    .filter(User.age.not_in([15, 65, 44]), User.email.regex('.*@example.*'))\
    .order(User.first_name, User.last_name.desc())\
    .link(
        tasks = Task.offset(1).limit(3).link(subtasks = SubTask.filter(
                gx.some(
                    SubTask.greater(10),
                    SubTask.name.is_in(['SubTask 4', 'SubTask 3'])
                )
            )
        ),
        laboratory = Laboratory.link(
            users = User.order(User.desc()).filter(
                gx.some(
                    User.is_in(User.filter(User.is_admin.equals(True)).pick_id()),
                    User.last_name.equals('')
                )
            )
        )
    ).all()
```

## License

Graphenix is released under the MIT license. See the `LICENSE` file for more details.

## Authors
- [Janez Sedeljšak](https://github.com/JanezSedeljsak)


