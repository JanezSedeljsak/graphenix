import time
import unittest
from .tests_data import *
from datetime import datetime

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
    
    def prepare_comlex_struct(self, method):
        """ A decorator that prepares a more complex structure to test a more complex (multilevel) query """
        def callable(*args, **kwargs):
            city1 = City(name="London", population_thousands=432).make()
            city2 = City(name="Ljubljana", population_thousands=100).make()
            city3 = City(name="New York", population_thousands=500).make()
            city4 = City(name="Tokyo", population_thousands=420).make()

            user1 = User(first_name="John", last_name="Doe", email="john.doe@example.com", age=30, is_admin=True, created_at=datetime.now(), city=city1).make()
            user2 = User(first_name="John", last_name="Smith", email="john.smith@example.com", age=25, is_admin=False, created_at=datetime.now(), city=city2).make()
            user3 = User(first_name="Bob", last_name="Johnson", email="bob.johnson@example.com", age=28, is_admin=False, created_at=datetime.now(), city=city3).make()
            user4 = User(first_name="Eve", last_name="Anderson", email="eve.anderson@example.com", age=15, is_admin=False, created_at=datetime.now(), city=city4).make()
            user5 = User(first_name="Nobody", last_name="Anderson", email="nobodyanderson@gmail.com", age=40, is_admin=False, created_at=datetime.now(), city=city4).make()
            user6 = User(first_name="Franc", last_name="Anderson", email="franc@example.com", age=44, is_admin=False, created_at=datetime.now(), city=city1).make()
            user7 = User(first_name="Jože", last_name="", email="joze@gmail.com", age=49, is_admin=False, created_at=datetime.now(), city=city1).make()

            for user in (user1, user2, user3, user4, user5, user6, user7):
                for task_num in range(1, 20):
                    task = Task(name=f"Task {task_num}", owner=user).make()
                    for subtask_num in range(1, 20):
                        SubTask(name=f"SubTask {subtask_num}", date_created=datetime.now(), parent_task=task).make()

            result = method(*args, **kwargs)
            return result
        
        return callable 
    
    def prepare_1M_cities(self, method):
        """ A decorator that inserts 1M cities into the db """
        def callable(*args, **kwargs):
            AMOUNT = 1_000_000
            temp_record = City(name="Ljubljana", country="SLO", population_thousands=280)
            for i in range(AMOUNT):
                temp_record.country = f'SLO{i}'
                temp_record._id = -1
                temp_record.save()

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