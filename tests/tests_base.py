import time
import unittest
from .tests_data import *

class CommonTestBase(unittest.TestCase):

    @staticmethod
    def perf(name, times=1, logger=False, test_id=None):
        def decorator(method):
            def wrapper(*args, **kwargs):
                total_elapsed_time = 0
                for _ in range(times):
                    start_time = time.monotonic()
                    method(*args, **kwargs)
                    end_time = time.monotonic()
                    elapsed_time = end_time - start_time
                    total_elapsed_time += elapsed_time

                avg_elapsed_time = total_elapsed_time / times
                print(f"[Graphenix_PERF_TEST] Avg: {avg_elapsed_time:.3f} - {name} (executed {times} times)")
                return None
            return wrapper
        return decorator
    
    @staticmethod
    def ignore(method):
        """ A decorator that ignores the method it wraps (it never calls it) """
        def callable(*args, **kwargs):
            return None
        
        return callable
    
    def prepare_and_destroy(self, method):
        """ A decorator that calls prepares the db before the method call and deletes it after the call """
        def callable(*args, **kwargs):
            self.delete_if_exists_and_create()
            result = method(*args, **kwargs)
            self.cleanup_and_validate()
            return result
        
        return callable
    
    def prepare_1M_cities(self, method):
        """ A decorator that inserts 1M cities into the db """
        def callable(*args, **kwargs):
            AMOUNT = 1_000_000
            temp_record = City(name="Ljubljana", country="SLO", population_thousands=280)
            for i in range(AMOUNT):
                temp_record.save()
                temp_record.country = f'SLO{i}'
                temp_record._id = -1

            result = method(*args, **kwargs)
            return result
        
        return callable


    def delete_if_exists_and_create(self):
        if mock_schema.exists():
            mock_schema.delete()

        mock_schema.create()
        exists = mock_schema.exists()
        self.assertTrue(exists)

    def cleanup_and_validate(self):
        exists = mock_schema.exists()
        self.assertTrue(exists)
        mock_schema.delete()
        exists = mock_schema.exists()
        self.assertFalse(exists)