#!/opt/fl0wer/bin/fl0py
import argparse,socket,ssl
import hashlib
try:
    import simplejson as json
except ImportError:
    import json
import time
import tempfile
import shutil
import datetime
import zipfile
import pprint
import getpass
import platform

import os, sys, csv
import fl0wernet as flnet
from tabulate import tabulate
import pprint


def comma_format(b):
	r =  str("{:,}".format(b))
	return r

def size_format(b):
	if b < 1024:
		h =  '%i' % b  + ' bytes'
	elif 1024 <= b < 1048576:
		h =  '%.1f' % float(b/1024) + ' KBytes'
	elif 1048576 <= b < 1073741824:
		h =  '%.1f' % float(b/1048576) + ' MBytes'
	elif 1073741824 <= b < 1099511627776:
		h =  '%.1f' % float(b/1073741824) + ' GBytes'
	elif 1099511627776 <= b < 1125899906842624:
		h =  '%.1f' % float(b/1099511627776) + ' TBytes'
	elif 1125899906842624 <= b:
		h = '%.1f' % float(b/1125899906842624) + ' PBytes'
	
	#r = comma_format(b) + " ("+h+")"	
	r = h 
	return r


user = "admin"
pw = "fl0werr0x"
server = "10.1.30.222"
server_port = 7443
port = server_port

lista = []

print("Retrieving Flowmatrix from Fl0wer server",server,"at port: ",port)
data_flowmatrix = flnet.GetFlowMatrix(user,pw,server,server_port)

datasource = data_flowmatrix

if (len(datasource) > 0):
	lista.append( ["srcZone","dstZone","srcSubnet","srcDescr","dstSubnet","dstDescr","flow_direction","protocol","dst_port","hits","bytes","packets","NPAR","RULE", "FlowDirection", "FirstSeen", "LastSeen", "Action","Category" ])
	for i in range(0,len(datasource)):
		srcZone = str(datasource[i]["FlowMatrixEntry"+str(i)]["srcZone"])
		dstZone = str(datasource[i]["FlowMatrixEntry"+str(i)]["dstZone"])
		srcNet	= str(datasource[i]["FlowMatrixEntry"+str(i)]["srcSubnet"])
		srcDsc	= str(datasource[i]["FlowMatrixEntry"+str(i)]["srcDescr"])
		srcMsk	= int(datasource[i]["FlowMatrixEntry"+str(i)]["srcNetmask"])
		dstNet	= str(datasource[i]["FlowMatrixEntry"+str(i)]["dstSubnet"])
		dstDsc	= str(datasource[i]["FlowMatrixEntry"+str(i)]["dstDescr"])
		dstMsk	= int(datasource[i]["FlowMatrixEntry"+str(i)]["dstNetmask"])
		protocol= str(datasource[i]["FlowMatrixEntry"+str(i)]["ipProtocol"])
		dstport	= int(datasource[i]["FlowMatrixEntry"+str(i)]["dstPort"])
		NPAR	= str(datasource[i]["FlowMatrixEntry"+str(i)]["NPAR"])
		RULE	= str(datasource[i]["FlowMatrixEntry"+str(i)]["RULE"])
		hits	= int(datasource[i]["FlowMatrixEntry"+str(i)]["hits"])
		packets = int(datasource[i]["FlowMatrixEntry"+str(i)]["packets"])
		bytes   = int(datasource[i]["FlowMatrixEntry"+str(i)]["bytes"])
		flowdir = str(datasource[i]["FlowMatrixEntry"+str(i)]["FlowDirection"])
		first   = str(datasource[i]["FlowMatrixEntry"+str(i)]["FirstSeen"])
		last    = str(datasource[i]["FlowMatrixEntry"+str(i)]["LastSeen"])
		action  = str(datasource[i]["FlowMatrixEntry"+str(i)]["action"])
		category= str(datasource[i]["FlowMatrixEntry"+str(i)]["trafficCategory"])
			
		lista.append([srcZone,dstZone,str(srcNet)+"/"+str(srcMsk), srcDsc, str(dstNet)+"/"+str(dstMsk),dstDsc,flowdir,protocol,dstport,hits,bytes,packets,NPAR,RULE,flowdir,first,last,action,category])

	try:
		if (platform.system() == 'Windows'):
			myfile = open('flowmatrix.csv', 'bw',encoding='utf-8')
		else:
			myfile = open('flowmatrix.csv', 'w',encoding='utf-8')

		writer = csv.writer(myfile,quoting=csv.QUOTE_ALL,lineterminator='\n')
		writer.writerows(lista)
		myfile.close()
	except:
		print("Can't write fl0wmatrix.csv")

pprint.pprint(datasource)
