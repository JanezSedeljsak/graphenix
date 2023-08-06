from graphenix import Field, Schema, Model
from .data import user_data, task_data
import sys
import time
import random
random.seed(12)

class User(Model):
    first_name = Field.String(size=15)
    last_name = Field.String(size=15)
    email = Field.String(size=25)
    points = Field.Int()
    is_admin = Field.Bool()
    created_at = Field.DateTime()
    tasks = Field.VirtualLink("user")

class Task(Model):
    title = Field.String(size=15)
    created_at = Field.DateTime()
    is_completed = Field.Bool()
    user = Field.Link()

def main():
    num_users = int(sys.argv[1])
    my_schema = Schema(f'join_db_{num_users}', models=[User, Task])
    if my_schema.exists():
        my_schema.delete()

    my_schema.create()

    start_time = time.perf_counter()

    user_data_list = []
    task_data_list = []

    for i in range(num_users):
        is_admin = i % 2 == 0
        usr = [user_data['first_name'] + str(i), user_data['last_name'], user_data['email'],
               random.randint(10, 8000), is_admin, int(user_data['created_at'].timestamp())]
        user_data_list.append(usr)

        for j in range(10):
            task_data_list.append([task_data['title'] + str(j), int(task_data['created_at'].timestamp()), j % 3 == 0, i])
    
    random.shuffle(task_data_list)
    User.bulkcreate(user_data_list)
    Task.bulkcreate(task_data_list)

    end_time = time.perf_counter()
    elapsed_time = (end_time - start_time) * 1000
    print(f"{elapsed_time:.2f}")

if __name__ == '__main__':
    main()

