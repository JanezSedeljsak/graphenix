from graphenix import Schema, Model, Field

class User(Model):
    name = Field.String(size=128)
    email = Field.String(size=255)

class Subject(Model):
    name = Field.String(size=128)

school = Schema('school', models=[
    User,
    Subject
])

if school.exists():
    school.delete()

school.create()

user0 = User(name='neki', email='neki@gmail.com')
user0.save()

user1 = User(name='janez', email='janez@gmail.com')
user1.save()

user2 = User(name='johndoe', email='johndoe@gmail.com')
user2.save()

z = User.get(1)
z.name = 'janez_update2'
z.save()

m = User.get(1)
m.name = 'keksi'
m.email = 'keksi@gmail.com'
m.save()

l = User.get(1)
l2 = User.get(2)
l0 = User.get(0)

print(l, m, z)

x = 5