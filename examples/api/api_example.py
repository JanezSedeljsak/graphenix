from graphenix import Schema, AGG
from flask import Flask, request, abort
from api_example_models import *
from api_example_searilizers import *
import json

app = Flask(__name__)
api_schema = Schema('api_schema', models=[Teacher, Laboratory])

def api_respnse(data):
    formated = json.dumps(data, indent=4)
    return app.response_class(formated, content_type='application/json')

@app.route('/labs')
def labs():
    _, labs_list = Laboratory.link(teachers = Teacher).all()
    return api_respnse(LaboratorySearilizer.jsonify(labs_list))

@app.route('/teachers')
def teacher():
    query = Teacher
    full_name = request.args.get('full_name')
    byname = request.args.get('byname')

    if full_name:
        query = query.filter(Teacher.full_name.regex(f'.*{full_name}.*'))

    if byname:
        query = query.order(Teacher.full_name)

    _, teacher_list = query.all()
    return api_respnse(TeacherSearilizer.jsonify(teacher_list))

@app.route('/teacher', methods=['POST'])
def add_teacher():
    full_name = request.form.get('full_name')
    email = request.form.get('email')
    lab_name = request.form.get('lab_name')

    lab = Laboratory.filter(Laboratory.name == lab_name).first()
    if lab is None:
        abort(404, description=f'Lab with name {lab_name} doesn\'t exist')

    new_teacher = Teacher(full_name=full_name, email=email, laboratory=lab).make()
    return api_respnse(TeacherSearilizer.jsonify(new_teacher))

@app.route('/stats')
def stats():
    query = Laboratory
    lab_name = request.args.get('lab_name')
    if lab_name:
        query.filter(Laboratory.name.regex(f'.*{lab_name}.*'))

    _, labs_list = query.all()
    lab_data = BasicLaboratorySearilizer.jsonify(labs_list)
    lab_ids = [lab['id'] for lab in lab_data]

    lab_stats = Teacher\
        .filter(Teacher.laboratory.is_in(lab_ids))\
        .agg(by=Teacher.laboratory, count=AGG.count())
    
    stats_dict = {stat.laboratory: stat.count for stat in lab_stats}
    for lab in lab_data:
        lab['teacher_count'] = stats_dict.get(lab['id'], 0)

    return api_respnse(lab_data)

@app.route('/teachers/<int:teacher_id>')
def get_teacher(teacher_id):
    teacher_detail = Teacher\
        .link(laboratory = Laboratory)\
        .filter(Teacher.equals(teacher_id))\
        .first()
    
    return api_respnse(DetailTeacherSearilizer.jsonify(teacher_detail))


if __name__ == '__main__':
    if not api_schema.exists():
        api_schema.create(delete_old=False)
        seed_db()

    app.run()
    
        