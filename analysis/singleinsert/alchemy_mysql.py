from sqlalchemy import create_engine, Column, String, Integer, Boolean, DateTime
from sqlalchemy.orm import sessionmaker, declarative_base
from _data import user_data
import mysql.connector
import time
import sys
import os

Base = declarative_base()

class User(Base):
    __tablename__ = 'users'
    id = Column(Integer, primary_key=True)
    first_name = Column(String(50))
    last_name = Column(String(50))
    email = Column(String(50))
    age = Column(Integer)
    is_admin = Column(Boolean)
    created_at = Column(DateTime)

def main():
    dbname = 'singleselect'
    conn = mysql.connector.connect(host='localhost', port=3307, user='root', password='root')
    cursor = conn.cursor()
    cursor.execute(f"DROP DATABASE IF EXISTS {dbname}")
    cursor.execute(f"CREATE DATABASE IF NOT EXISTS {dbname}")
    cursor.close()
    conn.close()
    
    engine = create_engine(f'mysql+mysqlconnector://root:root@localhost:3307/{dbname}')
    num_users = int(sys.argv[1])
    Base.metadata.create_all(engine)
    Session = sessionmaker(bind=engine)
    start_time = time.perf_counter()

    session = Session()
    for i in range(num_users):
        current_user = {**user_data, "is_admin": i%2 == 0}
        user = User(**current_user)
        session.add(user)

    session.commit()
    session.close()
    end_time = time.perf_counter()
    elapsed_time = (end_time - start_time) * 1000
    print(f"{elapsed_time:.2f}")

if __name__ == '__main__':
    main()