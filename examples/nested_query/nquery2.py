from datetime import datetime, timedelta
import graphenix as gx

class Task(gx.Model):
    content = gx.Field.String(size=100)
    user = gx.Field.Link()

class Message(gx.Model):
    content = gx.Field.String(size=50)
    date = gx.Field.DateTime().as_index()
    sender = gx.Field.Link()
    reciever = gx.Field.Link()

class User(gx.Model):
    name = gx.Field.String()
    tasks = gx.Field.VirtualLink("user")
    sent_msgs = gx.Field.VirtualLink("sender")
    recieved_msgs = gx.Field.VirtualLink("reciever")

    @classmethod
    def get_messages_and_last_n_tasks(cls, user_id: int, n: int):
        ''' Returns last <n> messages and tasks for a specific user '''
        user_data = cls.link(
            tasks = Task.limit(n),
            recieved_msgs = Message
                .link(sender = cls.link(tasks = Task.limit(n)))
                .order(Message.date.desc())
                .limit(n)
        )\
        .filter(cls.equals(user_id))\
        .first(as_view=True)

        return user_data
    
class TaskSearilizer(gx.ViewSearilizer):
    fields = ('id', 'content')

class UserBasicSearilizer(gx.ViewSearilizer):
    fields = ('id', 'name', 'tasks')
    tasks = TaskSearilizer

class MsgsSearilizer(gx.ViewSearilizer):
    fields = ('id', 'content', 'date', 'sender')
    sender = UserBasicSearilizer

class UserDetailSearilizer(gx.ViewSearilizer):
    fields = ('id', 'name', 'tasks', 'recieved_msgs')
    recieved_msgs = MsgsSearilizer
    tasks = TaskSearilizer

def main():
    schema = gx.Schema('test', models=[Task, Message, User])
    schema.create(delete_old=True)

    user1 = User(name='John Doe').make()
    user2 = User(name='Jane Doe').make()

    for i in range(10):
        Task(content=f'Taskk {i}', user=user1).make()
        Task(content=f'Taskk_Jane {i}', user=user2).make()
        Message(
            content=f'Message {i}',
            date=datetime.now() + timedelta(seconds=i),
            sender=user1,
            reciever=user2
        ).make()

    user_data = User.get_messages_and_last_n_tasks(1, 5)
    searilized = UserDetailSearilizer.jsonify(user_data) 
    print(searilized)


if __name__ == '__main__':
    main()