#!/usr/bin/env python 
import os, sys
import getopt
import operator
import copy
import math

'''
This script should be run after executing aiopro_parser.py
usage: python info.py -c [config_file] -d [log_dir]

Input
      config_file: user config file which specifies func_id and its attrbitute to visualize
      foramt:      LayerName | function_id | function_name | color
                  e.g., SystemCall | 400 | SYSCALL_DEFINE3(read) | dark green

      log_dir: A directory which contains log files

Output
      info files per @func_id
      output filename: info_400.txt, info_800.txt ...
      format: layer start_time end_time io_flag fd lba offset io_size rw filename

Note
      function depedent information
      - fsync/fdatasync: func_id 418, rw:0 => fsync, rw:1 => fdatasync
      - queue depth: func_id 921's fd
'''


filtered_log_files = ['aio_sys.txt', 'aio_sys2.txt', 
                      'aio_mm.txt', 'aio_bio.txt', 
                      'aio_ufs.txt','aio_fs.txt']

VCD_FILENAME = 'output.vcd'
GTKW_FILENAME = 'output.gtkw'
MODULE_NAME = 'AIOPro'
VISUALIZER_FILENAME = 'visualizer'
QUEUE_DEPTH_NAME = 'queue_depth' 
NUM_GROUPED_LINE_LIMIT = 15 #custom config variable
log_start_time = -1
log_end_time = -1
ascii_char_list = []

class _aiopro_line:
    def __init__(self, start_time_sec, start_time_nsec, exec_time_nsec,
                 uid, pid, tid, IO_layer, rw, func_id, io_flag, fd, offset,
                 size, inode_num, lba, filename, start_time, end_time):
        self.start_time_sec = start_time_sec
        self.start_time_nsec = start_time_nsec
        self.exec_time_nsec = exec_time_nsec
        self.uid = uid
        self.pid = pid
        self.tid = tid
        self.IO_layer = IO_layer
        self.rw = rw
        self.func_id = func_id
        self.io_flag = io_flag
        self.fd = fd
        self.offset = offset
        self.size = size
        self.inode_num = inode_num
        self.lba = lba
        self.filename = filename
        self.start_time = start_time
        self.end_time = end_time
                                
class _visualize_line:
    def __init__(self, layer, start_time, end_time, func_id, io_flag,
                 fd, offset, lba, io_size, rw, filename, color):
        self.layer = layer
        self.start_time = start_time
        self.end_time = end_time
        self.func_id = func_id
        self.io_flag = io_flag
        self.fd = fd
        self.offset = offset
        self.lba = lba
        self.io_size = io_size
        self.rw = rw
        self.filename = filename
        self.color = color

class _info_file:
    def __init__(self, func_id):
        self.func_id = func_id
        self.line_list = []
        self.layer_list = []
    def _add_line(self, line):
        self.line_list.append(line)
    def _add_layer(self, layer, filename):
        info_file_layer = _info_file_layer(layer, filename)
        self.layer_list.append(info_file_layer)
    def _add_layer_line_list(self, layer, line):
        for info_file_layer in self.layer_list:
            if info_file_layer.layer == layer:
                info_file_layer._add_line(line)
    def init_ascii_char(self, ascii_char):
        self.ascii_char = ascii_char

class _info_file_layer:
    def __init__(self, layer, filename):
        self.layer = layer
        self.filename = filename
        self.line_list = []
    def _add_line(self, line):
        self.line_list.append(line)
    
color_predefined_hash = {'dark green' : 4, 'blue' : 5, 'orange' : 2, 'purple' : 7, 'red' : 1, 'black' : 0}
#K: func_id V:color name
color_hash = {}
funcname_hash = {}
io_flag_hash = {0 : 'O_BUFFERED', 1 : 'O_SYNC', 2 : 'O_DIRECT', 3 : 'NO_FLAG'}
layer_name_hash = {} #key:func_id, value:func_name

