from graphenix import Field, Schema, Model, AGG, ViewSearilizer
from flask import Flask, request, jsonify, abort
from functools import wraps
from datetime import datetime, timedelta
import json

class ReqInfo(Model):
    route = Field.String(size=255)
    timestamp = Field.DateTime().as_index()
    route_req = Field.String(size=1024)
    route_res = Field.String(size=1024)
    resp_code = Field.Int()

logging = Schema('logging', models=[ReqInfo])
if not logging.exists():
    logging.create()

# last 3 requests
print('Last 3 ----------------')
count, data = ReqInfo.order(ReqInfo.timestamp.desc()).limit(3).all()
for row in data:
    print(ReqInfo.from_view(row))

# stats
print('Most popular routes ----------------')
route_stats = ReqInfo\
    .agg(by=ReqInfo.route, count=AGG.count())

sorted_stats = list(sorted(route_stats, key=lambda x: -x.count))
for row in sorted_stats:
    print(row)

# errors today
print('Errors today ----------------')
count, api_errors = ReqInfo\
    .filter(
        ReqInfo.resp_code >= 400,
        ReqInfo.timestamp > datetime.now() - timedelta(days=1)
    )\
    .order(ReqInfo.timestamp.desc())\
    .all()

print(f'Count: {count}')
for err in api_errors:
    print(ReqInfo.from_view(err))

# create csv
_, all_reqs = ReqInfo.all()
success = ViewSearilizer.default().dump2csv(
    all_reqs,
    f'log_{datetime.now().timestamp()}.csv'
)
