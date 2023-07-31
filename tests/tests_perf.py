import unittest
from datetime import datetime, timedelta
from graphenix import some
from .tests_base import *
from .tests_data import *


class GraphenixPerfTests(CommonTestBase):
    
    @CommonTestBase.ignore
    @CommonTestBase.perf("Create 10K users and read them by IDs", times=5)
    @CommonTestBase().prepare_and_destroy
    def test_create_10k_users_and_read(self):
        """ Create 10K users and read them by IDs """
        AMOUNT = 10_000

        amount_half = AMOUNT // 2
        dt = datetime.now()
        tmp_user = User(first_name="John", last_name="Doe", email="john.doe@example.com", age=25, created_at=dt)
        for uid in range(AMOUNT):
            tmp_user.is_admin = uid < amount_half
            tmp_user.created_at = dt + timedelta(days=uid)

            tmp_user.save()
            tmp_user._id = -1

        for uid in range(AMOUNT):
            read_user = User.get(uid)
            self.assertEqual(tmp_user.first_name, read_user.first_name)
            self.assertEqual(tmp_user.age, read_user.age)
            self.assertEqual(uid, read_user.id)
            self.assertFalse(read_user.is_new)
            self.assertEqual(uid<amount_half, read_user.is_admin)
            temp_dt = dt + timedelta(days=uid)
            self.assertEqual(temp_dt.day, read_user.created_at.day)

    @CommonTestBase.ignore
    @CommonTestBase.perf("Create 100K basic codelist records and read them by IDs", times=5)
    @CommonTestBase().prepare_and_destroy
    def test_create_100k_records_and_read(self):
        AMOUNT = 100_000

        temp_record = City(name="Ljubljana", country="SLO", population_thousands=280)
        for _ in range(AMOUNT):
            temp_record.save()
            temp_record._id = -1

        for rid in range(AMOUNT):
            city = City.get(rid)
            self.assertEqual(rid, city.id)
            self.assertEqual(temp_record.name, city.name)
            self.assertEqual(temp_record.population_thousands, city.population_thousands)

    @CommonTestBase.ignore
    @CommonTestBase().prepare_and_destroy
    @CommonTestBase().prepare_1M_cities
    @CommonTestBase.perf("Read 1M records at once", times=5)
    def test_read_1M_records(self):
        AMOUNT = 1_000_000
        count, rows = City.all()
        self.assertEqual(AMOUNT, count)
        first = rows[0]
        self.assertIsInstance(City.from_view(first), City)
        self.assertEqual(0, first.id)
        self.assertEqual('SLO0', first.country)

    @CommonTestBase.ignore
    @CommonTestBase().prepare_and_destroy
    @CommonTestBase().prepare_1M_cities
    @CommonTestBase.perf("Read with limit 100 from Model with 1M records", times=5)
    def test_read_with_limit_1M_records_table(self):
        LIMIT = 100
        count, rows = City.limit(LIMIT).all()
        self.assertEqual(LIMIT, count)
        first = rows[0]
        self.assertIsInstance(City.from_view(first), City)
        self.assertEqual(0, first.id)
        self.assertEqual('SLO0', first.country)

    @CommonTestBase.ignore
    @CommonTestBase().prepare_and_destroy
    @CommonTestBase().prepare_1M_cities
    @CommonTestBase.perf("Read with order by country from Model with 1M records", times=5)
    def test_read_with_order_by_1M_records_table(self):
        AMOUNT = 1_000_000
        count, rows = City.order(City.country.desc()).all()
        self.assertEqual(AMOUNT, count)
        first = rows[0]
        last_id = AMOUNT - 1
        self.assertIsInstance(City.from_view(first), City)
        self.assertEqual(last_id, first.id)
        self.assertEqual(f'SLO{last_id}', first.country)

    @CommonTestBase.ignore
    @CommonTestBase().prepare_and_destroy
    @CommonTestBase().prepare_1M_cities
    @CommonTestBase.perf("Read with limit 100 + order by country from Model with 1M records", times=5)
    def test_read_with_limit_order_by_1M_records_table(self):
        AMOUNT = 1_000_000
        LIMIT = 100
        count, rows = City.order(City.country.desc()).limit(LIMIT).all()
        self.assertEqual(LIMIT, count)
        first = rows[0]
        last_id = AMOUNT - 1
        self.assertIsInstance(City.from_view(first), City)
        self.assertEqual(last_id, first.id)
        self.assertEqual(f'SLO{last_id}', first.country)

    @CommonTestBase.ignore
    @CommonTestBase().prepare_and_destroy
    @CommonTestBase().prepare_1M_cities
    @CommonTestBase.perf("Read with limit 1500 + order by country from Model with 1M records", times=5)
    def test_read_with_limit1500_order_by_1M_records_table(self):
        AMOUNT = 1_000_000
        LIMIT = 1500
        count, rows = City.order(City.country.desc()).limit(LIMIT).all()
        self.assertEqual(LIMIT, count)
        first = rows[0]
        last_id = AMOUNT - 1
        self.assertIsInstance(City.from_view(first), City)
        self.assertEqual(last_id, first.id)
        self.assertEqual(f'SLO{last_id}', first.country)

    @CommonTestBase().prepare_and_destroy
    @CommonTestBase().prepare_comlex_struct
    @CommonTestBase.perf("Read multilevel query and searilize data", times=5)
    def test_read__with_multilevel_query(self):
        count, data = User\
            .filter(User.age.not_in([15, 65, 44]), User.email.regex('.*@example.*'))\
            .order(User.first_name, User.last_name.desc())\
            .link(
                tasks = Task.offset(1).limit(3).link(subtasks = SubTask.filter(
                        some(
                            SubTask.greater(10),
                            SubTask.name.not_in(['SubTask 4', 'SubTask 3'])
                        )
                    )
                ),
                city = City.link(
                    users = User.order(User.desc()).filter(
                        some(
                            User.is_in(User.filter(User.is_admin.equals(True)).pick_id()),
                            User.last_name.equals('')
                        )
                    )
                )
            ).all()
        
        self.assertEqual(3, count)
        first = User.from_view(data[0])
        self.assertEqual(2, first.id)
        searilized = DeepUserSearilizer.jsonify(data)
        last_user_city = searilized[-1]['city']
        self.assertEqual('London', last_user_city['name'])
        self.assertEqual(2, len(last_user_city['users']))
        self.assertEqual(6, last_user_city['users'][0]['id'])
        self.assertEqual(0, last_user_city['users'][1]['id'])
        



if __name__ == '__main__':
    unittest.main()