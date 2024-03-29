import sqlite3
import time
import sys

def main():
    num_users = int(sys.argv[1])
    dbname = f'db_index_{num_users}'
    conn = sqlite3.connect(f'graphenix_db/{dbname}.db')
    
    start_time = time.perf_counter()
    cursor = conn.cursor()

    cursor.execute("SELECT * FROM users WHERE points = 5432")
    searilized = cursor.fetchall()
    
    assert len(searilized) == 1 and isinstance(searilized, list) 
    
    cursor.close()
    conn.close()
    end_time = time.perf_counter()
    elapsed_time = (end_time - start_time) * 1000
    print(f"{elapsed_time:.2f}")

if __name__ == '__main__':
    main()