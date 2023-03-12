from graphenix import Schema, Model

class User(Model):
    name: str
    email: str

class Subject(Model):
    name: str

school = Schema('school', models=[
    User,
    Subject
])

if not school.exists():
    school.create()

user = User(name='neki', email='neki@gmail.com')
user.save()

user = User(name='janez', email='janez@gmail.com')
user.save()

user = User(name='test', email='test@gmail.com')
user.save()

print(User.get_by_id(0))
print(User.get_by_id(2))
print(User.get_by_id(1))

school.delete()