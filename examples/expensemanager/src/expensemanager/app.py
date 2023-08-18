import toga
from toga.style import Pack
from toga.style.pack import COLUMN, ROW, CENTER
from datetime import datetime
import graphenix as gx

TBL_PLACEHOLDER = ' ' * 40

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

        top_label = toga.Label(f"{invoice.title} - {invoice.amount}€", style=Pack(padding=(5, 10), font_weight='bold'))
        bottom_label = toga.Label(f"{invoice.expense_type.name} - {invoice.day.strftime('%d. %m. %Y')}", style=Pack(padding=(5, 10)))
        
        delete_button = toga.Button('Delete', on_press=self.delete_invoice, id=f'delete_{invoice.id}', style=Pack(width=80, padding=(10, 10)))
        edit_button = toga.Button('Edit', on_press=self.edit_invoice, id=f'edit_{invoice.id}', style=Pack(width=80, padding=(10, 10)))
    
        text_box = toga.Box(style=Pack(flex=1, direction=COLUMN))
        text_box.add(top_label)
        text_box.add(bottom_label)

        expense_box.add(text_box)
        expense_box.add(edit_button)
        expense_box.add(delete_button)

        return expense_box
    
    def get_search_tuple(self, widget, key):
        idx = -1
        search_id = widget.id.replace(key, 'expense_')
        actual_id = int(widget.id.replace(key, ''))
        for i, expense in enumerate(self.items_box.children):
            if expense.id == search_id:
                idx = i
                break
        
        return idx, actual_id
    
    def edit_invoice(self, widget):
        idx, actual_id = self.get_search_tuple(widget, 'edit_')
        if idx != -1:
            record = Invoice.link(expense_type=ExpenseType).filter(Invoice.equals(actual_id)).first()
            self.title_input.value = record.title
            self.amount_input.value = record.amount
            self.expense_type_input.value = record.expense_type.name
            self.on_open_form(None)
            self.edit_id = actual_id
    
    def delete_invoice(self, widget):
        idx, actual_id = self.get_search_tuple(widget, 'delete_')
        if idx != -1:
            self.items_box.remove(self.items_box.children[idx])
            record = Invoice.get(actual_id)
            record.delete()

    def display_expenses(self, search=None, order_by=None):
        query = Invoice.link(expense_type=ExpenseType)
        if search:
            query = query.filter(
                Invoice.title.iregex(f'.*{search}.*')
            )

        match order_by:
            case 'Title':
                query = query.order(Invoice.title)
            case 'Amount':
                query = query.order(Invoice.amount.desc())
            case _:
                query = query.order(Invoice.day.desc())
        
        _, invoices = query.all()
        for invoice in invoices:
            self.items_box.add(self.make_card(invoice))

    def do_refresh(self, widget):
        while self.items_box.children:
            self.items_box.remove(self.items_box.children[0])

        search_value = self.search.value
        order_value = self.order_by_options.value
        self.display_expenses(search=search_value, order_by=order_value)

    def refresh_stats(self):
        agg_data = Invoice.agg(by=Invoice.expense_type,
                               count=gx.AGG.count(), 
                               amount=gx.AGG.sum(Invoice.amount), 
                               latest=gx.AGG.max(Invoice.day))
        
        data = sorted(agg_data, key=lambda x: -x.amount)
        searilized = [(self.expense_type_names[row.expense_type], datetime.fromtimestamp(row.latest).strftime('%d. %m. %Y %H:%M'), str(row.count), f"{row.amount}€") for row in data]
        searilized.append((TBL_PLACEHOLDER, TBL_PLACEHOLDER, TBL_PLACEHOLDER, TBL_PLACEHOLDER))
        searilized.append(('Total', datetime.fromtimestamp(max([row.latest for row in data])).strftime('%d. %m. %Y %H:%M'),
                           sum([row.count for row in data]), f'{sum([row.amount for row in data])}€'))
        searilized.append((TBL_PLACEHOLDER, TBL_PLACEHOLDER, TBL_PLACEHOLDER, TBL_PLACEHOLDER))
        while self.stats.children:
            self.stats.remove(self.stats.children[0])

        table = toga.Table(['Expense type', 'Latest', 'Count', 'Total'], data=searilized, style=Pack(flex=1, alignment=CENTER))
        self.stats.add(table)

    def show_stats(self, widget):
        self.main_window.content = self.stats
        self.refresh_stats()

    def reset_form(self):
        self.title_input.value = ""
        self.amount_input.value = 10
        self.expense_type_input.value = self.expense_type_names[0]

    def on_go_home(self, widget):
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
            if hasattr(self, 'edit_id') and (self.edit_id or self.edit_id == 0):
                edit_data['_id'] = self.edit_id
            
            Invoice(
                title=input_content,
                amount=amount_input,
                day=datetime.now(),
                expense_type=self.name2etype[expense_type],
                **edit_data
            ).make()
            self.do_refresh(None)

            self.edit_id = None
            self.reset_form()
            self.on_go_home(None)

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
        self.back_button = toga.Button('Cancel', on_press=self.on_go_home, style=Pack(flex=0.5, padding=(5, 0)))
        self.search = toga.TextInput(style=Pack(flex=1, padding=(5, 0)), placeholder='Search ...', on_change=self.do_refresh)
        self.items_box = toga.Box(style=Pack(direction=COLUMN, padding=(10, 0)))
        self.scrollable = toga.ScrollContainer(content=self.items_box, style=Pack(height=400))
        self.order_by_options = toga.Selection(style=Pack(flex=1, padding=(5, 0)), items=['Title', 'Amount', 'Date'], on_select=self.do_refresh)
        
        self.main_box.add(self.search)
        self.main_box.add(self.order_by_options)
        self.main_box.add(self.scrollable)

        self.form = toga.Box(style=Pack(direction=COLUMN, padding=10))
        self.form.add(self.title_input)
        self.form.add(self.amount_input)
        self.form.add(self.expense_type_input)
        self.form.add(self.add_button)
        self.form.add(self.back_button)

        self.stats = toga.Box(style=Pack(direction=COLUMN, padding=10, flex=1))

        self.reset_form()
        self.display_expenses()
        self.main_window = toga.MainWindow(title=self.formal_name)

        actions = toga.Group("Actions")
        add_new_action = toga.Command(self.on_open_form, text="Add new item", tooltip="", group=actions)
        go_home_action = toga.Command(self.on_go_home, text="Home", tooltip="", group=actions)
        stats_action = toga.Command(self.show_stats, text="Stats", tooltip="", group=actions,)
        self.commands.add(add_new_action, go_home_action, stats_action)

        self.main_window.content = self.main_box
        self.main_window.show()


def main():
    return ExpenseManager()