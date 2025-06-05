#!/bin/bash

rm -f flowmatrix.csv internal_networks.neo4j flowmatrix.db *.csv summary.txt

# Retrieve the flow matrix and the internal networks
/opt/fl0wer/bin/flcli admin fl0werr0x 10.1.30.210 7443 /flowmatrix 2>&1 > /dev/null

# for ease of use, load everything on a small database to perform some queries
sqlite3 -csv flowmatrix.db ".import flowmatrix.csv flowmatrix"
sqlite3 flowmatrix.db "CREATE INDEX srczone on flowmatrix(SRC_ZONE)"
sqlite3 flowmatrix.db "CREATE INDEX dstzone on flowmatrix(DST_ZONE)"

for i in `cat internal_networks.neo4j | grep -v ":ZONE" | awk -F, '{print $1}' | sort | uniq | sed 's/\"//g'`
do
        echo "Processing "$i
	 # Create per-VPC flow-matrix table in CSV for further analysis
        sqlite3 -header -csv flowmatrix.db "SELECT * FROM flowmatrix where\ SRC_ZONE=\""$i"\" OR DST_ZONE=\""$i"\" ORDER BY\ SRC_ZONE,SRC_SUBNET,DST_SUBNET,CAST(HITS AS INTEGER) DESC" > TRAFFIC-$i.csv
        echo "Zone: "$i" Flows: "`wc -l TRAFFIC-$i.csv` >> summary.txt
done
