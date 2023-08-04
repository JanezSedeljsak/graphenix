from graphenix import Field, Schema, Model
import sys
import time

sizes = {
    100000: 1_459,
    1000000: 14_197,
    10000000: 140_758,
}

class User(Model):
    first_name = Field.String(size=15)
    last_name = Field.String(size=15)
    email = Field.String(size=25)
    points = Field.Int()
    is_admin = Field.Bool()
    created_at = Field.DateTime()

def main():
    num_users = int(sys.argv[1])
    Schema(f'db_index2_{num_users}', models=[User])

    start_time = time.perf_counter()
    
    _, data = User.filter(User.points.equals(32)).all()
    assert len(data) == sizes[num_users] and isinstance(data, list)

    end_time = time.perf_counter()
    elapsed_time = (end_time - start_time) * 1000
    print(f"{elapsed_time:.2f}")

if __name__ == '__main__':
    main()

