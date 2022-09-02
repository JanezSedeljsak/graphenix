import unittest
from src.schema_engine import Schema, Model, Field

my_schema = Schema("university_db", models={
    "students": Model({
        "fullname": Field.String(size=100),
        "email": Field.String(size=255).as_index(),
        "courses": Field.Link("courses")
    }),
    "courses": Model({
        "name": Field.String(size=100),
        "teacher": Field.Link("teachers", multi=False),
        "students": Field.Link("students")
    }),
    "teachers": Model({
        "fullname": Field.String(size=100),
        "email": Field.String(size=255).as_index(),
        "birthdate": Field.DateTime().as_index(),
        "courses": Field.Link("courses")
    }, lazy_delete=True),
})

"""
students
    - fullname string size=100
    - email string size=255
    - courses courses link multi=1
courses
    - name string size=100
    - teacher teachers link multi=0
    - students students link multi=1
teachers
    - fullname string size=100
    - email string size=255
    - birthdate datetime
    - courses courses link multi=1
"""

class TestPyBase(unittest.TestCase):
    
    def test_create_schema(self):
        """Test schema create"""
        self.assertEqual(my_schema.create(), True)


if __name__ == '__main__':
    unittest.main()