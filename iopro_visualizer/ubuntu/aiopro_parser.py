import os
import sys
import re
import string
import getopt
from operator import itemgetter

### to caculate the exception rate
num_total_lines = 0
num_err_lines = 0
###
log_files = ['log_sys.txt', 'log_sys2.txt', 'log_mm.txt', 'log_bio.txt', 'log_ufs.txt', 'log_fs.txt']
se_file = 'log_se.txt'
log_start_time_sec = -1
log_end_time_sec = -1

target_uid = None
target_filename = None
target_type = None

def main(argv):
    global target_uid
    global target_filename
    global target_type
    if len(argv) > 8 or len(argv) < 2:
        print_usage()
        sys.exit(-1)
    log_dir = None
    opts, args = getopt.getopt(argv, ':u:f:t:d:')
    for opt, arg in opts:
        if opt == '-u':
            target_uid = int(arg)
        elif opt == '-f':
            target_filename = arg
        elif opt == '-t':
            target_type = arg
        elif opt == '-d':
            log_dir = arg
    if target_uid != None:
        print('target_uid:' + str(target_uid))
    if target_filename != None:
        print('target_filename:' + target_filename)
    if target_type != None:
        print('target_type:' + target_type)
        if target_type != 'r' and target_type != 'w':
            print_usage()
            sys.exit(-1)
    if log_dir == None:
        print('[log_dir] must be specified')
        sys.exit(-1)
    set_log_start_end_time(log_dir)
    traverse_log_files(log_dir, target_uid, target_filename, target_type)
    report()
    #remove_original_files(log_dir)
    
def remove_original_files(log_dir):
    for current_dir, subdirs, current_dir_files in os.walk(log_dir):
        for current_dir_file in current_dir_files:
            if current_dir_file in log_files or current_dir_file == se_file:
                os.remove(os.path.join(current_dir, current_dir_file))
                
def set_log_start_end_time(log_dir):
    global log_start_time_sec
    global log_end_time_sec
    target_file_path = None
    for current_dir, subdirs, current_dir_files in os.walk(log_dir):
        for current_dir_file in current_dir_files:
            if current_dir_file == se_file:
                target_file_path = os.path.join(current_dir, current_dir_file)
    if target_file_path == None:
        print('[ERR]' + se_file + ' doest not exist')
        sys.exit(-1)
    #0 denotes start_time, 1 denotes end_time
    with open(target_file_path) as f:
        for line in f:
            start_end, time_sec = line.split()
            time_sec = int(time_sec)
            if start_end == '0' and time_sec < log_start_time_sec:
                log_start_time_sec = time_sec
            if start_end == '1' and time_sec > log_end_time_sec:
                log_end_time_sec = time_sec
    print('log_start_time_sec:' + str(log_start_time_sec))
    print('log_end_time_sec:' + str(log_end_time_sec))
    f.close()

def filter_out_with_key(uid, type, filename):
    converted_target_type = '0' if target_type is 'r' else '1'
    if (target_uid == None or target_uid == uid) \
       and (target_filename == None or target_filename == filename)\
       and (target_type == None or converted_target_type == type):
        return True
    return False

def report():
    #print("num_total_lines:" + str(num_total_lines))
    if num_total_lines != 0:
        print("filtered log rate:" + str(round((num_out_of_range_lines/float(num_total_lines)*100),3)) + "%")
    else:
        print ("filtered log rate:" + "0.000%")
    sys.stdout.flush()

def traverse_log_files(log_dir, target_uid, target_filename, target_type):
    for current_dir, subdirs, current_dir_files in os.walk(log_dir):
        for current_dir_file in current_dir_files:
            if current_dir_file in log_files:
                print("parsing " + current_dir_file + "...")
                sys.stdout.flush()
                filter_out_file(current_dir, current_dir_file)

                
