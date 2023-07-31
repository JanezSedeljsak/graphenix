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
    first_name = Column(String(50))
    last_name = Column(String(50))
    email = Column(String(50))
    age = Column(Integer)
    is_admin = Column(Boolean)
    created_at = Column(DateTime)

def main():
    num_users = int(sys.argv[1])
    engine = create_engine(f'sqlite:///graphenix_db/sqlite_singleinsert_alchemy_{num_users}.db')
    Session = sessionmaker(bind=engine)
    start_time = time.perf_counter()

    session = Session()
    data = session.query(User).all()
    searilized = [row._asdict() for row in data]
    if len(searilized) != num_users or not isinstance(searilized[0], dict):
        raise ValueError("Searilization failed - recorsds should be list[dict]")

    session.close()
    end_time = time.perf_counter()
    elapsed_time = (end_time - start_time) * 1000
    print(f"{elapsed_time:.2f}")

if __name__ == '__main__':
    main()