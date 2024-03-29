from graphenix import Field, Schema, Model
from .data import user_data, get_shuffled_points
import sys
import time

class User(Model):
    first_name = Field.String(size=15)
    last_name = Field.String(size=15)
    email = Field.String(size=25)
    points = Field.Int().as_index()
    is_admin = Field.Bool()
    created_at = Field.DateTime()

def main():
    num_users = int(sys.argv[1])
    my_schema = Schema(f'db_index_{num_users}', models=[User])
    if my_schema.exists():
        my_schema.delete()

    my_schema.create()

    start_time = time.perf_counter()

    points = get_shuffled_points(num_users)
    user_data_list = []
    for i in range(num_users):
        is_admin = i % 2 == 0
        usr = [user_data['first_name'] + str(i), user_data['last_name'], user_data['email'],
               points[i], is_admin, int(user_data['created_at'].timestamp())]
        user_data_list.append(usr)

    User.bulkcreate(user_data_list)

    end_time = time.perf_counter()
    elapsed_time = (end_time - start_time) * 1000
    print(f"{elapsed_time:.2f}")

if __name__ == '__main__':
    main()

