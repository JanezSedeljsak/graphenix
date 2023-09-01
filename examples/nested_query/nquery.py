from graphenix import Field, Schema, Model, ViewSearilizer, some, every
from datetime import datetime
import json

class User(Model):
    first_name = Field.String(size=15)
    last_name = Field.String(size=15)
    email = Field.String(size=50)
    age = Field.Int()
    is_admin = Field.Bool()
    created_at = Field.DateTime()
    tasks = Field.VirtualLink("owner")
    laboratory = Field.Link()

class Task(Model):
    name = Field.String(size=20)
    owner = Field.Link()
    subtasks = Field.VirtualLink('parent_task')

class SubTask(Model):
    name = Field.String(size=40)
    date_created = Field.DateTime()
    parent_task = Field.Link()

class Laboratory(Model):
    name = Field.String(size=50)
    room_number = Field.Int()
    users = Field.VirtualLink('laboratory')


class UserBasicSearilizer(ViewSearilizer):
    fields = ('id', 'first_name', 'last_name', 'is_admin')

class LaboratorySearilizer(ViewSearilizer):
    fields = ('name', 'room_number', 'users')
    users = UserBasicSearilizer

class SubTaskSearilizer(ViewSearilizer):
    fields = ('id', 'name', 'date_created')

class TaskSearilizer(ViewSearilizer):
    fields = ('name', 'subtasks')
    subtasks = SubTaskSearilizer

class UserSearilizer(ViewSearilizer):
    fields = '*'
    tasks = TaskSearilizer
    laboratory = LaboratorySearilizer

mock_schema = Schema('mock_schema', models=[User, Laboratory, Task, SubTask])
mock_schema.create(delete_old=True)

laboratory1 = Laboratory(name="AI", room_number=81751).make()
laboratory2 = Laboratory(name="MAFS", room_number=8982).make()
laboratory3 = Laboratory(name="Sports", room_number=37393).make()
laboratory4 = Laboratory(name="Algorithms", room_number=5009).make()

user1 = User(first_name="John", last_name="Doe", email="john.doe@example.com", age=30, is_admin=True, created_at=datetime.now(), laboratory=laboratory1).make()
user2 = User(first_name="John", last_name="Smith", email="john.smith@example.com", age=25, is_admin=False, created_at=datetime.now(), laboratory=laboratory2).make()
user3 = User(first_name="Bob", last_name="Johnson", email="bob.johnson@example.com", age=28, is_admin=False, created_at=datetime.now(), laboratory=laboratory3).make()
user4 = User(first_name="Eve", last_name="Anderson", email="eve.anderson@example.com", age=15, is_admin=False, created_at=datetime.now(), laboratory=laboratory4).make()
user5 = User(first_name="Nobody", last_name="Anderson", email="nobodyanderson@gmail.com", age=40, is_admin=False, created_at=datetime.now(), laboratory=laboratory4).make()
user6 = User(first_name="Franc", last_name="Anderson", email="franc@example.com", age=44, is_admin=False, created_at=datetime.now(), laboratory=laboratory1).make()
user7 = User(first_name="Joze", last_name="", email="joze@gmail.com", age=49, is_admin=False, created_at=datetime.now(), laboratory=laboratory1).make()

for user in [user1, user2, user3, user4, user5]:
    for task_num in range(1, 6):
        task = Task(name=f"Task {task_num}", owner=user).make()
        for subtask_num in range(1, 5):
            subtask = SubTask(name=f"SubTask {subtask_num}", date_created=datetime.now(), parent_task=task).make()


count, data = User\
    .filter(User.age.not_in([15, 65, 44]), User.email.regex('.*@example.*'))\
    .order(User.first_name, User.last_name.desc())\
    .link(
        tasks = Task.offset(1).limit(3).link(subtasks = SubTask.filter(
                some(
                    SubTask.greater(10),
                    SubTask.name.is_in(['SubTask 4', 'SubTask 3'])
                )
            )
        ),
        laboratory = Laboratory.link(
            users = User.order(User.first_name.desc()).filter(
                some(
                    User.is_in(User.filter(User.is_admin.equals(True)).pick_id()),
                    User.last_name.equals('')
                )
            )
        )
    ).all()

print(json.dumps(UserSearilizer.jsonify(data), indent=5))