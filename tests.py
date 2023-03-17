import time
import unittest
import graphenix_engine
from graphenix import Field, Schema, Model
from random import randint
from datetime import datetime, timedelta

class User(Model):
    first_name = Field.String(size=15)
    last_name = Field.String(size=15)
    email = Field.String(size=50)
    age = Field.Int()
    is_admin = Field.Bool()
    created_at = Field.DateTime()

class City(Model):
    name = Field.String(size=50)
    country = Field.String(size=50)
    population_thousands = Field.Int()

mock_schema = Schema('test_school',
                      models=[User, City])

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

class GraphenixUnitTests(CommonTestBase):

    def test_library_hearbeat(self):
        """ Test if heartbeat returns 12 (it's my birthdate so i return that as heartbeat) """
        heartbeat_response = graphenix_engine.heartbeat()
        self.assertEqual(12, heartbeat_response)

    def test_create_and_delete_schema(self):
        """ Test if schema get's deleted and created correctly based on exists method """
        self.delete_if_exists_and_create()

        mock_schema.delete()
        exists = mock_schema.exists()
        self.assertFalse(exists)

    @CommonTestBase().prepare_and_destroy
    def test_create_with_delete_option(self):
        exists = mock_schema.exists()
        self.assertTrue(exists)

        mock_schema.create(delete_old=True)
        exists = mock_schema.exists()
        self.assertTrue(exists)

    @CommonTestBase().prepare_and_destroy
    def test_create_schema_and_10_records(self):
        """ Test creating 10 users and then read the records by id and verify each exists """
        users = [
            User(first_name="John", last_name="Doe", email="john.doe@example.com", age=randint(18, 60)),
            User(first_name="Jane", last_name="Doe", email="jane.doe@example.com", age=randint(18, 60)),
            User(first_name="Alice", last_name="Smith", email="alice.smith@example.com", age=randint(18, 60)),
            User(first_name="Bob", last_name="Johnson", email="bob.johnson@example.com", age=randint(18, 60)),
            User(first_name="Emily", last_name="Williams", email="emily.williams@example.com", age=randint(18, 60)),
            User(first_name="Daniel", last_name="Brown", email="daniel.brown@example.com", age=randint(18, 60)),
            User(first_name="Olivia", last_name="Jones", email="olivia.jones@example.com", age=randint(18, 60)),
            User(first_name="David", last_name="Garcia", email="david.garcia@example.com", age=randint(18, 60)),
            User(first_name="Isabella", last_name="Martinez", email="isabella.martinez@example.com", age=randint(18, 60)),
            User(first_name="Jonhhny", last_name="Brave", email="jb@gmail.com", age=randint(8, 60)),
        ]

        for user in users:
            user.save()
            # once saved users have to have is_new flag set to false
            self.assertFalse(user.is_new) 

        # ids are auto set and are indexed from [0 - (n-1)]
        user_ids = list(range(len(users))) 

        for user_id in user_ids:
            temp_user = User.get(user_id)
            self.assertEqual(users[user_id].id, temp_user.id)
            self.assertEqual(users[user_id].first_name, temp_user.first_name)
            self.assertEqual(users[user_id].last_name, temp_user.last_name)
            self.assertEqual(users[user_id].email, temp_user.email)
            self.assertEqual(users[user_id].age, temp_user.age)
            self.assertFalse(users[user_id].is_admin)

    @CommonTestBase().prepare_and_destroy
    def test_record_create_general_update(self):
        """ Test if user gets created and updated correctly validate with db read """
        user0 = User(first_name="user1", last_name="last1", email="fl@gmail.com", age=30)
        user1 = User(first_name="user2", last_name="last2", email="fl2@gmail.com", age=32)

        user0.save()
        self.assertFalse(user0.is_new)

        self.assertTrue(user1.is_new)
        user1.save()
        self.assertFalse(user1.is_new)
        self.assertFalse(user1.is_admin)

        update_name = 'update'

        user1.first_name = update_name
        user1.is_admin = True
        user1.save()
        self.assertFalse(user1.is_new)

        user1_db = User.get(1)
        self.assertEqual(user1.id, user1_db.id)
        self.assertEqual(update_name, user1_db.first_name)
        self.assertEqual(user1.first_name, user1_db.first_name)
        self.assertEqual(user1.is_admin, user1_db.is_admin)
        self.assertTrue(user1_db.is_admin)

    @CommonTestBase().prepare_and_destroy
    def test_record_create_datetime_update(self):
        user0 = User(first_name="user1", last_name="last1", email="fl@gmail.com", age=30)
        user0.save()
        dt = datetime.now()
        user0.created_at = dt
        user0.save()

        usr = User.get(0)
        self.assertEqual(dt.year, usr.created_at.year)
        self.assertEqual(dt.month, usr.created_at.month)
        self.assertEqual(dt.day, usr.created_at.day)
        self.assertEqual(dt.hour, usr.created_at.hour)
        self.assertEqual(dt.minute, usr.created_at.minute)


class GraphenixPerfTests(CommonTestBase):

    # @CommonTestBase.ignore
    @CommonTestBase.perf("Create 10K users and read them by IDs", times=3)
    @CommonTestBase().prepare_and_destroy
    def test_create_10k_users_and_read(self):
        """ Create 10K users and read them by IDs """
        AMOUNT = 10_000

        amount_half = AMOUNT // 2
        tmp_user = User(first_name="John", last_name="Doe", email="john.doe@example.com", age=25)
        dt = datetime.now()
        for uid in range(AMOUNT):
            new_user = User(
                first_name=tmp_user.first_name,
                last_name=tmp_user.last_name,
                email=tmp_user.email,
                age=tmp_user.age,
                is_admin=uid<amount_half,
                created_at=dt+timedelta(days=uid)
            )

            new_user.save()

        for uid in range(AMOUNT):
            read_user = User.get(uid)
            self.assertEqual(tmp_user.first_name, read_user.first_name)
            self.assertEqual(tmp_user.age, read_user.age)
            self.assertEqual(uid, read_user.id)
            self.assertFalse(read_user.is_new)
            self.assertEqual(uid<amount_half, read_user.is_admin)
            temp_dt = dt+timedelta(days=uid)
            self.assertEqual(temp_dt.day, read_user.created_at.day)

    # @CommonTestBase.ignore
    @CommonTestBase.perf("Create 100K basic codelist records and read them by IDs", times=3)
    @CommonTestBase().prepare_and_destroy
    def test_create_100k_records_and_read(self):
        AMOUNT = 100_000

        temp_record = City(name="Ljubljana", country="SLO", population_thousands=280)
        for _ in range(AMOUNT):
            new_rec = City(
                name=temp_record.name,
                country=temp_record.country,
                population_thousands=temp_record.population_thousands
            )

            new_rec.save()

        for rid in range(AMOUNT):
            city = City.get(rid)
            self.assertEqual(rid, city.id)
            self.assertEqual(temp_record.name, city.name)
            self.assertEqual(temp_record.population_thousands, city.population_thousands)




if __name__ == '__main__':
    unittest.main()