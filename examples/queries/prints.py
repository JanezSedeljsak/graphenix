from graphenix import Field, Schema, Model, ViewSearilizer, some, every, AGG
from datetime import datetime
import graphenix_engine2 as ge2

class User(Model):
    name = Field.String(size=100)
    lab = Field.Link()

class Laboratory(Model):
    name = Field.String(size=50)
    room_number = Field.Double().as_index()
    users = Field.VirtualLink('lab')

class UserSearilizer(ViewSearilizer):
    fields = ('name',)

class LabSearilizer(ViewSearilizer):
    fields = '*'
    users = UserSearilizer

mock_schema = Schema('queries_db', models=[Laboratory, User])
mock_schema.create(delete_old=True)

for i in range(120):
    lab = Laboratory(name=f"Lab {i}", room_number=i + 15.7).make()
    for u in range(10):
        User(name=f'John {u+i}', lab=lab).make()

labs = Laboratory.pick(Laboratory.name)

labs2 = Laboratory.order(Laboratory.name.desc()).pick(Laboratory.name)

labs3 = Laboratory.order(Laboratory.room_number.desc()).pick(Laboratory.name)

_, view1 = Laboratory.all()

_, view2 = Laboratory.filter(Laboratory.name.iregex(f'.*{1}.*')).all()

_, view3 = Laboratory.filter(Laboratory.room_number.equals(94.7)).all()

_, view4 = Laboratory.filter(Laboratory.room_number.greater(70), Laboratory.name.iregex(f'.*{1}.*')).all()

_, view5 = Laboratory.filter(Laboratory.room_number.greater(70), Laboratory.name.iregex(f'.*{1}.*')).offset(10).limit(3).all()

_, view6 = Laboratory.link(users=User.limit(2)).limit(3).all()
# print(LabSearilizer.jsonify(view6))

agg1 = User.agg(by=User.lab, count=AGG.count())

x = 5