def main(argv):
    global log_start_time
    global log_end_time
    global valid_info_file_list
    config_file = ''
    log_dir = ''
    log_file_path = ''
    if len(argv) != 4:
        print_usage()
        sys.exit(-1)
    opts, args = getopt.getopt(argv, ":c:d:f:")
    for opt, arg in opts:
        if opt == '-c':
            config_file = arg
        elif opt == '-d':
            log_dir = arg
        elif opt == '-f':
            log_file_path = arg

    if (config_file == '' or log_dir == '') and \
    (config_file == '' or log_file_path == ''):
        print_usage()
        sys.exit(-1)
    if log_dir != '' and log_file_path != '':
        print_usage()
        sys.exit(-1)
    if log_file_path != '': #file
        add_file_to_line_list(log_file_path)
        log_dir = os.path.split(log_file_path)[0]
    elif log_dir != '': #directory
        traverse_files(log_dir) #add all lines to @global_line_list

    init_ascii_char_list()
    #Now, we've got all the lines in @aiopro_line_list!
    #sort @aiopro_line_list by start_time
    aiopro_line_list.sort(key=operator.attrgetter('start_time'))
    #need to write vcd file
    log_start_time = aiopro_line_list[0].start_time
    log_end_time = aiopro_line_list[len(aiopro_line_list)-1].end_time

    #create a file per function
    #[config_file] is used to check if @func_id is valid
    #ignore @func_id which is not specified in [config_file]
    write_info_files(log_dir, config_file)
    
    #just make sure that appropriate @log_start_time & @log_end_time
    write_vcd_file(log_dir)

    #valid_info_file_list = sorted(valid_info_file_list, key = operator.attrgetter('func_id'))

    write_gtkw_file(log_dir, config_file)

    #write_run_script(log_dir)
    
QD_max_layer = -1
def init_QD_max_layer():
    global QD_max_layer
    for info_file in info_file_list:
        if info_file.func_id == 921:
            for line in info_file.line_list:
                if line.fd > QD_max_layer:
                    QD_max_layer = line.fd
                    
QD_max_layer_log2 = 0
def write_vcd_file(log_dir):
    global QD_max_layer_log2
    #get Max Queue Depth 
    init_QD_max_layer()
    f = open(os.path.join(log_dir,VCD_FILENAME), 'w')
    f.write('$date')
    f.write('\tWed Feb  3 15:15:46 2016\n')
    f.write('$end\n')
    f.write('$version\n')
    f.write('\tIcarus Verilog\n')
    f.write('$end\n')
    f.write('$timescale\n')
    f.write('\t1ns\n')
    f.write('$end\n')
    f.write('$scope module ' + MODULE_NAME + ' $end\n')
    for i in range(0, len(info_file_list)):
        info_file = info_file_list[i]
        ascii_char = ascii_char_list[i]
        info_file.init_ascii_char(ascii_char)
        f.write('$var wire ' + '1' + ' ' \
                + info_file.ascii_char + ' ' + ' F' + str(info_file.func_id) + ' $end\n')
    #QD
    QD_ascii_char = ascii_char_list[i+1]
    if QD_max_layer != -1:
            QD_max_layer_log2 = int(math.ceil(math.log(QD_max_layer,2)))
            binformat = '{0:0' + str(QD_max_layer_log2) + 'b}'
            f.write('$var wire ' + str(QD_max_layer_log2) + ' ' \
                    + QD_ascii_char + ' ' + QUEUE_DEPTH_NAME + ' $end\n')
                    
    f.write('$upscope $end\n')
    f.write('$enddefinitions $end\n')
    f.write('#' + str(log_start_time-1) + '\n')
    f.write('$dumpvars\n')
    #initialization START
    for i in range(0, len(info_file_list)):
        info_file = info_file_list[i]
        f.write('0' + info_file.ascii_char + '\n')
    #QD
    if QD_max_layer != -1:
        f.write('b' + str(binformat.format(0)) + ' ' + QD_ascii_char + '\n')
        f.write('$end\n')
    #initialization END
    #QD start
    if QD_max_layer != -1:
        for info_file in info_file_list:
            if info_file.func_id == 921:
                info_file.line_list.sort(key=operator.attrgetter('start_time'))
                for line in info_file.line_list:
                    f.write('#' + str(line.start_time) + '\n')
                    f.write('b' + str(binformat.format(line.fd)) + ' ' + QD_ascii_char + '\n')
                break
    #QD end
    #wrap up start
    for info_file in info_file_list:
        f.write('1 ' + info_file.ascii_char + '\n')
    #wrap up end
    f.close()
    
def init_ascii_char_list():
    global ascii_char_list
    start_num = 33
    end_num = 126
    for i in range(start_num, end_num + 1):
        ascii_char_list.append(str(unichr(i)))
                               
