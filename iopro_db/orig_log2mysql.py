#!/usr/bin/python
#-*- coding: utf-8 -*-

import MySQLdb as mdb
import sys, getopt, os
import collections

def main(argv):
  inputfile = 'input.txt'
#  serveraddr = 'localhost'
  serveraddr = '147.46.241.65'
  fh = None
  errorfo = None

# Command line arugments
  try:
    opts, args = getopt.getopt(argv, "hi:s:", ["ifile=", "server="])
  except getopt.GetoptError:
    print sys.argv[0] + ' -i <inputfile> -s <serveraddr>'
    print 'Using default option -i input.txt -s 147.46.241.65'
    pass

  for opt, arg in opts:
    if opt == '-h':
      print sys.argv[0] + ' -i <inputfile> -s <serveraddr>'
      sys.exit()
    elif opt in ("-i", "--ifile"):
      inputfile = arg
    elif opt in ("-s", "--server"):
      serveraddr = arg

# MySQL DB connection
  try:
    con = mdb.connect(serveraddr, 'aiopro', 'sowkdgud', 'aiopro')
    cur = con.cursor()

    fh = open(inputfile)
    errorfile = inputfile + ".err"
    if os.path.isfile(errorfile):
      os.remove(errorfile)
    errorfo = open(errorfile, "w")

    if inputfile == "log_sys.txt":
      cur.execute("delete from aiopro.aio_log where id>0;")
      con.commit()
    
    Log = collections.namedtuple('Log', ['start_time_sec', 'start_time_nsec',
      'exec_time_nsec', 'uid', 'pid', 'tid', 'IO_layer', 'type', 'func_id',
      'func_id_2', 'fd', 'offset', 'size', 'inode_num', 'lba', 'filename'])
# INSERT Query
    add_log = ("INSERT INTO test "
               "(start_time_sec, start_time_nsec, exec_time_nsec, uid, pid, tid, "
               "IO_layer, type, func_id, func_id_2, fd, offset, size, inode_num, "
               "lba, filename) "
               "VALUES (%(start_time_sec)s, %(start_time_nsec)s, %(exec_time_nsec)s, "
               "%(uid)s, %(pid)s, %(tid)s, %(IO_layer)s, %(type)s, %(func_id)s, "
               "%(func_id_2)s, %(fd)s, %(offset)s, %(size)s, %(inode_num)s, %(lba)s, %(filename)s)"
              )

    while 1:
      try:
        line = fh.readline().rstrip()
      	if not line: break
        element = Log._make(line.split(' '))
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
	errorfo.write(line + '\n')

  except IOError, e:
    print "I/O error({0}): {1}".format(e.errno, e.strerror)
    sys.exit(1)
  except mdb.Error, e:
    print "Error %d: %s" % (e.args[0], e.args[1])
    sys.exit(1)

  finally:
    if con:
      con.close()
    if fh:
      fh.close()
    if errorfo:
      errorfo.close()

if __name__ == "__main__":
  main(sys.argv[1:])
