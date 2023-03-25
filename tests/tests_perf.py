import unittest
from datetime import datetime, timedelta
from .tests_base import *
from .tests_data import *


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