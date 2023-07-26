from graphenix import Field, Schema, Model, some, every, AGG
from datetime import datetime, timedelta

class Course(Model):
    name = Field.String(size=30)
    short_name = Field.String(size=5)
    teacher = Field.Link() # link to teacher
    difficulty = Field.Int()

class CourseStudent(Model):
    student = Field.Link()
    course = Field.Link()

class Student(Model):
    email = Field.String(size=50)
    start_date = Field.DateTime()
    student_id = Field.Int().as_index()
    height = Field.Double()
    courses = Field.VirtualLink("student")

class Teacher(Model):
    email = Field.String(size=50)
    lab = Field.String(size=30)
    courses = Field.VirtualLink("teacher")

user_schema = Schema('users', models=[Student, Teacher, Course])
user_schema.create(delete_old=True)

user1 = Student(email="john1doe@gmail.com", start_date=datetime.now() - timedelta(seconds=5), student_id=10, height=165.43).make()
user2 = Student(email="lorem@gmail.com", start_date=datetime.now(), student_id=12, height=175).make()
user2 = Student(email="kekec12@gmail.com", start_date=datetime.now(), student_id=17, height=175).make()
user3 = Student(email="jozica@gmail.com", start_date=datetime.now(), student_id=12, height=190).make()
user3 = Student(email="", start_date=datetime.now(), student_id=152, height=140).make()

count, data = Student.filter(
    Student.email == None
).all()

rows = list(data)
for row in rows:
    print(row)

print("-------------")
agg_rows = Student\
    .agg(
        by=Student.student_id,
        count=AGG.count(),
        max_amount=AGG.max("height"),
        min_amount=AGG.min("height"),
        total_amount=AGG.sum("height"),
        id_sum=AGG.sum("student_id"),
        latest=AGG.max("student_id"),
        latest_date=AGG.max("start_date")
    )

for agg in agg_rows:
    print(agg)

query = Student.filter(
    Student.email.not_in(["jozica@gmail.com"])
)

count, data = query.all()
first = query.first()
print("-------------")
print(first)

print("-------------")
for row in data:
    print(row)

print("-------------")
query = Student.filter(
    Student.email.regex("^[^0-9]*$")
)

count, res = query.all()
for row in res:
    print(row)