def write_gtkw_file(log_dir, config_file):
    vcd_file_path = os.path.join(log_dir, VCD_FILENAME)
    f = open(os.path.join(log_dir, GTKW_FILENAME), 'w')
    f.write('[*]\n')
    f.write('[*] GTKWave Aanalyer v3.3.32 (w)1999-2012 BSI\n')
    f.write('[*] Sun Feb 12 03:53:14 2012\n')
    f.write('[*]\n')
    f.write('[dumpfile] ' + '\"' + os.path.realpath(vcd_file_path) + '\"\n')
    f.write('[dumpfile_mtime] \"Thu Apirl  21 20:40:48 2011\"\n')
    f.write('[dumpfile_size] ' + str(os.path.getsize(os.path.realpath(vcd_file_path))) + '\n')
    f.write('[savefile] \"' + os.path.realpath(os.path.join(log_dir, GTKW_FILENAME)) + '\"\n')
    info_file_list.sort(key=operator.attrgetter('func_id'))
    for info_file in info_file_list:
        f.write('@c00200\n')
        f.write('-' + funcname_hash[info_file.func_id] + '\n')
        f.write('@10000022\n')
        for info_file_layer in info_file.layer_list:
            if len(info_file_layer.line_list) != 0:
                f.write('^<1 ' + os.path.realpath(VISUALIZER_FILENAME) + ' '
                        + os.path.realpath(os.path.join(log_dir, info_file_layer.filename +
                                                        ' ' + os.path.realpath(config_file) + '\n')))
                f.write('[color] ' + str(color_predefined_hash[color_hash[info_file.func_id]]) + '\n')
                f.write(MODULE_NAME + '.' + 'F' + str(info_file.func_id) + '\n')
        f.write('@200\n')
        f.write('@1401200\n')
        f.write('-group' + str(info_file.func_id) + '\n')
    #QD start
    f.write('@88023\n')
    f.write('[color] 6\n')
    f.write(MODULE_NAME + '.' + QUEUE_DEPTH_NAME + '[' + str(QD_max_layer_log2-1) + ':0]\n')
    f.write('@20000\n')
    f.write('-\n')
    f.write('-\n')
    f.write('-\n')
    f.write('-\n')
    f.write('-\n')
    f.write('-\n')
    f.write('-\n')
    f.write('-\n')
    #QD end
    f.close()
    
info_file_list = []
def write_info_files(log_dir, config_filepath):
    global info_file_list
    global color_hash
    global funcname_hash
    global layer_name_hash
    
    cf = open(config_filepath)
    #possible @func_id list specified in config_file
    possible_func_id_list=[]
    while True:
        line = cf.readline()
        if not line : break
        splitted_line_list = line.split('|')
        #remove space at the front and end
        for i in range(0, len(splitted_line_list)):
            splitted_line_list[i] = splitted_line_list[i].strip()
        layer_name = splitted_line_list[0]
        func_id = int(splitted_line_list[1])
        colorname = splitted_line_list[3]
        funcname = splitted_line_list[2]
        possible_func_id_list.append(func_id)
        color_hash[func_id] = colorname
        funcname_hash[func_id] = funcname
        layer_name_hash[func_id] = layer_name
    cf.close()

    #now write info files per @func_id
    #filename example: info_400_1.txt 
    ## of lines in a group are limited by @NUM_GROUPED_LINE_LIMIT - 1
    for aiopro_line in aiopro_line_list:
        if aiopro_line.func_id not in possible_func_id_list: 
            continue
        is_exist = False
        for info_file in info_file_list:
            if aiopro_line.func_id == info_file.func_id:
                info_file._add_line(aiopro_line)
                is_exist = True
                break
        if is_exist == False:
            info_file = _info_file(aiopro_line.func_id)
            info_file._add_line(aiopro_line)
            info_file_list.append(info_file)
    #doesn't need anymore
    del aiopro_line_list[:]
    write_info_files_per_func_id(log_dir)
            
def write_info_files_per_func_id(log_dir):
    #for each info_file,
    for info_file in info_file_list:
        #key function
        #limit by @NUM_GROUPED_LINE_LIMIT
        #it contains the end_time
        for i in range(0, NUM_GROUPED_LINE_LIMIT):
            filename = "info_" + str(info_file.func_id) + "_" + str(i) + ".txt"
            info_file._add_layer(i, filename)
        buf_line_list = []
        for line in info_file.line_list:
            if len(buf_line_list) == 0:
                buf_line_list.append(line)
                continue
            if line.start_time < buf_line_list[len(buf_line_list)-1].end_time: #check duplication
                #check if len(buf_line_list) exceeds LIMIT
                if len(buf_line_list) < NUM_GROUPED_LINE_LIMIT:
                    buf_line_list.append(line)
                else: #just ignore
                    continue
            else: #no duplication, flush buf & add line to buf
                for i in range(0, len(buf_line_list)):
                    info_file._add_layer_line_list(i, buf_line_list[i])
                del buf_line_list[:]
                buf_line_list.append(line)
        #last flush if data exists in buf_line_list
        for i in range(0, len(buf_line_list)):
            info_file._add_layer_line_list(i, buf_line_list[i])
        del buf_line_list[:]

    #now write!
    for info_file in info_file_list:
        for info_file_layer in info_file.layer_list:
            if len(info_file_layer.line_list) != 0:
                f = open(os.path.join(log_dir, info_file_layer.filename), 'w')
                f.write(layer_name_hash[info_file.func_id] + '\n')
                f.write(str(info_file.func_id) + '\n')
                for line in info_file_layer.line_list:
                    write_aiopro_line(f, line)
                f.close()

