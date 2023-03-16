import unittest
import graphenix_engine
from graphenix import Field, Schema, Model
from random import randint

class User(Model):
    first_name = Field.String(size=15)
    last_name = Field.String(size=15)
    email = Field.String(size=50)
    age = Field.Int()

mock_schema = Schema('test_school', models=[
    User
])

class GraphenixTests(unittest.TestCase):

    def delete_if_exists_and_create(self):
        if mock_schema.exists():
            mock_schema.delete()

        mock_schema.create()
        exists = mock_schema.exists()
        self.assertTrue(exists)

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

        self.delete_if_exists_and_create()
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

        mock_schema.delete()
        exists = mock_schema.exists()
        self.assertFalse(exists)

    def test_record_create_and_update(self):
        """ Test if user gets created and updated correctly validate with db read """
        self.delete_if_exists_and_create()
        user0 = User(first_name="user1", last_name="last1", email="fl@gmail.com", age=30)
        user1 = User(first_name="user2", last_name="last2", email="fl2@gmail.com", age=32)

        user0.save()
        self.assertFalse(user0.is_new)

        self.assertTrue(user1.is_new)
        user1.save()
        self.assertFalse(user1.is_new)

        update_name = 'update'

        user1.first_name = update_name
        user1.save()
        self.assertFalse(user1.is_new)

        user1_db = User.get(1)
        self.assertEqual(user1.id, user1_db.id)
        self.assertEqual(update_name, user1_db.first_name)
        self.assertEqual(user1.first_name, user1_db.first_name)

        exists = mock_schema.exists()
        self.assertTrue(exists)
        mock_schema.delete()
        exists = mock_schema.exists()
        self.assertFalse(exists)


if __name__ == '__main__':
    unittest.main()