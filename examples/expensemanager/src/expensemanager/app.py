import toga
from toga.style import Pack
from toga.style.pack import COLUMN, ROW
from datetime import datetime
import graphenix as gx

class Invoice(gx.Model):
    title = gx.Field.String(size=20)
    amount = gx.Field.Int()
    day = gx.Field.DateTime().as_index()
    expense_type = gx.Field.Link()

class ExpenseType(gx.Model):
    name = gx.Field.String(size=30)

class ExpenseManager(toga.App):

    def make_card(self, invoice):
        expense_box = toga.Box(style=Pack(
            direction=COLUMN,
            padding=(5, 5),
            background_color='white'
        ))

        if isinstance(invoice, Invoice):
            invoice.connect_child(ExpenseType, 'expense_type')

        title_label = toga.Label(f"{invoice.title} - {invoice.amount}€", style=Pack(padding=(5, 10), font_weight='bold'))
        date_label = toga.Label(f"{invoice.expense_type.name} - {invoice.day.strftime('%d. %m. %Y')}", style=Pack(padding=(5, 10)))

        expense_box.add(title_label)
        expense_box.add(date_label)

        return expense_box

    def add_expense(self, **kwargs):
        Invoice(**kwargs).make()
        self.do_refresh(None)

    def display_expenses(self, search=None, order_by=None):
        query = Invoice.link(expense_type=ExpenseType)
        if search:
            query = query.filter(Invoice.title.regex(f'.*{search}.*'))

        match order_by:
            case 'Title':
                query = query.order(Invoice.title)
            case 'Amount':
                query = query.order(Invoice.amount.desc())
            case _:
                query = query.order(Invoice.day.desc())
        
        _, invoices = query.all()
        for invoice in invoices:
            card = self.make_card(invoice)
            self.items_box.add(card)

    def cmd_action(self, widget):
        print(f"Command {widget.label} was clicked")

    def do_refresh(self, widget):
        while self.items_box.children:
            self.items_box.remove(self.items_box.children[0])

        search_value = self.search.value
        order_value = self.order_by_options.value
        self.display_expenses(search=search_value, order_by=order_value)


    def startup(self):
        schema = gx.Schema('expensemanager_db', models=[Invoice, ExpenseType])
        if not schema.exists():
            schema.create()
            ExpenseType.bulkcreate([
                ["Food"], ["Utilities"], ["Clothing"],
                ["Entertainment"], ["Gas"]
            ])
        
        _, expense_types = ExpenseType.all()
        expense_types = expense_types
        self.expense_type_names = [et.name for et in expense_types]
        self.name2etype = {et.name: et.id for et in expense_types}

        self.main_box = toga.Box(style=Pack(direction=COLUMN, padding=10))
        self.title_input = toga.TextInput(style=Pack(flex=1, padding=(5, 0)), placeholder='Title')
        self.amount_input = toga.NumberInput(style=Pack(flex=1, padding=(5, 0)))
        self.expense_type_input = toga.Selection(style=Pack(flex=1, padding=(5, 0)), items=self.expense_type_names)
        self.add_button = toga.Button('+', on_press=self.add_button_handler, style=Pack(flex=0.5, padding=(5, 0)))
        self.search = toga.TextInput(style=Pack(flex=1, padding=(5, 0)), placeholder='Search ...', on_change=self.do_refresh)
        self.items_box = toga.Box(style=Pack(direction=COLUMN, padding=(10, 0)))
        self.scrollable = toga.ScrollContainer(content=self.items_box, style=Pack(height=400))
        self.order_by_options = toga.Selection(style=Pack(flex=1, padding=(5, 0)), items=['Title', 'Amount', 'Date'], on_select=self.do_refresh)
        
        self.amount_input.value = 10
        self.main_box.add(self.title_input)
        self.main_box.add(self.amount_input)
        self.main_box.add(self.expense_type_input)
        self.main_box.add(self.add_button)
        self.main_box.add(self.search)
        self.main_box.add(self.order_by_options)
        self.main_box.add(self.scrollable)
        self.display_expenses()

        self.main_window = toga.MainWindow(title=self.formal_name)
        self.main_window.content = self.main_box
        self.main_window.show()

    def add_button_handler(self, widget):
        input_content = self.title_input.value
        amount_input = self.amount_input.value
        expense_type = self.expense_type_input.value

        if amount_input and input_content:
            self.add_expense(
                title=input_content,
                amount=amount_input,
                day=datetime.now(),
                expense_type=self.name2etype[expense_type]
            )
            self.title_input.value = ""
            self.amount_input.value = 10
            self.expense_type_input.value = self.expense_type_names[0]


def main():
    return ExpenseManager()