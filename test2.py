from graphenix import Field, Schema, Model
import time

class City(Model):
    name = Field.String(size=50)
    country = Field.String(size=50)
    population_thousands = Field.Int()

mock_schema = Schema('test3', models=[City])
#mock_schema.create(delete_old=True)

#AMOUNT = 10_000_000
#temp_record = City(name="Ljubljana", country="SLO", population_thousands=280)
#for i in range(AMOUNT):
#    if i % 500_000 == 0:
#        print(f"Done with {i} -> {i/AMOUNT*100}%")
#
#    temp_record.save()
#    temp_record._id = -1

start_time = time.perf_counter()
count, data = City.all()
end_time = time.perf_counter()

print(f"Execution time: {end_time - start_time:.6f} seconds")
x = 5