num_out_of_range_lines = 0
zero_count = 0
def filter_out_file(target_dir, target_file):
    global num_total_lines
    global num_out_of_range_lines
    global zero_count
    global num_err_lines
    #e.g., @target_file = log.1/log_bio.txt  @filtered_file = log.1/aio_bio.txt
    temp_list = target_file.split('_')
    parsed_file = 'aio_' + temp_list[1]
    filtered_file = 'filtered_' + temp_list[1]
    err_file = 'err_' + temp_list[1]
    #print ('filtered_file:' + filtered_file + '\n')
    target_file_path = os.path.join(target_dir, target_file)
    parsed_file_path = os.path.join(target_dir, parsed_file)
    filtered_file_path = os.path.join(target_dir, filtered_file)
    err_file_path = os.path.join(target_dir, err_file)
    f = open(target_file_path)
    pf = open(parsed_file_path, 'w')
    ff = open(filtered_file_path, 'w')
    print('parsed_file:' + os.path.join(target_dir, parsed_file) + '\n')
    sys.stdout.flush()
    line_list = fit_line_length(f)
    parsed_line_list = []
    prev_410_offset = None
    for line in line_list:
        start_time_sec = line[0] 
        start_time_nsec = line[1]
        exec_time_nsec = line[2] 
        uid = line[3] 
        pid = line[4] 
        tid = line[5] 
        IO_layer = line[6] 
        type = line[7]
        func_id = line[8] 
        #func_id_2 = line[9]
        io_flag = line[9]
        fd = line[10]
        offset =line[11] 
        size = line[12] 
        inode_num =line[13] 
        lba = line[14]
        '''
        if 'jpg' in line[15]:
            line[15] = 'mtptemp.jpg'
        '''
        filename = line[15]

        '''
        if func_id == '410':
            temp = offset
            if prev_410_offset != None:
                line[11] = prev_410_offset
            prev_410_offset = temp
        '''
        end_time_sec = int(start_time_sec) + int(start_time_nsec)/1000000000.0 + int(exec_time_nsec)/1000000000.0
        
        #out_of_range lines are only writes to filtered file
        if (((int(start_time_sec) < log_start_time_sec)
           or (int(end_time_sec) > log_end_time_sec)) and func_id != '418'):
            num_out_of_range_lines = num_out_of_range_lines + 1
            for i in range(0, len(line)): 
                if i == len(line) - 1:
                    ff.write(line[i] + '\n')
                else:
                    ff.write(line[i] + ' ')
        elif (((int(lba) == 0) or (int(size) == 0)) and func_id != '418'):
            zero_count = zero_count + 1
        elif not ((int(exec_time_nsec) < 0) or (int(exec_time_nsec) > 999999999999 )
                or (int(start_time_sec) < 0 ) or (int(start_time_sec) > 999999 )
                or (int(start_time_nsec) < 0 ) or (int(start_time_nsec) > 999999999 )
                or (int(offset) < 0) 
                or (int(size) < 0) 
                or (int(inode_num) < 0) 
                or (int(lba) < 0) 
                or (int(func_id) < 400) or (int(func_id) > 999)
                or (int(io_flag) < 0) or (int(io_flag) > 3)
                or (int(type) < 0) or (int(type) > 1)
                or (int(IO_layer) < 4) or (int(IO_layer) > 9)
                or (int(fd) < 0) or (int(fd) > 9999)
                or (int(uid) < 0)
                or (int(pid) < 0)
                or (int(tid) < 0)
                or isbinary(filename)):
            if filter_out_with_key(int(uid), type, filename) == True:
                parsed_line_list.append(line)
        else:
            num_err_lines = num_err_lines + 1
            continue
            for i in range(0, len(line)):
                if i == len(line) - 1:
                    ef.write(line[i] + '\n')
                else:
                    ef.write(line[i] + ' ')
        num_total_lines = num_total_lines + 1

    #sort by start time
    sorted_parsed_line_list = sorted(parsed_line_list, key=itemgetter(0,1))
    #to fix VFS size
    prev_size_508 = 0
    prev_size_501 = 0
    for parsed_line in sorted_parsed_line_list:
        if int(parsed_line[8]) == 508:
            temp = int(parsed_line[12])
            parsed_line[12] = str(int(parsed_line[12]) - prev_size_508)
            prev_size_508 = temp
        elif int(parsed_line[8]) == 501:
            temp = int(parsed_line[12])
            parsed_line[12] = str(int(parsed_line[12]) - prev_size_501)
            prev_size_501 = temp
        for i in range(0, len(parsed_line)):
            member = parsed_line[i]
            if i == len(parsed_line) - 1:
                pf.write(member + '\n')
            else:
                pf.write(member + ' ')
    ff.close()
    if os.path.getsize(filtered_file_path) == 0:
        os.remove(filtered_file_path)
    pf.close()
    f.close()
    
def fit_line_length(f):
    global num_total_lines
    line_list = []
    ###For Debug
    line_count = 0
    ###
    while True:
        line = f.readline()
        if not line: break
        line_count = line_count + 1
        #split line by ' '
        line_split_list = line.split()
        #expected # of element
        e_n = 16
        if len(line_split_list) > e_n: #this should contain filename which contains ' ' 
        #append filename (e_n-1 ~ (len(list) - 1))
            for i in range(e_n, len(line_split_list)):
                line_split_list[e_n-1] = line_split_list[e_n-1] + ' ' + line_split_list[i]
            #shrink the size to 16
            for i in range(len(line_split_list) - 1, e_n-1, -1):
                del line_split_list[i]
        elif len(line_split_list) < e_n: 
            #print("Warning: # of space is smaller than 15! (line " + str(line_count) + ")" + " IGNORE")
            #ef.write(line)
            num_total_lines = num_total_lines + 1
            continue
        line_list.append(line_split_list)
    return line_list
    
def isbinary(s):
    text_characters="".join(map(chr, range(32,127)) + list("\n\r\t\b"))
    _null_trans=string.maketrans("","")

    # an "empty" string is "text" (arbitrary but reasonable choice):
    #if not s:
        #return True
    # if s contains any null, it's not text:
    if "\0" in s:
        return True
    # Get the substring of s made up of non-text characters
    t = s.translate(_null_trans, text_characters)
    if float(len(t))/float(len(s)) > 0.05:
        return True
    return False
    # s is 'text' if less than 30% of its characters are non-text ones:
    #return len(t)/len(s) <= threshold

def print_usage():
    print("usage: " + sys.argv[0] + " -u [uid] -f \"[filename]\" -t [r/w] -d [log_dir]")
    sys.stdout.flush()

main(sys.argv[1:])
