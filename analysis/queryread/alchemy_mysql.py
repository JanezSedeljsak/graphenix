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
    age = Column(Integer)
    is_admin = Column(Boolean)
    created_at = Column(DateTime)

def main():
    num_users = int(sys.argv[1])
    dbname = f'singleselect_{num_users}'
    
    engine = create_engine(f'mysql+mysqlconnector://root:root@localhost:3307/{dbname}')
    Session = sessionmaker(bind=engine)

    start_time = time.perf_counter()
    session = Session()

    data = session.query(User).filter(User.is_admin == 1).order_by(User.age, User.id).limit(500).all()
    searilized = [row._asdict() for row in data]
    assert len(searilized) == 500 and isinstance(searilized[0], dict) and searilized[0]['age'] == 10 \
        and searilized[0]['first_name'] == 'John12'

    session.close()
    end_time = time.perf_counter()
    elapsed_time = (end_time - start_time) * 1000
    print(f"{elapsed_time:.2f}")

if __name__ == '__main__':
    main()