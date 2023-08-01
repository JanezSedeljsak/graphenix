from graphenix import Field, Schema, Model
from .data import user_data
import sys
import time

class User(Model):
    first_name = Field.String(size=50)
    last_name = Field.String(size=50)
    email = Field.String(size=50)
    age = Field.Int()
    is_admin = Field.Bool()
    created_at = Field.DateTime()

def main():
    num_users = int(sys.argv[1])
    my_schema = Schema(f'singleinsert_bulk_{num_users}', models=[User])
    if my_schema.exists():
        my_schema.delete()

    my_schema.create()

    start_time = time.perf_counter()

    user_data_list = []
    for i in range(num_users):
        is_admin = i % 2 == 0
        usr = [user_data['first_name'], user_data['last_name'], user_data['email'],
                      user_data['age'], is_admin, int(user_data['created_at'].timestamp())]
        user_data_list.append(usr)

    User.bulkcreate(user_data_list)

    end_time = time.perf_counter()
    elapsed_time = (end_time - start_time) * 1000
    print(f"{elapsed_time:.2f}")

if __name__ == '__main__':
    main()
