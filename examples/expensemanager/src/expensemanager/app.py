import toga
from toga.style import Pack
from toga.style.pack import COLUMN, ROW, RIGHT
from datetime import datetime
import graphenix as gx

class Invoice(gx.Model):
    title = gx.Field.String(size=20)
    amount = gx.Field.Int()
    day = gx.Field.DateTime()
    expense_type = gx.Field.Link()

class ExpenseType(gx.Model):
    name = gx.Field.String(size=30)

class ExpenseManager(toga.App):

    def make_card(self, invoice):
        expense_box = toga.Box(style=Pack(
            direction=ROW,
            padding=(5, 5),
            background_color='white',
            flex=1
        ), id=f'expense_{invoice.id}')

        if isinstance(invoice, Invoice):
            invoice.connect_child(ExpenseType, 'expense_type')

        top_label = toga.Label(f"{invoice.title} - {invoice.amount}€", style=Pack(padding=(5, 10), font_weight='bold'))
        bottom_label = toga.Label(f"{invoice.expense_type.name} - {invoice.day.strftime('%d. %m. %Y')}", style=Pack(padding=(5, 10)))
        
        delete_button = toga.Button('X', on_press=self.delete_invoice, id=f'delete_{invoice.id}', style=Pack(width=80, padding=(10, 10)))
        edit_button = toga.Button('Edit', on_press=self.edit_invoice, id=f'edit_{invoice.id}', style=Pack(width=80, padding=(10, 10)))
    
        text_box = toga.Box(style=Pack(flex=1, direction=COLUMN))
        text_box.add(top_label)
        text_box.add(bottom_label)

        expense_box.add(text_box)
        expense_box.add(edit_button)
        expense_box.add(delete_button)

        return expense_box
    
    def edit_invoice(self, widget):
        idx = -1
        search_id = widget.id.replace('edit_', 'expense_')
        actual_id = int(widget.id.replace('edit_', ''))
        for i, expense in enumerate(self.items_box.children):
            if expense.id == search_id:
                idx = i
                break

        if idx != -1:
            record = Invoice.get(actual_id)
            record.connect_child(ExpenseType, 'expense_type')
            self.title_input.value = record.title
            self.amount_input.value = record.amount
            self.expense_type_input.value = record.expense_type.name
            self.on_open_form(None)
            self.edit_id = actual_id
    
    def delete_invoice(self, widget):
        idx = -1
        search_id = widget.id.replace('delete_', 'expense_')
        actual_id = int(widget.id.replace('delete_', ''))
        for i, expense in enumerate(self.items_box.children):
            if expense.id == search_id:
                idx = i
                break

        if idx != -1:
            self.items_box.remove(self.items_box.children[idx])
            record = Invoice.get(actual_id)
            record.delete()

    def upsert_expense(self, **invoice_data):
        Invoice(**invoice_data).make()
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
        self.add_button = toga.Button('Save', on_press=self.on_upsert_expense, style=Pack(flex=0.5, padding=(5, 0)))
        self.back_button = toga.Button('Cancel', on_press=self.on_go_back, style=Pack(flex=0.5, padding=(5, 0)))
        self.open_form = toga.Button('+', on_press=self.on_open_form, style=Pack(flex=0.5, padding=(5, 0)))
        self.search = toga.TextInput(style=Pack(flex=1, padding=(5, 0)), placeholder='Search ...', on_change=self.do_refresh)
        self.items_box = toga.Box(style=Pack(direction=COLUMN, padding=(10, 0)))
        self.scrollable = toga.ScrollContainer(content=self.items_box, style=Pack(height=400))
        self.order_by_options = toga.Selection(style=Pack(flex=1, padding=(5, 0)), items=['Title', 'Amount', 'Date'], on_select=self.do_refresh)
        
        self.amount_input.value = 10
        self.main_box.add(self.open_form)
        self.main_box.add(self.search)
        self.main_box.add(self.order_by_options)
        self.main_box.add(self.scrollable)

        self.form = toga.Box(style=Pack(direction=COLUMN, padding=10))
        self.form.add(self.title_input)
        self.form.add(self.amount_input)
        self.form.add(self.expense_type_input)
        self.form.add(self.add_button)
        self.form.add(self.back_button)

        self.reset_form()
        self.display_expenses()
        self.main_window = toga.MainWindow(title=self.formal_name)
        self.main_window.content = self.main_box
        self.main_window.show()

    def reset_form(self):
        self.title_input.value = ""
        self.amount_input.value = 10
        self.expense_type_input.value = self.expense_type_names[0]

    def on_go_back(self, widget):
        self.main_window.content = self.main_box
        self.edit_id = None

    def on_open_form(self, widget):
        self.main_window.content = self.form

    def on_upsert_expense(self, widget):
        input_content = self.title_input.value
        amount_input = self.amount_input.value
        expense_type = self.expense_type_input.value

        if amount_input and input_content:
            edit_data = {}
            if self.edit_id:
                edit_data['_id'] = self.edit_id

            self.upsert_expense(
                title=input_content,
                amount=amount_input,
                day=datetime.now(),
                expense_type=self.name2etype[expense_type],
                **edit_data
            )

            self.edit_id = None
            self.reset_form()
            self.on_go_back(None)


def main():
    return ExpenseManager()