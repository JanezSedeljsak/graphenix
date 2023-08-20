from datetime import datetime
import pandas as pd
import polars as pl
import graphenix as gx

class ReqInfo(gx.Model):
    route = gx.Field.String(size=255)
    timestamp = gx.Field.DateTime().as_index()
    route_req = gx.Field.String(size=1024)
    route_res = gx.Field.String(size=1024)
    resp_code = gx.Field.Int()

logging = gx.Schema('logging', models=[ReqInfo])
if not logging.exists():
    logging.create()

_, qview = ReqInfo.order(ReqInfo.timestamp)\
    .filter(ReqInfo.timestamp.greater_or_equal(
        datetime(2020, 1, 1)))\
    .limit(10000)\
    .all()

pandas_df : pd.DataFrame = qview.as_pandas_df()
polars_df : pl.DataFrame = qview.as_polars_df()

print(pandas_df[:2])
print("-------------------")
print(polars_df[:2])
