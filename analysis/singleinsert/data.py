from datetime import datetime
import random
random.seed(12)

user_data = {
    "first_name": "John",
    "last_name": "Doe",
    "email": "john.doe@example.com",
    "points": 35,
    "is_admin": False,
    "created_at": datetime.now()
}

def get_shuffled_points(n):
    nums = list(range(n))
    random.shuffle(nums)
    return nums