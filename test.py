import unittest

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
        self.assertEqual(1+1, 2)


if __name__ == '__main__':
    unittest.main()

