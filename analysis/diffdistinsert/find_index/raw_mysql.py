import mysql.connector
import time
import sys
import os

sizes = {
    100000: 1_459,
    1000000: 14_197,
    10000000: 140_758,
}


def main():
    num_users = int(sys.argv[1])
    dbname = f'db_index2_{num_users}'

    conn = mysql.connector.connect(host='localhost', port=3307, user='root', password='root')
    cursor = conn.cursor()
    cursor.execute(f"USE {dbname}")
    start_time = time.perf_counter()
    
    cursor.execute("SELECT * FROM users WHERE points = 32")
    searilized = cursor.fetchall()
    assert len(searilized) == sizes[num_users] and isinstance(searilized, list)
    
    cursor.close()
    conn.close()
    end_time = time.perf_counter()
    elapsed_time = (end_time - start_time) * 1000
    print(f"{elapsed_time:.2f}")

if __name__ == '__main__':
    main()