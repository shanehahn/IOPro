import os,sys
import shutil

def main(argv):
    if len(argv) != 1:
        print_usage()
        sys.exit(-1)
    log_dir = argv[0]
    adb_pull(log_dir)
    file_exist_check(log_dir)

def print_usage():
    print(sys.argv[0] + ' [log_dir]')
    
def file_exist_check(log_dir):
    bio = False
    mm = False
    sys = False
    sys2 = False
    ufs = False
    se = False
    fs = False
    for current_dir, subdirs, current_dir_files in os.walk(log_dir):
        for current_dir_file in current_dir_files:
            if current_dir_file == "log_bio.txt":
                bio = True
            elif current_dir_file == "log_mm.txt":
                mm = True
            elif current_dir_file == "log_sys.txt":
                sys = True
            elif current_dir_file == "log_sys2.txt":
                sys2 = True
            elif current_dir_file == "log_ufs.txt":
                ufs = True
            elif current_dir_file == "log_se.txt":
                se = True
            elif current_dir_file == "log_fs.txt":
                fs = True
    all_exist=True                
    if bio == False:
        print('log_bio.txt does not exist')
        all_exist = False
    if mm == False:
        print('log_mm.txt does not exist')
        all_exist = False
    if sys == False:
        print('log_sys.txt does not exist')
        all_exist = False
    if sys2 == False:
        print('log_sys2.txt does not exist')
        all_exist = False
    if ufs == False:
        print('log_ufs.txt does not exist')
        all_exist = False
    if se == False:
        print('log_se.txt does not exist')
        all_exist = False
    if fs == False:
        print('log_fs.txt does not exist')
        all_exist = False
    if all_exist == False:
        raise SystemExit

def adb_pull(log_dir):
    if os.path.isdir(log_dir):
        shutil.rmtree(log_dir)

    os.makedirs(log_dir)
    print('adb pull /sdcard/test/log_bio.txt ' + log_dir + '/log_bio.txt ...')
    os.system('adb pull /sdcard/test/log_bio.txt ' + log_dir)
        
    print('adb pull /sdcard/test/log_mm.txt ' + log_dir + '/log_mm.txt ...')
    os.system('adb pull /sdcard/test/log_mm.txt ' + log_dir)
    
    print('adb pull /sdcard/test/log_sys.txt ' + log_dir + '/log_sys.txt ...')
    os.system('adb pull /sdcard/test/log_sys.txt ' + log_dir)
    
    print('adb pull /sdcard/test/log_sys2.txt ' + log_dir + '/log_sys2.txt ...')
    os.system('adb pull /sdcard/test/log_sys2.txt ' + log_dir)

    print('adb pull /sdcard/test/log_ufs.txt ' + log_dir + '/log_ufs.txt ...')
    os.system('adb pull /sdcard/test/log_ufs.txt ' + log_dir)

    print('adb pull /sdcard/test/log_se.txt ' + log_dir + '/log_se.txt ...')
    os.system('adb pull /sdcard/test/log_se.txt ' + log_dir)

    print('adb pull /sdcard/test/log_fs.txt ' + log_dir + '/log_fs.txt ...')
    os.system('adb pull /sdcard/test/log_fs.txt ' + log_dir)

main(sys.argv[1:])
