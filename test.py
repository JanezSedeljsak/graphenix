import unittest
import pybase_core as PYB_Core
from core.pyb import Schema, Model, Field

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

    def test_heartbeat(self):
        """Heartbeat function should return 12"""
        self.assertEqual(PYB_Core.heartbeat(), 12)
    
    def test_create_schema(self):
        """Test schema create"""
        self.assertEqual(my_schema.create(), True)

    def test_migrate_schema(self):
        """Test schema migration"""
        self.assertEqual(my_schema.migrate(), True)
    
    def test_delete_schema(self):
        """Test delete schema"""
        self.assertEqual(my_schema.delete(), True)

    def test_print_schema(self):
        """Test print schema"""
        my_schema.print()

    def test_load_schema(self):
        """Test load schema"""
        loaded, _ = Schema.load('test')
        self.assertEqual(loaded, True)


if __name__ == '__main__':
    unittest.main()

