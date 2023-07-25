from graphenix import Field, Schema, Model
from flask import Flask, request, jsonify
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
            response = f(request, *args, **kwargs)
            ReqInfo(
                route=route,
                timestamp=datetime.now(),
                route_req=json.dumps(dict(request.args)),
                route_res=response.get_data(as_text=True),
                resp_code=response.status_code
            ).make()

            return response
        
        app.add_url_rule(route, view_func=wrapper)
        return wrapper
    return decorator

app = Flask(__name__)
@route_with_log('/get-range')
def hello(request):
    start = int(request.args.get('start', 0))
    end = int(request.args.get('end', 10))
    step = int(request.args.get('step', 1))
    return jsonify({'Range': list(range(start, end, step))})


if __name__ == '__main__':
    if not logging.exists():
        logging.create(delete_old=True)

    count, data = ReqInfo.order(ReqInfo.timestamp.desc()).limit(3).all()
    print(f'Count: {count}')
    for row in data:
        print(ReqInfo.from_view(row))

    app.run()
