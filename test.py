import unittest
import pybase as pyb

class TestPyBase(unittest.TestCase):

    def test_heartbeat(self):
        """Heartbeat function should return 12"""
        self.assertEqual(pyb.heartbeat(), 12)
    
    def test_create_base(self):
        """Test creating a database"""
        self.assertEqual(pyb.create_base(), True)
    
    def test_delete_base(self):
        """Test delete database"""
        self.assertEqual(pyb.delete_base(), True)

if __name__ == '__main__':
    unittest.main()

