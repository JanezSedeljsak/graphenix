import toga
from toga.style import Pack
from toga.style.pack import COLUMN, ROW
import graphenix as gx

class DailyGoal(gx.Model):
    title = gx.Field.String(size=20)

class Dailyplanner(toga.App):

    def add_goal(self, content: str):
        DailyGoal(title=content).make()
        label = toga.Label(content, style=Pack(padding=(0, 5)))
        self.main_box.add(label)

    def display_goals(self):
        _, goals = DailyGoal.all()
        for goal in goals:
            label = toga.Label(goal.title, style=Pack(padding=(0, 5)))
            self.main_box.add(label)


    def startup(self):
        schema = gx.Schema('dailyplanner_db', models=[DailyGoal])
        if not schema.exists():
            schema.create()
        
        self.main_box = toga.Box(style=Pack(direction=COLUMN, padding=10))

        # Create a Box to hold the text input and button horizontally
        input_box = toga.Box(style=Pack(direction=ROW, padding=(0, 0, 10, 0)))

        # Create a TextInput widget
        self.text_input = toga.TextInput(style=Pack(flex=1))

        # Create a Button widget with text "+"
        add_button = toga.Button('+', on_press=self.add_button_handler)

        # Add the TextInput and Button widgets to the input box
        input_box.add(self.text_input)
        input_box.add(add_button)

        # Add the input box to the main box
        self.main_box.add(input_box)
        self.display_goals()

        self.main_window = toga.MainWindow(title=self.formal_name)
        self.main_window.content = self.main_box
        self.main_window.show()

    def add_button_handler(self, widget):
        input_content = self.text_input.value
        if input_content:
            self.add_goal(input_content)
            self.text_input.value = ""


def main():
    return Dailyplanner()