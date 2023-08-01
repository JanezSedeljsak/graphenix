#!/bin/bash

size=$1

echo "Started file size analysis"

if [ -f "analysis/singleinsert/size_$size.txt" ]; then
    rm "analysis/singleinsert/size_$size.txt"
fi

if [ -d "graphenix_db/singleinsert_bulk_$size" ]; then
    echo "  Found graphenix db..."
    graphenix_size=$(du -sb graphenix_db/singleinsert_bulk_$size | awk '{print $1}')
    echo "$graphenix_size" >> "analysis/singleinsert/size_$size.txt"
    echo "  Size was: $graphenix_size"
fi

if [ -f "graphenix_db/sqlite_singleinsert_raw_$size.db" ]; then
    echo "  Found sqlite db..."
    sqlite_size=$(du -sb graphenix_db/sqlite_singleinsert_raw_$size.db | awk '{print $1}')
    echo "$sqlite_size" >> "analysis/singleinsert/size_$size.txt"
    echo "  Size was: $sqlite_size"
fi

# get mysql size
echo "  Trying to get MySQL db size..."
mysql_size=$(docker exec -it fdc7ff9ad883 bash -c "mysql -u root -proot << EOF
    USE information_schema;
    SELECT SUM(t.data_length + t.index_length) AS size
    FROM tables as t
    WHERE t.table_schema = 'raw_singleselect_$size';
EOF
" | sed -n '2p' | awk '{print $1}')

echo "$mysql_size" >> "analysis/singleinsert/size_$size.txt"
echo "  Size was: $mysql_size"
echo "Done....."