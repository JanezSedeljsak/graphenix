import sqlite3
import time
import sys

def main():
    num_users = int(sys.argv[1])
    dbname = f'sqlite_singleinsert_raw_{num_users}'
    conn = sqlite3.connect(f'graphenix_db/{dbname}.db')
    
    start_time = time.perf_counter()
    cursor = conn.cursor()

    cursor.execute("SELECT * FROM users WHERE is_admin = 1 ORDER BY age, id LIMIT 500")
    searilized = [{col[0]: row[idx] for idx, col in enumerate(cursor.description)}
                  for row in cursor.fetchall()]
    
    assert len(searilized) == 500 and isinstance(searilized[0], dict) and searilized[0]['age'] == 10 \
        and searilized[0]['first_name'] == 'John12'
    
    cursor.close()
    conn.close()
    end_time = time.perf_counter()
    elapsed_time = (end_time - start_time) * 1000
    print(f"{elapsed_time:.2f}")

if __name__ == '__main__':
    main()