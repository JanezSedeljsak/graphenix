from graphenix import Field, Schema, Model, AGG
import sys
import time
import random

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
    Schema(f'join_db_{num_users}', models=[User, Task])

    start_time = time.perf_counter()
    data = Task.agg(
        by=Task.user,
        count=AGG.count(),
        latest=AGG.max(Task.created_at)
    )
    
    assert len(data) == num_users and all(row.count == 10 for row in data)
    end_time = time.perf_counter()
    
    elapsed_time = (end_time - start_time) * 1000
    print(f"{elapsed_time:.2f}")

if __name__ == '__main__':
    main()

