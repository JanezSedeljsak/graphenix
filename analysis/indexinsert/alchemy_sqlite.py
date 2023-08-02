from sqlalchemy import create_engine, Column, String, Integer, Boolean, DateTime, Index
from sqlalchemy.orm import sessionmaker, declarative_base
from .data import user_data
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
    age_index = Index('idx_age', User.age)
    User.__table__.append_constraint(age_index)

    num_users = int(sys.argv[1])
    if os.path.exists(f'graphenix_db/db_index_{num_users}.db'):
        os.remove(f'graphenix_db/db_index_{num_users}.db')
       
    engine = create_engine(f'sqlite:///graphenix_db/db_index_{num_users}.db')
    Base.metadata.create_all(engine)
    Session = sessionmaker(bind=engine)
    start_time = time.perf_counter()

    session = Session()
    users = [User(**{**user_data, "first_name": user_data['first_name'] + str(i),
                     "is_admin": i%2 == 0, "age": random.randint(10, 80)}) for i in range(num_users)]
    session.bulk_save_objects(users)

    session.commit()
    session.close()
    end_time = time.perf_counter()
    elapsed_time = (end_time - start_time) * 1000
    print(f"{elapsed_time:.2f}")

if __name__ == '__main__':
    main()