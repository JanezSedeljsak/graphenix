import sqlite3
import time
import sys

def main():
    num_users = int(sys.argv[1])
    dbname = f'join_db_{num_users}'
    conn = sqlite3.connect(f'graphenix_db/{dbname}.db')
    cursor = conn.cursor()
    
    start_time = time.perf_counter()
    cursor.execute("SELECT user_id, COUNT(*) as 'count', MAX(created_at) as 'latest' FROM tasks GROUP BY user_id")
    data = cursor.fetchall()
    assert len(data) == num_users and all(row[1] == 10 for row in data)
    end_time = time.perf_counter()

    cursor.close()
    conn.close()
    elapsed_time = (end_time - start_time) * 1000
    print(f"{elapsed_time:.2f}")

if __name__ == '__main__':
    main()