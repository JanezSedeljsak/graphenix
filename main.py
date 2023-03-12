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

user0 = User(name='neki', email='neki@gmail.com')
user0.save()

user1 = User(name='janez', email='janez@gmail.com')
user1.save()

user2 = User(name='test', email='test@gmail.com')
user2.save()

print(User.get_by_id(0))
print(User.get_by_id(1))
print(User.get_by_id(2))

print('after update')

user0.name = 'new name'
user0.email = 'update@gmail.com'
user0.save()

print(User.get_by_id(0))

school.delete()