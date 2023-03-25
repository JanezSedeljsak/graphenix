import graphenix_engine2
import unittest
from random import randint
from datetime import datetime
from .tests_base import *
from .tests_data import *


class GraphenixUnitTests(CommonTestBase):
    @classmethod
    def _get_users(cls):
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
        return users

    def test_library_hearbeat(self):
        """ Test if heartbeat returns 12 (it's my birthdate so i return that as heartbeat) """
        heartbeat_response = graphenix_engine2.heartbeat() # type: ignore
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
        users = self._get_users()

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

    @CommonTestBase().prepare_and_destroy
    def test_insert_and_delete(self):
        user0 = User(first_name="user1", last_name="last1", email="fl@gmail.com", age=30)
        user0.save()

        user0.delete(lazy=True)
        with self.assertRaises(RuntimeError):
            User.get(0)

    @CommonTestBase().prepare_and_destroy
    def test_insert_and_delete_not_lazy(self):
        user0 = User(first_name="user1", last_name="last1", email="fl@gmail.com", age=30)
        user0.save()

        user0.delete(lazy=False)
        with self.assertRaises(RuntimeError):
            User.get(0)

    @CommonTestBase().prepare_and_destroy
    def test_insert_10_users_and_delete(self):
        users = self._get_users()

        for user in users:
            user.save()

        for i in range(len(users)):
            usr = User.get(i)
            usr.delete(lazy=False)
            with self.assertRaises(RuntimeError):
                User.get(i)

    @CommonTestBase().prepare_and_destroy
    def test_insert_10_users_and_delete_reverse(self):
        users = self._get_users()

        for user in users:
            user.save()

        for i in range(-len(users), -1, -1):
            usr = User.get(i)
            usr.delete(lazy=False)
            with self.assertRaises(RuntimeError):
                User.get(i)

if __name__ == '__main__':
    unittest.main()