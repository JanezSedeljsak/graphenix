from graphenix import Field, Schema, Model
from flask import Flask, request, jsonify, Response
from functools import wraps
from datetime import datetime
import json

class ReqInfo(Model):
    route = Field.String(size=255)
    timestamp = Field.DateTime().as_index()
    route_req = Field.String(size=1024)
    route_res = Field.String(size=1024)
    resp_code = Field.Int()

logging = Schema('logging', models=[ReqInfo])

def route_with_log(route):
    def decorator(f):
        @wraps(f)
        def wrapper(*args, **kwargs):
            try:
                response = f(request, *args, **kwargs)
            except Exception as err:
                response = None

            ReqInfo(
                route=route,
                timestamp=datetime.now(),
                route_req=json.dumps(dict(request.args)),
                route_res=response.get_data(as_text=True) if response else '',
                resp_code=response.status_code if response else 500
            ).make()

            return response
        
        app.add_url_rule(route, view_func=wrapper)
        return wrapper
    return decorator

app = Flask(__name__)
@route_with_log('/get-range')
def get_range(request):
    start = int(request.args.get('start', 0))
    end = int(request.args.get('end', 10))
    step = int(request.args.get('step', 1))
    return jsonify({'Range': list(range(start, end, step))})

@route_with_log('/hello')
def hello(request):
    return jsonify({'Hello': 'World'})

@route_with_log('/err')
def err404(request):
    return Response('404 - page not found', status=404)

if __name__ == '__main__':
    if not logging.exists():
        logging.create()

    app.run()
