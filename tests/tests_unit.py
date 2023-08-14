import graphenix_engine2 as ge2
import unittest
from datetime import datetime, timedelta
from .tests_base import *
from .tests_data import *
from graphenix import AGG, some, every, QueryView

class GraphenixUnitTests(CommonTestBase):
    @classmethod
    def _get_users(cls):
        users = [
            User(first_name="John", last_name="Doe", email="john.doe@example.com", age=23),
            User(first_name="Jane", last_name="Doe", email="jane.doe@example.com", age=45),
            User(first_name="Alice", last_name="Smith", email="alice.smith@example.com", age=32),
            User(first_name="Bob", last_name="Johnson", email="bob.johnson@example.com", age=18),
            User(first_name="Emily", last_name="Williams", email="emily.williams@example.com", age=50),
            User(first_name="Daniel", last_name="Brown", email="daniel.brown@example.com", age=60),
            User(first_name="Olivia", last_name="Jones", email="olivia.jones@example.com", age=73),
            User(first_name="David", last_name="Garcia", email="david.garcia@example.com", age=54),
            User(first_name="Isabella", last_name="Martinez", email="isabella.martinez@example.com", age=41),
            User(first_name="Jonhhny", last_name="Brave", email="jb@gmail.com", age=35),
        ]
        return users
    
    @classmethod
    def _make_subtasks(cls):
        SubTask(name="SubTask 1", date_created=datetime.now()).make()
        SubTask(name="SubTask 5", date_created=datetime.now()).make()
        SubTask(name="SubTask 3", date_created=datetime.now()).make()
        SubTask(name="SubTask 1", date_created=datetime.now()).make()

    def test_library_hearbeat(self):
        """ Test if heartbeat returns 12 (it's my birthdate so i return that as heartbeat) """
        heartbeat_response = ge2.heartbeat() # type: ignore
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

        user0.delete()
        with self.assertRaises(RuntimeError):
            User.get(0)

    @CommonTestBase().prepare_and_destroy
    def test_insert_10_users_and_delete(self):
        users = self._get_users()

        for user in users:
            user.save()

        for i in range(len(users)):
            usr = User.get(i)
            usr.delete()
            with self.assertRaises(RuntimeError):
                User.get(i)

    @CommonTestBase().prepare_and_destroy
    def test_insert_10_users_and_delete_reverse(self):
        users = self._get_users()

        for user in users:
            user.save()

        for i in range(-len(users), -1, -1):
            usr = User.get(i)
            usr.delete()
            with self.assertRaises(RuntimeError):
                User.get(i)

    @CommonTestBase().prepare_and_destroy
    def test_linking_a_capital_city_to_country(self):
        """ Tests linking functionality """
        user = User(first_name="user1", last_name="last1", email="fl@gmail.com", age=30).make()
        task_no_user : Task = Task(name="Learn Python").make()
        task_with_user = Task(name="Learn C++", owner=user).make()
        self.assertIsNone(task_no_user.owner)
        self.assertTrue(isinstance(task_with_user.owner, User))
        if isinstance(task_with_user.owner, User):
            self.assertEqual(user.first_name, task_with_user.owner.first_name)

        # task_with_user.owner = None
        # self.assertIsNone(task_with_user.owner)
        task_with_user.owner = User.get(0)
        if isinstance(task_with_user.owner, User):
            self.assertEqual(user.first_name, task_with_user.owner.first_name)

    @CommonTestBase().prepare_and_destroy
    def test_offset_without_limit(self):
        """ Tests offset without limit functionality """
        users = self._get_users()

        for user in users:
            user.save()
            self.assertFalse(user.is_new)

        count, rows = User.offset(0).all()
        self.assertEqual(len(users), count)
        for i, row in enumerate(rows):
            self.assertEqual(i, row.id)

        count, rows = User.offset(3).all()
        self.assertEqual(len(users) - 3, count)
        for i, row in enumerate(rows):
            self.assertEqual(i + 3, row.id)

        

        
    @CommonTestBase().prepare_and_destroy
    def test_offset_plus_limit(self):
        """ Tests offset + limit functionality """
        users = self._get_users()

        for user in users:
            user.save()
            self.assertFalse(user.is_new)

        count, _ = User.offset(0).all()
        self.assertEqual(len(users), count)

        count, rows = User.limit(2).offset(4).all()
        self.assertEqual(2, count)
        for i, row in enumerate(rows):
            self.assertEqual(i + 4, row.id)

        count, rows = User.limit(3).offset(0).all()
        self.assertEqual(3, count)
        for i, row in enumerate(rows):
            self.assertEqual(i, row.id)

        count, rows = User.limit(4).offset(5).all()
        self.assertEqual(4, count)
        for i, row in enumerate(rows):
            self.assertEqual(i + 5, row.id)


    @CommonTestBase().prepare_and_destroy
    def test_query_empty_table(self):
        """ test if querying empty table """
        _, data = SubTask.all()
        self.assertEqual(0, len(data))

        ids = SubTask.pick_id()
        self.assertEqual(0, len(ids))

        first = SubTask.first()
        self.assertIsNone(first)

    @CommonTestBase().prepare_and_destroy
    def test_limit_empty_table(self):
        """ test if limit set on empty table """

        _, data = SubTask.limit(10).all()
        self.assertEqual(0, len(data))

    @CommonTestBase().prepare_and_destroy
    def test_offset_empty_table(self):
        """ test if offset set on empty table """

        _, data = SubTask.offset(10).all()
        self.assertEqual(0, len(data))

    @CommonTestBase().prepare_and_destroy
    def test_offset_limit_empty_table(self):
        """ test if offset + limit set on empty table """

        _, data = SubTask.offset(100).limit(500).all()
        self.assertEqual(0, len(data))

    @CommonTestBase().prepare_and_destroy
    def test_offset_pick_empty_table(self):
        """ test pick on empty table """

        picked = SubTask.pick(SubTask.name)
        self.assertEqual(0, len(picked))

        picked = SubTask.pick_id()
        self.assertEqual(0, len(picked))

    @CommonTestBase().prepare_and_destroy
    def test_order_by_1(self):
        """ test if simple order by works - desc PK """
        self._make_subtasks()
        _, data = SubTask.order(SubTask.desc()).all()
        self.assertEqual(4, len(data))

        self.assertEqual(3, data[0].id)
        self.assertEqual(2, data[1].id)
        self.assertEqual(1, data[2].id)
        self.assertEqual(0, data[3].id)


    @CommonTestBase().prepare_and_destroy
    def test_order_by_2(self):
        """ test if simple order by works - asc PK """
        self._make_subtasks()
        _, data = SubTask.order(SubTask).all()
        self.assertEqual(4, len(data))

        self.assertEqual(0, data[0].id)
        self.assertEqual(1, data[1].id)
        self.assertEqual(2, data[2].id)
        self.assertEqual(3, data[3].id)

    @CommonTestBase().prepare_and_destroy
    def test_order_by_3(self):
        """ test if simple order by works - name.desc, PK """
        self._make_subtasks()
        _, data = SubTask.order(SubTask.name.desc(), SubTask).all()
        self.assertEqual(4, len(data))

        self.assertEqual(1, data[0].id)
        self.assertEqual(2, data[1].id)
        self.assertEqual(0, data[2].id)
        self.assertEqual(3, data[3].id)

    
    @CommonTestBase().prepare_and_destroy
    def test_order_by_4(self):
        """ test if simple order by works - name, PK """
        self._make_subtasks()
        _, data = SubTask.order(SubTask.name, SubTask).all()
        self.assertEqual(4, len(data))

        self.assertEqual(0, data[0].id)
        self.assertEqual(3, data[1].id)
        self.assertEqual(2, data[2].id)
        self.assertEqual(1, data[3].id)

    @CommonTestBase().prepare_and_destroy
    def test_filter_1(self):
        """ test simple filter """
        self._make_subtasks()
        _, data = SubTask.filter(SubTask.name.equals("SubTask 1")).all()
        self.assertEqual(2, len(data))

        _, data = SubTask.filter(SubTask.name.equals("SubTask 3")).all()
        self.assertEqual(1, len(data))

    @CommonTestBase().prepare_and_destroy
    def test_filter_2(self):
        """ test simple regex filter """
        self._make_subtasks()
        _, data = SubTask.filter(SubTask.name.regex(".*SubTask.*")).all()
        self.assertEqual(4, len(data))

    @CommonTestBase().prepare_and_destroy
    def test_filter_3(self):
        """ test simple regex filter """
        self._make_subtasks()
        _, data = SubTask.filter(SubTask.name.regex(".*XD.*")).all()
        self.assertEqual(0, len(data))
        
    @CommonTestBase().prepare_and_destroy
    def test_filter_4(self):
        """ test simple is_in + regex filter """
        self._make_subtasks()
        ids = SubTask.filter(SubTask.name.regex(".*1.*")).pick_id()
        _, data = SubTask.filter(SubTask.is_in(ids)).all()
        self.assertEqual(2, len(data))

    @CommonTestBase().prepare_and_destroy
    def test_filter_5(self):
        """ test simple not_in + regex filter """
        self._make_subtasks()
        ids = SubTask.filter(SubTask.name.regex(".*1.*")).pick_id()
        _, data = SubTask.filter(SubTask.not_in(ids)).all()
        self.assertEqual(2, len(data))

    @CommonTestBase().prepare_and_destroy
    def test_filter_6(self):
        """ test simple not_in + regex filter """
        self._make_subtasks()
        ids = SubTask.filter(SubTask.name.regex(".*1.*")).pick_id()
        _, data = SubTask.filter(SubTask.not_in(ids)).all()
        self.assertEqual(2, len(data))

    @CommonTestBase().prepare_and_destroy
    def test_filter_7(self):
        """ test simple multiple in filter """
        self._make_subtasks()
        _, data = SubTask.filter(
            SubTask.name.regex(".*1.*"),
            SubTask.equals(0),
        ).all()

        # one has ID = 0 + a "1" in name
        self.assertEqual(1, len(data))

    @CommonTestBase().prepare_and_destroy
    def test_filter_8(self):
        """ test simple some in filter """
        self._make_subtasks()
        _, data = SubTask.filter(
            some(
                SubTask.equals(1),
                SubTask.equals(0),           
            )
        ).all()

        self.assertEqual(2, len(data))

    @CommonTestBase().prepare_and_destroy
    def test_filter_9(self):
        """ test simple every in filter """
        self._make_subtasks()
        _, data = SubTask.filter(
            every(
                SubTask.equals(1),
                SubTask.equals(0),           
            )
        ).all()

        self.assertEqual(0, len(data))

    @CommonTestBase().prepare_and_destroy
    def test_filter_10(self):
        """ test simple every in filter 2 """
        self._make_subtasks()
        _, data = SubTask.filter(
            every(
                SubTask.name.is_not("SubTask 3"),
                SubTask.equals(0),           
            )
        ).all()

        self.assertEqual(1, len(data))

    @CommonTestBase().prepare_and_destroy
    def test_filter_11(self):
        """ test simple some + every in filter """
        self._make_subtasks()
        _, data = SubTask.filter(
            every(
                SubTask.name.greater_or_equal("SubTask 2"),
                SubTask.name.regex(".*SubTask.*")
            ),
            some(
                SubTask.name.regex(".*1.*"),
                SubTask.date_created.greater(datetime.now() - timedelta(days=1)),           
            )
        ).all()

        self.assertEqual(2, len(data))

    @CommonTestBase().prepare_and_destroy
    def test_filter_12(self):
        """ test between filter for string """
        self._make_subtasks()
        bwtn = SubTask.name.between("SubTask 3", "SubTask 5")
        _, data = SubTask\
                .order(SubTask.name)\
                .filter(bwtn)\
                .all()

        self.assertEqual(2, len(data))
        self.assertEqual('SubTask 3', data[0].name)
        self.assertEqual('SubTask 5', data[1].name)

    @CommonTestBase().prepare_and_destroy
    def test_filter_13(self):
        """ test between filter for string - get all """
        self._make_subtasks()
        bwtn = SubTask.name.between("SubTask 1", "SubTask 9")
        _, data = SubTask.filter(bwtn).all()
        self.assertEqual(4, len(data))

    @CommonTestBase().prepare_and_destroy
    def test_filter_14(self):
        """ test between filter for string - get 3 """
        self._make_subtasks()
        bwtn = SubTask.name.between("SubTask 1", "SubTask 3")
        _, data = SubTask.filter(bwtn).all()
        self.assertEqual(3, len(data))

    @CommonTestBase().prepare_and_destroy
    def test_filter_15(self):
        """ test between filter for string - get 1 """
        self._make_subtasks()
        bwtn = SubTask.name.between("SubTask 2", "SubTask 3")
        _, data = SubTask.filter(bwtn).all()
        self.assertEqual(1, len(data))

    @CommonTestBase().prepare_and_destroy
    def test_filter_16(self):
        """ test between filter for int - get 1 """
        for user in self._get_users():
            user.save()

        _, data = User.filter(User.age.between(19, 30)).all()
        self.assertEqual(1, len(data))
        self.assertEqual(23, data[0].age)

    @CommonTestBase().prepare_and_destroy
    def test_filter_17(self):
        """ test between filter for int - get all """
        for user in self._get_users():
            user.save()

        _, data = User.filter(User.age.between(18, 90)).all()
        self.assertEqual(10, len(data))

    @CommonTestBase().prepare_and_destroy
    def test_filter_18(self):
        """ test between filter for int - get some """
        for user in self._get_users():
            user.save()

        _, data = User.filter(User.age.between(23, 47)).all()
        self.assertEqual(5, len(data))
        for usr in data:
            self.assertTrue(23 <= usr.age and usr.age <= 47)

    @CommonTestBase().prepare_and_destroy
    def test_filter_19(self):
        """ test instant pk filter with PK condition """
        for user in self._get_users():
            user.save()

        _, data = User.filter(User.equals(3)).all()
        self.assertEqual(1, len(data))
        self.assertEqual(3, data[0].id)

    @CommonTestBase().prepare_and_destroy
    def test_filter_20(self):
        """ test instant pk filter with PK condition """
        for user in self._get_users():
            user.save()

        _, data = User.filter(User.is_in([3,2])).order(User.desc()).all()
        self.assertEqual(2, len(data))
        self.assertEqual(3, data[0].id)
        self.assertEqual(2, data[1].id)


    @CommonTestBase().prepare_and_destroy
    def test_filter_21(self):
        """ test instant pk filter with PK condition 2 """
        for user in self._get_users():
            user.save()

        _, data = User.filter(User.is_in([3,2]), User.less(10)).order(User.desc()).all()
        self.assertEqual(2, len(data))
        self.assertEqual(3, data[0].id)
        self.assertEqual(2, data[1].id)

    @CommonTestBase().prepare_and_destroy
    def test_filter_22(self):
        """ test filter with model condition """
        users = self._get_users()
        for user in users:
            user.save()
        
        Task(name="Travel to Italy", owner=users[0]).make()
        _, tasks = Task.all()
        self.assertEqual(1, len(tasks))

        _, tasks = Task.filter(Task.owner.equals(users[0])).all()
        self.assertEqual(1, len(tasks))

        _, tasks = Task.filter(Task.owner.equals(users[1])).all()
        self.assertEqual(0, len(tasks))

        _, tasks = Task.filter(Task.owner.is_not(users[0])).all()
        self.assertEqual(0, len(tasks))   


    @CommonTestBase().prepare_and_destroy
    @CommonTestBase().prepare_comlex_struct
    def test_join_0(self):
        """ test join 2 tables """
        _, data = Task.link(subtasks = SubTask).all()
        self.assertEqual(133, len(data))
        self.assertEqual(data[0].name, 'Task 1')

        _, data = Task.filter(Task.name.not_in(["Task 1", "Task"])).link(subtasks = SubTask).all()
        self.assertIsInstance(data, QueryView)
        self.assertEqual(126, len(data))
        self.assertEqual(data[0].name, 'Task 2')
        self.assertIsInstance(data[0].subtasks, ge2.View)
        self.assertEqual("SubTask 1", data[0].subtasks[0].name)

    
    @CommonTestBase().prepare_and_destroy
    def test_bulk_create(self):
        """ test bulk create function """
        rows = [
            ['SubTask 1', int(datetime.now().timestamp()), -1],
            ['SubTask 2', int(datetime.now().timestamp()), -1],
            ['SubTask 3', int(datetime.now().timestamp()), -1],
            ['SubTask 4', int(datetime.now().timestamp()), -1],
            ['SubTask 5', int(datetime.now().timestamp()), -1],
        ]

        SubTask.bulkcreate(rows)
        _, data = SubTask.all()
        self.assertEqual(len(rows), len(data))
        self.assertEqual(rows[0][0], data[0].name)
        self.assertEqual(-1, data[0].parent_task)
        subtask_instance = SubTask.from_view(data[0])
        self.assertEqual(rows[0][0], subtask_instance.name)
        self.assertIsNone(subtask_instance.parent_task)


    @CommonTestBase().prepare_and_destroy
    def test_bulk_delete(self):
        """ Tests bulk delete """
        self._make_subtasks()
        _, data = SubTask.all()
        self.assertEqual(4, len(data))

        ids = SubTask.pick_id()
        SubTask.bulkdelete(ids)
        _, data = SubTask.all()
        self.assertEqual(0, len(data))


    @CommonTestBase().prepare_and_destroy
    def test_bulk_delete_with_filter(self):
        """ Tests bulk delete with filter """
        self._make_subtasks()
        _, data = SubTask.all()
        self.assertEqual(4, len(data))

        ids = SubTask.filter(SubTask.name.regex(".*1.*")).pick_id()
        SubTask.bulkdelete(ids)
        _, data = SubTask.order(SubTask.name.desc()).all()
        self.assertEqual(2, len(data))
        names = [row.name for row in data]
        self.assertEqual(names[0], 'SubTask 5')
        self.assertEqual(names[1], 'SubTask 3')

if __name__ == '__main__':
    unittest.main()