from sqlalchemy import create_engine, Column, String, Integer, Boolean, DateTime, inspect
from sqlalchemy.orm import sessionmaker, as_declarative

import time
import sys

@as_declarative()
class Base:
    def _asdict(self):
        return {c.key: getattr(self, c.key)
                for c in inspect(self).mapper.column_attrs}

class User(Base):
    __tablename__ = 'users'
    id = Column(Integer, primary_key=True)
    first_name = Column(String(15))
    last_name = Column(String(15))
    email = Column(String(30))
    points = Column(Integer)
    is_admin = Column(Boolean)
    created_at = Column(DateTime)

class Task(Base):
    __tablename__ = 'tasks'

    id = Column(Integer, primary_key=True)
    title = Column(String(15))
    created_at = Column(DateTime)
    is_completed = Column(Boolean)
    user_id = Column(Integer)

def main():
    num_users = int(sys.argv[1])
    engine = create_engine(f'sqlite:///graphenix_db/join_db_{num_users}.db')
    Session = sessionmaker(bind=engine)
    session = Session()

    start_time = time.perf_counter()
    users_with_tasks = session.query(User).join(Task, User.id == Task.user_id).all()
    end_time = time.perf_counter()

    session.close()
    elapsed_time = (end_time - start_time) * 1000
    print(f"{elapsed_time:.2f}")

if __name__ == '__main__':
    main()