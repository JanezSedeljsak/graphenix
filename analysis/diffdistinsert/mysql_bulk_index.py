from .data import user_data
import mysql.connector
import time
import sys
import os
import random
random.seed(12)

create_table = """
    CREATE TABLE `users` (
        `id` INTEGER NOT NULL AUTO_INCREMENT,
        `first_name` varchar(15) DEFAULT NULL,
        `last_name` varchar(15) DEFAULT NULL,
        `email` varchar(25) DEFAULT NULL,
        `points` int(11) DEFAULT NULL,
        `is_admin` tinyint(1) DEFAULT NULL,
        `created_at` datetime DEFAULT NULL,
    PRIMARY KEY (`id`),
    INDEX `idx_points` (`points`) USING BTREE
    ) ENGINE=InnoDB DEFAULT CHARSET=latin1;
    """

def main():
    num_users = int(sys.argv[1])
    dbname = f'db_index2_{num_users}'

    conn = mysql.connector.connect(host='localhost', port=3307, user='root', password='root')
    cursor = conn.cursor()
    cursor.execute(f"DROP DATABASE IF EXISTS {dbname}")
    cursor.execute(f"CREATE DATABASE IF NOT EXISTS {dbname}")
    cursor.execute(f"USE {dbname}")
    cursor.execute(create_table)
    cursor.close()
    conn.close()
    
    start_time = time.perf_counter()
    conn = mysql.connector.connect(host='localhost', port=3307, user='root', password='root')
    cursor = conn.cursor()
    cursor.execute(f"USE {dbname}")
    first_name_counter = 0
    chunk_size = 10000
    
    for chunk_start in range(0, num_users, chunk_size):
        chunk_end = min(chunk_start + chunk_size, num_users)
        user_data_list = []

        for i in range(chunk_start, chunk_end):
            is_admin = i % 2 == 0
            user_tuple = (user_data['first_name'] + str(first_name_counter), user_data['last_name'], user_data['email'],
                        random.randint(10, 80), int(is_admin), user_data['created_at'])
            user_data_list.append(user_tuple)

        first_name_counter += 1
        insert_query = f"INSERT INTO users (first_name, last_name, email, points, is_admin, created_at) " \
                    + "VALUES (%s, %s, %s, %s, %s, %s)"
        cursor.executemany(insert_query, user_data_list)
        conn.commit()

    cursor.close()
    conn.close()
    end_time = time.perf_counter()
    elapsed_time = (end_time - start_time) * 1000
    print(f"{elapsed_time:.2f}")

if __name__ == '__main__':
    main()