import logging
from sqlalchemy import create_engine, Column, String, Integer, ForeignKey
from sqlalchemy.orm import relationship, sessionmaker, joinedload
from sqlalchemy.ext.declarative import declarative_base

# Configure logging to print the executed queries
logging.basicConfig()
logging.getLogger('sqlalchemy.engine').setLevel(logging.INFO)

# Create a SQLite database engine
engine = create_engine('sqlite:///users.db')

# Create a base class for declarative models
Base = declarative_base()

# Define the User model
class User(Base):
    __tablename__ = 'users'
    id = Column(Integer, primary_key=True, autoincrement=True)
    name = Column(String)
    
    # One-to-many relationship with Task model
    tasks = relationship('Task', back_populates='user')

    # One-to-many relationship with Message model
    messages = relationship('Message', back_populates='user')

# Define the Task model
class Task(Base):
    __tablename__ = 'tasks'
    id = Column(Integer, primary_key=True, autoincrement=True)
    description = Column(String)
    user_id = Column(Integer, ForeignKey('users.id'))

    # Many-to-one relationship with User model
    user = relationship('User', back_populates='tasks')

# Define the Message model
class Message(Base):
    __tablename__ = 'messages'
    id = Column(Integer, primary_key=True, autoincrement=True)
    content = Column(String)
    user_id = Column(Integer, ForeignKey('users.id'))

    # Many-to-one relationship with User model
    user = relationship('User', back_populates='messages')

# Create the database tables
Base.metadata.create_all(engine)

# Create a session to interact with the database
Session = sessionmaker(bind=engine)
session = Session()

# Add some sample users, tasks, and messages to the database
users_data = [{'name': 'Alice'}, {'name': 'Bob'}, {'name': 'Charlie'}, {'name': 'John'}, {'name': 'Random'}]

for i in range(1000):
    user = User(**users_data[i % len(users_data)])
    session.add(user)

    for j in range(200):
        tasks_data = {'description': f'Task {j+1}', 'user_id': i+1}
        task = Task(**tasks_data)
        session.add(task)

    for j in range(1000):
        messages_data = {'content': f'Message {j+1}', 'user_id': i+1}
        message = Message(**messages_data)
        session.add(message)


# Commit the changes to the database
session.commit()

# Retrieve and print each user along with their tasks and messages
# users_with_tasks_messages = (
#     session.query(User)
#     .options(joinedload(User.tasks), joinedload(User.messages))
#     .all()
# )
# 
# for user in users_with_tasks_messages:
#     print(f"User ID: {user.id}, Name: {user.name}")
#     print("Tasks:")
#     for task in user.tasks:
#         print(f"  Task ID: {task.id}, Description: {task.description}")
#     print("Messages:")
#     for message in user.messages:
#         print(f"  Message ID: {message.id}, Content: {message.content}")

# Close the session
session.close()