from sqlalchemy import create_engine, Column, String, Integer, Boolean, DateTime
from sqlalchemy.orm import sessionmaker, declarative_base
from .data import user_data
import mysql.connector
import time
import sys
import os
import random
random.seed(12)

Base = declarative_base()

class User(Base):
    __tablename__ = 'users'
    id = Column(Integer, primary_key=True)
    first_name = Column(String(15))
    last_name = Column(String(15))
    email = Column(String(30))
    age = Column(Integer)
    is_admin = Column(Boolean)
    created_at = Column(DateTime)

def main():
    num_users = int(sys.argv[1])
    dbname = f'singleselect_{num_users}'

    conn = mysql.connector.connect(host='localhost', port=3307, user='root', password='root')
    cursor = conn.cursor()
    cursor.execute(f"DROP DATABASE IF EXISTS {dbname}")
    cursor.execute(f"CREATE DATABASE IF NOT EXISTS {dbname}")
    cursor.close()
    conn.close()
    
    engine = create_engine(f'mysql+mysqlconnector://root:root@localhost:3307/{dbname}')
    Base.metadata.create_all(engine)
    Session = sessionmaker(bind=engine)
    start_time = time.perf_counter()

    users = [User(**{**user_data, "first_name": user_data['first_name'] + str(i),
                     "is_admin": i%2 == 0, "age": random.randint(10, 80)}) for i in range(num_users)]

    session = Session()
    session.bulk_save_objects(users)
    session.commit()

    session.close()
    end_time = time.perf_counter()
    elapsed_time = (end_time - start_time) * 1000
    print(f"{elapsed_time:.2f}")

if __name__ == '__main__':
    main()