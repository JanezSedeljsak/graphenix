from graphenix import Field, Schema, Model
import sys
import time

class User(Model):
    first_name = Field.String(size=50)
    last_name = Field.String(size=50)
    email = Field.String(size=50)
    age = Field.Int()
    is_admin = Field.Bool()
    created_at = Field.DateTime()

def main():
    num_users = int(sys.argv[1])
    my_schema = Schema(f'singleinsert_{num_users}', models=[User])

    start_time = time.perf_counter()
    
    _, data = User\
        .filter(User.is_admin.equals(True))\
        .order(User.age, User).limit(500).all()
    
    searilized = data.as_dict()
    assert len(searilized) == 500 and isinstance(searilized[0], dict) and searilized[0]['age'] == 10 \
        and searilized[0]['first_name'] == 'John12'

    end_time = time.perf_counter()
    elapsed_time = (end_time - start_time) * 1000
    print(f"{elapsed_time:.2f}")

if __name__ == '__main__':
    main()