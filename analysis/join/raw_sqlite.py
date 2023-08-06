import sqlite3
import time
import sys

def main():
    num_users = int(sys.argv[1])
    dbname = f'join_db_{num_users}'
    conn = sqlite3.connect(f'graphenix_db/{dbname}.db')
    cursor = conn.cursor()
    
    start_time = time.perf_counter()
    cursor.execute("SELECT * FROM users INNER JOIN tasks ON tasks.user_id = users.id")
    data = cursor.fetchall()
    end_time = time.perf_counter()

    cursor.close()
    conn.close()
    elapsed_time = (end_time - start_time) * 1000
    print(f"{elapsed_time:.2f}")

if __name__ == '__main__':
    main()