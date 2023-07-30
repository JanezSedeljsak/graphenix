from graphenix import Field, Schema, Model
from datetime import datetime
import sys
import time

class User(Model):
    first_name = Field.String(size=50)
    last_name = Field.String(size=50)
    email = Field.String(size=50)
    age = Field.Int()
    is_admin = Field.Bool()
    created_at = Field.DateTime()

my_schema = Schema('singleinsert', models=[User])
my_schema.delete()
my_schema.create()

user_data = {
    "first_name": "John",
    "last_name": "Doe",
    "email": "john.doe@example.com",
    "age": 35,
    "is_admin": False,
    "created_at": datetime.now()
}

def main():
    num_users = int(sys.argv[1])
    start_time = time.perf_counter()
    for i in range(num_users):
        current_user = {**user_data, "is_admin": i%2 == 0}
        user = User(**current_user)
        user.save()

    end_time = time.perf_counter()
    elapsed_time = (end_time - start_time) * 1000
    print(f"{elapsed_time:.2f}")

if __name__ == '__main__':
    main()

