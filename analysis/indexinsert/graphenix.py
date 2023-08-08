from graphenix import Field, Schema, Model
from .data import user_data
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

def main():
    num_users = int(sys.argv[1])
    my_schema = Schema(f'graphenix_db_index_{num_users}', models=[User])
    if my_schema.exists():
        my_schema.delete()

    my_schema.create()

    start_time = time.perf_counter()
    for i in range(num_users):
        current_user = {**user_data, "first_name": user_data['first_name'] + str(i),
                        "is_admin": i%2 == 0, "points": random.randint(10, 8000)}
        user = User(**current_user)
        user.save()

    end_time = time.perf_counter()
    elapsed_time = (end_time - start_time) * 1000
    print(f"{elapsed_time:.2f}")

if __name__ == '__main__':
    main()

