from graphenix import Model, Field

class Teacher(Model):
    full_name = Field.String(size=256)
    email = Field.String(size=128)
    laboratory = Field.Link()

class Laboratory(Model):
    name = Field.String(size=64)
    room_number = Field.Int()
    teachers = Field.VirtualLink('laboratory')

def seed_db():
    Laboratory(name="AI", room_number=108).make(),
    Laboratory(name="Computer graphics", room_number=207).make(),
    Laboratory(name="Algorithms", room_number=31).make(),
    Laboratory(name="Bioinformatics", room_number=44).make(),
    Laboratory(name="Computer vision", room_number=100).make()

    Teacher(full_name="John Doe", email="johndoe@gmail.com", laboratory=Laboratory.get(0)).make()
    Teacher(full_name="Jane Smith", email="janesmith@gmail.com", laboratory=Laboratory.get(1)).make()
    Teacher(full_name="Lorem Ipsum", email="lorem@gmail.com", laboratory=Laboratory.get(1)).make()
    Teacher(full_name="Alice Johnson", email="alicejohnson@gmail.com", laboratory=Laboratory.get(2)).make()
    Teacher(full_name="Michael Brown", email="michaelbrown@gmail.com", laboratory=Laboratory.get(3)).make()
    Teacher(full_name="Emily Lee", email="emilylee@gmail.com", laboratory=Laboratory.get(3)).make()
