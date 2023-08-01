from .data import user_data
import sqlite3
import time
import sys
import os
import random
random.seed(12)

create_table = """
     CREATE TABLE users (
        id INTEGER PRIMARY KEY,
        first_name TEXT,
        last_name TEXT,
        email TEXT,
        age INTEGER,
        is_admin INTEGER,
        created_at TIMESTAMP
    )
    """

create_index = """
    CREATE INDEX idx_age 
    ON users (age);
"""

def main():
    num_users = int(sys.argv[1])
    dbname = f'db_index_{num_users}'
    
    if os.path.exists(f'graphenix_db/{dbname}.db'):
        os.remove(f'graphenix_db/{dbname}.db')

    conn = sqlite3.connect(f'graphenix_db/{dbname}.db')
    cursor = conn.cursor()
    cursor.execute(create_table)
    conn.commit()
    cursor.execute(create_index)
    conn.commit()
    
    start_time = time.perf_counter()

    for i in range(num_users):
        cu = {**user_data, "is_admin": i%2 == 0, "age": random.randint(10, 80),
              "first_name": user_data['first_name'] + str(i)}
        cursor.execute(
            "INSERT INTO users (first_name, last_name, email, age, is_admin, created_at) VALUES (?, ?, ?, ?, ?, ?)",
            tuple(cu.values())
        )
    
    conn.commit()
    cursor.close()
    conn.close()
    end_time = time.perf_counter()
    elapsed_time = (end_time - start_time) * 1000
    print(f"{elapsed_time:.2f}")

if __name__ == '__main__':
    main()