from .data import user_data, task_data
import sqlite3
import time
import sys
import os
import random
random.seed(12)

create_table_users = """
     CREATE TABLE users (
        id INTEGER PRIMARY KEY,
        first_name TEXT,
        last_name TEXT,
        email TEXT,
        points INTEGER,
        is_admin INTEGER,
        created_at TIMESTAMP
    )
    """

create_table_tasks = """
    CREATE TABLE tasks (
        id INTEGER PRIMARY KEY,
        title TEXT,
        created_at TIMESTAMP,
        is_completed INTEGER,
        user_id INTEGER
    )
    """

def main():
    num_users = int(sys.argv[1])
    dbname = f'join_db_{num_users}'
    
    if os.path.exists(f'graphenix_db/{dbname}.db'):
        os.remove(f'graphenix_db/{dbname}.db')

    conn = sqlite3.connect(f'graphenix_db/{dbname}.db')
    cursor = conn.cursor()
    cursor.execute(create_table_users)
    cursor.execute(create_table_tasks)
    cursor.execute("CREATE INDEX idx_tasks_user_id ON tasks(user_id);")
    conn.commit()
    
    start_time = time.perf_counter()

    for i in range(num_users):
        cu = {**user_data, "is_admin": i%2 == 0, "points": random.randint(10, 8000),
              "first_name": user_data['first_name'] + str(i)}
        cursor.execute(
            "INSERT INTO users (first_name, last_name, email, points, is_admin, created_at) VALUES (?, ?, ?, ?, ?, ?)",
            tuple(cu.values())
        )

        for j in range(10):
            cursor.execute(f"INSERT INTO tasks (title, created_at, is_completed, user_id) VALUES (?, ?, ?, ?)",
                (task_data['title'] + str(j), task_data['created_at'], j % 3 == 0, i))
    
    conn.commit()
    cursor.close()
    conn.close()
    end_time = time.perf_counter()
    elapsed_time = (end_time - start_time) * 1000
    print(f"{elapsed_time:.2f}")

if __name__ == '__main__':
    main()