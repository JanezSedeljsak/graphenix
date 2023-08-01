import mysql.connector
import time
import sys
import os

def main():
    num_users = int(sys.argv[1])
    dbname = f'raw_singleselect_{num_users}'

    conn = mysql.connector.connect(host='localhost', port=3307, user='root', password='root')
    start_time = time.perf_counter()

    cursor = conn.cursor(dictionary=True)
    cursor.execute(f"USE {dbname}")
    cursor.execute("SELECT * FROM users WHERE is_admin = 1 ORDER BY age, id LIMIT 500")
    searilized = cursor.fetchall()
    assert len(searilized) == 500 and isinstance(searilized[0], dict) and searilized[0]['age'] == 10 \
        and searilized[0]['first_name'] == 'John12'  
    
    cursor.close()
    conn.close()
    end_time = time.perf_counter()
    elapsed_time = (end_time - start_time) * 1000
    print(f"{elapsed_time:.2f}")

if __name__ == '__main__':
    main()