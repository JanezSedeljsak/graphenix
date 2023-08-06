import mysql.connector
import time
import sys
import os

def main():
    num_users = int(sys.argv[1])
    dbname = f'join_db_{num_users}'

    conn = mysql.connector.connect(host='localhost', port=3307, user='root', password='root')
    cursor = conn.cursor()
    cursor.execute(f"USE {dbname}")
    
    start_time = time.perf_counter()
    cursor.execute("SELECT * FROM users INNER JOIN tasks ON tasks.user_id = users.id")
    searilized = cursor.fetchall()
    end_time = time.perf_counter()

    cursor.close()
    conn.close()
    elapsed_time = (end_time - start_time) * 1000
    print(f"{elapsed_time:.2f}")

if __name__ == '__main__':
    main()