def write_aiopro_line(f, line):
    f.write(str(line.start_time) + ' ' + str(line.end_time) + ' ' + str(line.uid) + ' '
            + str(line.pid) + ' ' + str(line.tid) + ' ' + str(line.IO_layer) + ' ' \
            + line.rw + ' ' + str(line.func_id) + ' ' + io_flag_hash[line.io_flag] \
            + ' ' + str(line.fd) + ' ' + str(line.offset) + ' ' + str(line.size) + ' ' \
            + str(line.inode_num) + ' ' + str(line.lba) + ' ' + line.filename + '\n')
    
def get_max_layer(layer):
    for i in range(0, len(layer)):
        if layer[i] == -1:
            return i-1
    if len(temp_g_line_list) > 1:
        return NUM_GROUPED_LINE_LIMIT - 1
    print('Error Unexpeceed')
    sys.exit(0)
    
    
def print_usage():
    print("Usage: python " + sys.argv[0] + " -c [config_file] -d/-f [log_dir]/[log_file_path]")
    sys.stdout.flush()

aiopro_line_list = []
def add_file_to_line_list(filepath):
    global aiopro_line_list
    global line_id
    f = open(filepath)
    while True:
        line = f.readline()
        if not line: break
        fit_line = fit_line_length(line)
        start_time_sec = int(fit_line[0])
        start_time_nsec = int(fit_line[1])
        exec_time_nsec = int(fit_line[2])
        uid = int(fit_line[3])
        pid = int(fit_line[4])
        tid = int(fit_line[5])
        IO_layer = int(fit_line[6])
        rw = 'read' if int(fit_line[7]) == 0 else 'write'
        func_id = int(fit_line[8])
        io_flag = int(fit_line[9])
        fd = int(fit_line[10])
        offset = int(fit_line[11])
        size = int(fit_line[12])
        inode_num = int(fit_line[13])
        lba = int(fit_line[14])
        filename = fit_line[15]
        start_time = start_time_sec * 1000000000 + start_time_nsec #in nano sec
        end_time = start_time + exec_time_nsec
        aiopro_line = _aiopro_line(start_time_sec, start_time_nsec, exec_time_nsec, uid, pid, tid,
                      IO_layer, rw, func_id, io_flag, fd, offset, size, inode_num,
                      lba, filename, start_time, end_time)
        aiopro_line_list.append(aiopro_line)
    f.close()
    
def fit_line_length(line):
    line_split_list = line.split()
    expected_num_element = 16
    if len(line_split_list) > expected_num_element: #this should contain filename which contains ' '
        #append filename (expected_num_element-1 ~ (len(list) - 1))
        for i in range(expected_num_element, len(line_split_list)): #remove space in filename
            line_split_list[expected_num_element-1] = line_split_list[expected_num_element-1] + line_split_list[i]
        #shrink the size to 16
        for i in range(len(line_split_list) - 1, expected_num_element-1, -1):
            del line_split_list[i]
    return line_split_list

def traverse_files(log_dir):
    for current_dir, subdirs, current_dir_files in os.walk(log_dir):
        for current_dir_file in current_dir_files:
            if current_dir_file not in filtered_log_files:
                continue
            print ('adding lines of ' + current_dir_file + ' ...')
            sys.stdout.flush()
            add_file_to_line_list(os.path.join(current_dir, current_dir_file))

def write_run_script(log_dir):
    f = open(os.path.join(log_dir, 'run_gtkwave.sh'), 'w')
    f.write('while [ 1 ]\n')
    f.write('do\n')
    f.write('\tgtkwave output.gtkw\n')
    f.write('done\n')
    f.close()
    os.system('chmod +x ' + os.path.join(log_dir, 'run_gtkwave.sh'))
    
main(sys.argv[1:])
