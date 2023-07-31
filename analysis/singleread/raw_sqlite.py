import sqlite3
import time
import sys

def main():
    num_users = int(sys.argv[1])
    dbname = f'sqlite_singleinsert_raw_{num_users}'
    conn = sqlite3.connect(f'graphenix_db/{dbname}.db')
    
    start_time = time.perf_counter()
    cursor = conn.cursor()

    cursor = conn.cursor()
    cursor.execute("SELECT * FROM users")
    searilized = [{col[0]: row[idx] for idx, col in enumerate(cursor.description)}
                  for row in cursor.fetchall()]
    
    if len(searilized) != num_users or not isinstance(searilized[0], dict):
        raise ValueError("Searilization failed - recorsds should be list[dict]")  
    
    cursor.close()
    conn.close()
    end_time = time.perf_counter()
    elapsed_time = (end_time - start_time) * 1000
    print(f"{elapsed_time:.2f}")

if __name__ == '__main__':
    main()