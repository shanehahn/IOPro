#!/usr/bin/python
#-*- coding: utf-8 -*-

import MySQLdb as mdb
import sys, getopt, os
import collections

filtered_log_files = ['aio_sys.txt', 'aio_sys2.txt',
                      'aio_mm.txt', 'aio_bio.txt',
                      'aio_scsi.txt', 'aio_ufs.txt']
log_dir = ''
serveraddr = '147.46.241.65'
database = 'aiopro'
table = 'test'
user = 'aiopro'
password = 'sowkdgud'

def main(argv):
  global log_dir
  global serveraddr
  global database
  global table
  global user
  global password
  fh = None
  # Command line arugments
  try:
    opts, args = getopt.getopt(argv, ":l:s:d:t:u:p:", ["LogDir=", "ServerAddr=", "Database=", "Table=", "User=", "Password="])
  except getopt.GetoptError, e:
    print(e)
    print_usage()
    sys.exit(-1)
  for opt, arg in opts:
    if opt in ("-l", "--LogDir"):
      log_dir = arg
    elif opt in ("-s", "--ServerAddr"):
      serveraddr = arg
    elif opt in ("-d", "--Database"):
      database = arg
    elif opt in ("-t", "--Table"):
      table = arg
    elif opt in ("-u", "--User"):
      user = arg
    elif opt in ("-p", "--Password"):
      password = arg

  if log_dir == '' or serveraddr == '' or database == '' \
     or table == '' or user == '' or password == '':
    print_usage()
    sys.exit(-1)
  print('######################################################')
  print("@LogDir: " + log_dir)
  print("@ServerAddr: " + serveraddr)
  print("@Database: " + database)
  print("@Table: " + table)
  print("@User: " + user)
  print("@Password: " + password)
  print('######################################################')
  try:
    # MySQL DB connection
    con = mdb.connect(serveraddr, user, password, database)
    cur = con.cursor()
    if checkTableExists(con, cur, table) == True:
      print(table + " AlREADY exists")
      print('TRUNCATE ' + table)
      cur.execute('TRUNCATE TABLE ' + table)
    else:
      print(table + "NOT exists")
      print('CREATE ' + table)
      cur.execute('CREATE TABLE ' + table + ' LIKE andro1')
    con.commit()
    traverse_files(con, cur)
  except IOError, e:
    print "I/O error({0}): {1}".format(e.errno, e.strerror)
    sys.exit(-1)
  except mdb.Error, e:
    print "Error %d: %s" % (e.args[0], e.args[1])
    sys.exit(-1)
  finally:
    if con:
      con.close()
    if fh:
      fh.close()

def traverse_files(con, cur):
  for current_dir, subdirs, current_dir_files in os.walk(log_dir):
    for current_dir_file in current_dir_files:
      if current_dir_file not in filtered_log_files:
        continue
      print ('inserting lines of '
             + os.path.join(current_dir, current_dir_file) + ' to ' + database + '.' + table + ' ...')
      insert_file_to_mysql_server(os.path.join(current_dir, current_dir_file), con ,cur)

def insert_file_to_mysql_server(inputfile, con, cur):
  fh = open(inputfile)
  Log = collections.namedtuple('Log', ['start_time_sec', 'start_time_nsec',
      'exec_time_nsec', 'uid', 'pid', 'tid', 'IO_layer', 'type', 'func_id',
      'func_id_2', 'fd', 'offset', 'size', 'inode_num', 'lba', 'filename'])
    # INSERT Query
  add_log = ("INSERT INTO " + table + " " + 
             "(start_time_sec, start_time_nsec, exec_time_nsec, uid, pid, tid, "
             "IO_layer, type, func_id, func_id_2, fd, offset, size, inode_num, "
             "lba, filename) "
             "VALUES (%(start_time_sec)s, %(start_time_nsec)s, %(exec_time_nsec)s, "
             "%(uid)s, %(pid)s, %(tid)s, %(IO_layer)s, %(type)s, %(func_id)s, "
             "%(func_id_2)s, %(fd)s, %(offset)s, %(size)s, %(inode_num)s, %(lba)s, %(filename)s)"
  )
  while True:
    try:
      line = fh.readline().rstrip()
      if not line: break
      #element = Log._make(line.split(' '))
      element = Log._make(fit_line_length(line))
      if not ((int(element.exec_time_nsec) < 1) 
              or (int(element.start_time_sec)< 1 )
              or (int(element.start_time_nsec)< 0 ) or (int(element.start_time_nsec) > 999999999 )
              or (int(element.offset) < 0) 
              or (int(element.size) < 0) 
              or (int(element.inode_num) < 0) 
              or (int(element.lba) < 0)
              or (int(element.func_id) < 0) 	or (int(element.func_id) > 999) 
              or (int(element.func_id_2) < 0) 
              or (int(element.type) < 0) or (int(element.type) > 7) 
              or (int(element.IO_layer) < 4) or (int(element.IO_layer) > 9) 
              or (int(element.fd) < 0)
              or (int(element.uid) < 0) or (int(element.pid) < 0) or (int(element.tid) < 0)):
          # Execute add_log query with element as an option dictionary
        cur.execute(add_log, element._asdict())
        con.commit() # Commit needed
    except (TypeError, ValueError) as e:
      print str(e)
      print line

def fit_line_length(line):
    line_split_list = line.split()
    expected_num_element = 16
    if len(line_split_list) > expected_num_element: #this should contain filename which contains ' '
        #append filename (expected_num_element-1 ~ (len(list) - 1))
        for i in range(expected_num_element, len(line_split_list)):
            line_split_list[expected_num_element-1] = line_split_list[expected_num_element-1] + ' ' + line_split_list[i]
        #shrink the size to 16                                                                                                                 
        for i in range(len(line_split_list) - 1, expected_num_element-1, -1):
            del line_split_list[i]
    return line_split_list

def print_usage():
  print (sys.argv[0] + ' -l <LogDir> -s <ServerAddr> -d <Database> -t <Table> -u <User> -p <Password>')
  print ('Using default option -l <LogDir> -s 147.46.241.65 -d aiopro -t test -u aiopro -p sowkdgud')

def checkTableExists(con, cur, table):
  cur.execute('SHOW TABLES LIKE \'' + table + '\'')
  con.commit()
  result = cur.fetchone()
  if result:
    return True
  return False

if __name__ == "__main__":
  main(sys.argv[1:])
