from sqlalchemy import create_engine, Column, String, Integer, Boolean, DateTime, inspect, func
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

    id = Column(Integer, primary_key=True, autoincrement=True)
    title = Column(String(15), nullable=True)
    created_at = Column(DateTime, nullable=True)
    is_completed = Column(Boolean, nullable=True)
    user_id = Column(Integer, nullable=True)

def main():
    num_users = int(sys.argv[1])
    dbname = f'join_db_{num_users}'
    
    engine = create_engine(f'mysql+mysqlconnector://root:root@localhost:3307/{dbname}')
    Session = sessionmaker(bind=engine)
    session = Session()

    start_time = time.perf_counter()
    data = (
        session.query(
            Task.user_id,
            func.count().label('count'),
            func.max(Task.created_at).label('latest')
        )
        .group_by(Task.user_id)
        .all()
    )

    assert len(data) == num_users and all(row.count == 10 for row in data)
    end_time = time.perf_counter()
    
    session.close()
    elapsed_time = (end_time - start_time) * 1000
    print(f"{elapsed_time:.2f}")

if __name__ == '__main__':
    main()