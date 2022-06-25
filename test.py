import unittest
import pybase as pyb_core
import core.pyb as pyb

my_schema = pyb.Schema("university_db", models=[
    pyb.Model("students", slots=[
        pyb.Model("student_id", pyb.Type.STR_FIXED, size=255),
        pyb.Model("fullname", pyb.Type.STR_FIXED, size=255),
        pyb.Model("email", pyb.Type.STR_FIXED, size=255, indexed=True),
        pyb.Link("courses")
    ]),
    pyb.Model("courses", slots=[
        pyb.Model("name", pyb.Type.STR_FIXED, size=255),
        pyb.Link("students"),
        pyb.Link("teachers")
    ]),
    pyb.Model("teachers", slots=[
        pyb.Model("fullname", pyb.Type.STR_FIXED, size=255),
        pyb.Model("email", pyb.Type.STR_FIXED, size=255, indexed=True),
        pyb.Link("courses")
    ])
])

x = 5

class TestPyBase(unittest.TestCase):

    def test_heartbeat(self):
        """Heartbeat function should return 12"""
        self.assertEqual(pyb_core.heartbeat(), 12)
    
    def test_create_base(self):
        """Test creating a database"""
        self.assertEqual(pyb_core.create_base(), True)
    
    def test_delete_base(self):
        """Test delete database"""
        self.assertEqual(pyb_core.delete_base(), True)

if __name__ == '__main__':
    unittest.main()

