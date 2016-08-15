#!/usr/bin/python

from __future__ import print_function

import mysql.connector
from mysql.connector import errorcode
import os
import os.path
import sys
import argparse

class CrashDb(object):
	def __init__(self, db="crash",
		user="crash",
		password="crash",
		host="127.0.0.1"):
		self._user = user
		self._password = password
		self._host = host
		self._db = db
		self._connected = False
		
		self.__connect()

	def table_name(self):
		return "crash"

	def __connect(self):
		try:
			self._dbconn = mysql.connector.connect(user=self._user,
				password=self._password,
				host=self._host,
				database=self._db)
			self._connected = True
		except mysql.connector.Error as err:
			self._connected = False
			self._dbconn = None

	def connected(self):
		return self._connected

	def disconnect(self):
		self._dbconn.disconnect()
		self._connected = False
		self._dbconn = None

	def select(self, csid, group):
		select_query = ("select "
			"cbid, csid, original, team, grp, cbpath, filename, contents "
			"from " +
			self.table_name() +
			" where csid=%(csid)s and grp=%(grp)s")

		crashes = []
		try:
			cursor = self._dbconn.cursor()
			cursor.execute(select_query,
				{'grp': group, 'csid': csid}
			)
			for row in cursor:	
				crash = Crash()
				(crash.cbid,
				 crash.csid,
				 crash.original,
				 crash.team,
				 crash.group,
				 crash.cbpath,
				 crash.filename,
				 crash.contents) = row
				crashes.append(crash)

			cursor.close()
		except mysql.connector.errors.Error as err:
			pass

		return crashes

	def insert(self, crash):
		insert_query = ("insert into " + self.table_name() + " "
			"(cbid, csid, original, team, grp, cbpath, filename, contents) "
			" VALUES "
			"(%(cbid)s, %(csid)s, %(original)s, %(team)s, %(group)s, "
			"%(cbpath)s, %(filename)s, %(contents)s)")
		try:
			cursor = self._dbconn.cursor()
			cursor.execute(insert_query, 
			{'cbid': crash.cbid,
			 'csid': crash.csid,
			 'original': crash.original,
			 'team': crash.team,
			 'group': crash.group,
			 'cbpath': crash.cbpath,
			 'filename': crash.filename,
			 'contents': crash.contents
			})
			cursor.fetchone()
			cursor.close()
			self._dbconn.commit()
		except mysql.connector.errors.Error as err:
			return False

		return True

class Crash(object):
	#
	#
	#
	@staticmethod
	def from_file(cbid, csid, original, team, group, cbpath, f):

		filename = os.path.basename(f)

		crash = Crash()
		crash.cbid = cbid
		crash.csid = csid
		crash.original = original
		crash.team = team
		crash.group = group
		crash.cbpath = cbpath
		crash.filename = filename
		crash.contents = None

		with open(f, 'rb') as fd:
			crash.contents = fd.read();

		if crash.contents == None:
			return None

		return crash
	@staticmethod
	def from_directory(cbid, csid, original, team, group, cbpath, directory):
		crashes = []

		for f in os.listdir(directory):
			df = directory + "/" + f
			crashes.append(Crash.from_file(cbid,csid,original,team,group,cbpath,df))

		return crashes

	#
	#
	#
	@staticmethod
	def from_db(db, csid, group):
		if not db.connected():
			return None
		return db.select(csid, group)

	#
	#
	#
	def to_db(self, db):
		if not db.connected():
			return False
		return db.insert(self)

	#
	#
	#
	def to_file(self, directory="./"):
		written = False
		filename = directory + "/" + self.filename
		#
		# We need to make sure that the directory exists.
		try:
			os.makedirs(directory)
		except OSError as err:
			pass

		with open(filename, 'wb') as fd:
			written = True
			fd.write(bytes(self.contents))
		return written

	def __repr__(self):
		return ("cbid: %s," % self.cbid) + \
			("csid: %s," % self.csid) + \
			("original: %s," % self.original) + \
			("team: %s," % self.team) + \
			("group: %s," % self.group) + \
			("cbpath: %s," % self.cbpath) + \
			("filename: %s," % self.filename)

def unit_test():
	db = CrashDb()

	crash = Crash.from_file("cbid", "csid", True, "7", "2016-07-18-19-11-17.189944_afl", "path", "./sample_crashes/2016-07-18-19-11-17.189944_afl/id_large_no_special,type1,QEMU.min")
	if crash.to_db(db):
		print("Success: adding crash (from Crash.from_file) to database.")
	else:
		print("Error:   adding crash (from Crash.from_file) to database.")

	crashes = Crash.from_directory("cbid", "csid", True, "7", "2016-07-18-19-11-41.261319_afl", "path", "sample_crashes/2016-07-18-19-11-41.261319_afl")
	for crash in crashes:
		if crash.to_db(db):
			print("Success: adding crash (from Crash.from_directory) to database.")
		else:
			print("Error:   adding crash (from Crash.from_directory) to database.")

	crashes = Crash.from_db(db, "csid", "2016-07-18-19-11-41.261319_afl")
	if len(crashes)==4:
		print("Success: Retrieved correct number of entries from CrashDb.")
	else:
		print("Error:   Retrieved incorrect number of entries from CrashDb.")

	db.disconnect()

def configure_arguments():
	parser=argparse.ArgumentParser(description='Process crashes w.r.t. CrashDb.')
	sub_parsers=parser.add_subparsers(dest='command_name')

	read_parser=sub_parsers.add_parser("read", help="read")
	read_parser.add_argument('--group', help="Crash group", required=True)
	read_parser.add_argument('--csid', help="Crash CSID", required=True)
	read_parser.add_argument('--directory', help="Ouput directory", required=True)

	write_parser=sub_parsers.add_parser("write", help="write")
	write_parser.add_argument('--cbid', help="The CBID",
		required=True)
	write_parser.add_argument('--csid', help="The CSID",
		required=True)

	original_group=write_parser.add_mutually_exclusive_group(required=True)
	original_group.add_argument('--original', help="Original binary?",
		dest="original", action="store_true")
	original_group.add_argument('--not-original',help="Original binary?",
		dest="original", action="store_false")
	write_parser.set_defaults(original=True)

	write_parser.add_argument('--team', help="The team number", type=int,
		required=True)
	write_parser.add_argument('--group', help="Crash group",
		required=True)
	write_parser.add_argument('--cbpath', help="The CB path",
		required=True)
	write_parser.add_argument('--filename',help="Crash filename")
	write_parser.add_argument('--directory',help="Crash directory", nargs="+")
	return parser

if __name__=="__main__":
	#unit_test()
	#sys.exit(0)

	argument_parser = configure_arguments()
	args = argument_parser.parse_args()

	db = CrashDb()
	if args.command_name == 'read':
		crashes = []
		crashes = Crash.from_db(db, args.csid, args.group)
		for crash in crashes:
			if not crash.to_file(args.directory):
				print("Error: Writing crash to file failed.")
				break

	elif args.command_name == 'write':
		crashes = []
		if args.directory != None:
			# we are reading from a directory.
			for d in args.directory:
				crashes += Crash.from_directory(args.cbid,
					args.csid,
					args.original,
					args.team,
					args.group,
					args.cbpath,
					d)
		else:
			# we are reading from a file.
			crashes = [Crash.from_file(args.cbid,
				args.csid,
				args.original,
				args.team,
				args.group,
				args.cbpath,
				args.filename)]

		for crash in crashes:
			if not crash.to_db(db):
				print("Error: Inserting crash into CrashDb failed.")
				break
	db.disconnect()
