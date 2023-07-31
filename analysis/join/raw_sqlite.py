import sqlite3
import time
import sys

def main():
    num_users = int(sys.argv[1])
    dbname = f'sqlite_singleinsert_raw_{num_users}'
    conn = sqlite3.connect(f'graphenix_db/{dbname}.db')
    
    start_time = time.perf_counter()
    cursor = conn.cursor()

    
    
    cursor.close()
    conn.close()
    end_time = time.perf_counter()
    elapsed_time = (end_time - start_time) * 1000
    print(f"{elapsed_time:.2f}")

if __name__ == '__main__':
    main()