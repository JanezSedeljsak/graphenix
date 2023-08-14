from sqlalchemy import create_engine, Column, String, Integer, Boolean, DateTime
from sqlalchemy.orm import sessionmaker, declarative_base
from .data import user_data, get_shuffled_points
import time
import sys
import os

Base = declarative_base()

class User(Base):
    __tablename__ = 'users'
    id = Column(Integer, primary_key=True)
    first_name = Column(String(15))
    last_name = Column(String(15))
    email = Column(String(30))
    points = Column(Integer)
    is_admin = Column(Boolean)
    created_at = Column(DateTime)

def main():
    num_users = int(sys.argv[1])
    if os.path.exists(f'graphenix_db/sqlite_singleinsert_alchemy_{num_users}.db'):
        os.remove(f'graphenix_db/sqlite_singleinsert_alchemy_{num_users}.db')
       
    engine = create_engine(f'sqlite:///graphenix_db/sqlite_singleinsert_alchemy_{num_users}.db')
    Base.metadata.create_all(engine)
    Session = sessionmaker(bind=engine)
    start_time = time.perf_counter()

    session = Session()
    points = get_shuffled_points(num_users)
    users = [User(**{**user_data, "first_name": user_data['first_name'] + str(i),
                     "is_admin": i%2 == 0, "points": points[i]}) for i in range(num_users)]
    session.bulk_save_objects(users)

    session.commit()
    session.close()
    end_time = time.perf_counter()
    elapsed_time = (end_time - start_time) * 1000
    print(f"{elapsed_time:.2f}")

if __name__ == '__main__':
    main()