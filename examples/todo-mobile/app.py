from kivy.app import App
from kivy.uix.boxlayout import BoxLayout
from kivy.uix.label import Label
from kivy.uix.scrollview import ScrollView

from datetime import datetime
import graphenix as gx

class DailyGoal(gx.Model):
    title = gx.Field.String(size=30)
    description = gx.Field.String(size=100)
    day = gx.Field.DateTime()
    is_done = gx.Field.Bool()

class ListItem(BoxLayout):
    def __init__(self, data, **kwargs):
        super(ListItem, self).__init__(**kwargs)
        self.orientation = 'horizontal'
        self.spacing = 10

        left_box = BoxLayout(orientation='vertical', size_hint=(0.7, 1))
        left_box.add_widget(Label(text=data['title'], bold=True, halign='left'))
        left_box.add_widget(Label(text=data['description'], halign='left'))

        right_box = BoxLayout(orientation='vertical', size_hint=(0.3, 1))
        right_box.add_widget(Label(text=data['day'], halign='right'))
        right_box.add_widget(Label(text=str(data['is_done']), halign='right'))

        self.add_widget(left_box)
        self.add_widget(right_box)

class MainApp(App):
    def build(self):
        root = ScrollView()
        layout = BoxLayout(orientation='vertical', spacing=0, padding=0)
        
        _, view = DailyGoal.all()
        rows = gx.ViewSearilizer.default().jsonify(view)
        print(rows)
        for row in rows:
            layout.add_widget(ListItem(row))

        root.add_widget(layout)
        return root

if __name__ == '__main__':
    todo_schema = gx.Schema('todomobile', models=[DailyGoal])
    todo_schema.delete()
    if not todo_schema.exists():
        todo_schema.create()

    DailyGoal(title='Go for a run', description='fdsf', day=datetime.now(), is_done=True).make()
    DailyGoal(title='Finish diploma', description='test', day=datetime.now(), is_done=False).make()
    DailyGoal(title='Hike to the top of Triglav 1', description='......', day=datetime.now(), is_done=False).make()
    DailyGoal(title='Hike to the top of Triglav 2', description='......', day=datetime.now(), is_done=False).make()
    DailyGoal(title='Hike to the top of Triglav 3', description='......', day=datetime.now(), is_done=False).make()
    DailyGoal(title='Hike to the top of Triglav 3', description='......', day=datetime.now(), is_done=False).make()
    DailyGoal(title='Hike to the top of Triglav 3', description='......', day=datetime.now(), is_done=False).make()
    DailyGoal(title='Hike to the top of Triglav 3', description='......', day=datetime.now(), is_done=False).make()
    DailyGoal(title='Hike to the top of Triglav 3', description='......', day=datetime.now(), is_done=False).make()

    MainApp().run()