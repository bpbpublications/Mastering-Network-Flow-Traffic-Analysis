#!/opt/fl0wer/bin/fl0py

import os
import time
import csv
import pandas as pd
import datetime as datetime
import pathlib
import time
import lz4
from os import system
from collections import Counter
from tabulate import tabulate
from datetime import timedelta
from datetime import datetime

import clickhouse_driver as CH
from clickhouse_driver import Client

def dumpcsv(dataframe,filename):
	dataframe.to_csv(filename, encoding='utf-8', index=False)


server = "10.1.20.17"
csvoutput = "/tmp/alerts.csv"
days = 1

try:
	conn = CH.Client(	server, 
				secure=False, 
				verify=False, 
				compression=True)
except:
	print("Can't connect to Clickhouse server at: "+server)
	sys.exit(2)

# Let’s retrieve data since yesterday
datefrom  = datetime.now() - timedelta(days)
dateto	  = datetime.now()

strFrom  = datefrom.strftime("%Y/%m/%d %H:%M:%S")
strTo    = dateto.strftime("%Y/%m/%d %H:%M:%S")

print("Retrieving data into "+csvoutput)

try:
	# Load the data into a Pandas dataframe 
	df = conn.query_dataframe ("SELECT TIMESTAMP,LEVEL,CATEGORY,SOURCE,MESSAGE FROM FL0WER.EVENTS WHERE LEVEL LIKE \'%POLICY%\' AND MESSAGE LIKE \'%DNS%\' AND ( TIMESTAMP >= toDateTime(\'"+strFrom+"\') AND TIMESTAMP <= toDateTime(\'"+strTo+"\'))  ORDER BY TIMESTAMP DESCENDING;")
except:
	print("Can't retrieve data from Clickhouse server at: "+server)
	sys.exit(2)

print("Data fetched, formatting ...")	
dumpcsv(df,csvoutput)
print(tabulate(df, headers='keys', tablefmt='psql'))
