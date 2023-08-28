from graphenix import Field, Schema, Model, ViewSearilizer

class User(Model):
    first_name = Field.String(size=15)
    last_name = Field.String(size=15)
    email = Field.String(size=50)
    age = Field.Int()
    is_admin = Field.Bool()
    created_at = Field.DateTime()
    tasks = Field.VirtualLink("owner")
    city = Field.Link()

class Task(Model):
    name = Field.String(size=20)
    owner = Field.Link()
    subtasks = Field.VirtualLink("parent_task")

class City(Model):
    name = Field.String(size=50)
    country = Field.String(size=50)
    population_thousands = Field.Int()
    users = Field.VirtualLink("city")

class SubTask(Model):
    name = Field.String(size=40)
    date_created = Field.DateTime()
    parent_task = Field.Link()

class IndexedSubTask(Model):
    name = Field.String(size=40)
    parent_task = Field.Link().as_index()

class IndexedTower(Model):
    name = Field.String(size=40)
    height = Field.Double().as_index()

class UserBasicSearilizer(ViewSearilizer):
    fields = ('id', 'first_name', 'last_name', 'is_admin')

class CitySearilizer(ViewSearilizer):
    fields = ('name', 'population_thousands', 'users')
    users = UserBasicSearilizer

class SubTaskSearilizer(ViewSearilizer):
    fields = ('id', 'name', 'date_created')

class TaskSearilizer(ViewSearilizer):
    fields = ('name', 'subtasks')
    subtasks = SubTaskSearilizer

class DeepUserSearilizer(ViewSearilizer):
    fields = '*'
    tasks = TaskSearilizer
    city = CitySearilizer

mock_schema = Schema('mock_schema', models=[User, City, Task, SubTask, IndexedSubTask, IndexedTower])