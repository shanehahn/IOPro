import os
import sys
import re
import fileinput
import shutil
import paramiko
from paramiko import SSHClient
from scp import SCPClient

#custom variables
username=''
password=''
IP_address=''
port=22
remote_aiopro_visualizer_location=''
######
server_info_file='server_info.txt'
command_file='remote_cmd.txt'
local_log_dir='log_dir'
remote_log_dir= ''
visualizer_files = ['aiopro_visualizer_win_run.sh', 'config.txt', 'aiopro_parser.py', 'info.py', 'visualizer.c']
log_files = ['aio_sys.txt', 'aio_sys2.txt', 'aio_mm.txt', 'aio_bio.txt', 'aio_scsi.txt', 'aio_ufs.txt', 'aio_fs.txt']

def file_exist_check(log_dir):
    bio=False
    mm=False
    scsi=False
    sys=False
    sys2=False
    ufs=False

    for current_dir, subdirs, current_dir_files in os.walk(log_dir):
        for current_dir_file in current_dir_files:
            if current_dir_file == "log_bio.txt":
                bio=True
            elif current_dir_file == "log_mm.txt":
                mm=True
            elif current_dir_file == "log_scsi.txt":
                scsi=True
            elif current_dir_file == "log_sys.txt":
                sys=True
            elif current_dir_file == "log_sys2.txt":
                sys2=True
            elif current_dir_file == "log_ufs.txt":
                ufs=True
                
    all_exist=True                
    if bio == False:
        print('log_bio.txt does not exist')
        all_exist=False
    if mm == False:
        print('log_mm.txt does not exist')
        all_exist=False
    if scsi == False:
        print('log_scsi.txt does not exist')
        all_exist=False
    if sys == False:
        print('log_sys.txt does not exist')
        all_exist=False
    if sys2 == False:
        print('log_sys2.txt does not exist')
        all_exist=False
    if ufs == False:
        print('log_ufs.txt does not exist')
        all_exist=False

    if all_exist == False:
        raise SystemExit
        
def scp_put_all(scp):
    #transfer log files
    for current_dir, subdirs, current_dir_files in os.walk(local_log_dir):
        for current_dir_file in current_dir_files:
            if current_dir_file in log_files:
                print('scp ' + current_dir_file + '...')
                local_log_file = local_log_dir + '\\' + current_dir_file
                print ('local_log_file: ' + local_log_file)
                remote_log_file = remote_log_dir + '/' + current_dir_file
                print ('remote_log_file:'+remote_log_file)
                scp.put(local_log_file, remote_log_file)
    #transfer visaulizer files
    for vs_file in visualizer_files:
        local_visualizer_file = os.path.dirname(os.path.realpath(__file__)) + '\\' + vs_file
        remote_visualizer_file = remote_log_dir + '/' + vs_file
        print('scp ' + vs_file + '...')
        print ('local_visualizer_file: ' + local_visualizer_file)
        print ('remote_visuzlier_file: ' + remote_visualizer_file)
        scp.put(local_visualizer_file, remote_visualizer_file)
    
def transfer_all_files():
    ssh_client=SSHClient()
    ssh_client.load_system_host_keys()
    ssh_client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh_client.connect(IP_address,port,username,password)
    scp=SCPClient(ssh_client.get_transport())
    scp_put_all(scp)

def write_command_file_to_make_dir(command_file):
    f=open(command_file, 'w')
    f.write('mkdir -p ' + remote_log_dir)
    f.close()
    
def write_command_file_to_run_visualizer(command_file):
    f=open(command_file, 'w')
    f.write('cd ' + remote_log_dir + '\n')
    run_script = visualizer_files[0]
    visualizer_config = visualizer_files[1]
    f.write('chmod +x ' + run_script + '\n')
    f.write('./' + run_script +  ' ' + visualizer_config + ' .' '\n')
    f.close()

def set_config():
    f=open(server_info_file, 'r')
    print ('###################################')
    while True:
        line=f.readline()
        if not line:
            break
        if 'username:' in line:
            split_list=re.split(':|\n',line)
            global username
            username=split_list[1]
            print ('username:'+username)
        elif 'password:' in line:
            split_list=re.split(':|\n',line)
            global password
            password=split_list[1]
            print ('password:'+password)
        elif 'IP_address:' in line:
            split_list=re.split(':|\n',line)
            global IP_address
            IP_address=split_list[1]
            print ('IP_address:'+IP_address)
        elif 'port:' in line:
            split_list=re.split(':|\n',line)
            global port
            port=int(split_list[1])
            print ('port:'+str(port))
        elif 'remote_aiopro_visualizer_location:' in line:
            split_list=re.split(':|\n',line)
            global remote_aiopro_visualizer_location
            remote_aiopro_visualizer_location=split_list[1]
            print ('remote_aiopro_visualizer_location:'+remote_aiopro_visualizer_location)
            global remote_log_dir
            remote_log_dir=remote_aiopro_visualizer_location
    print ('###################################')
    f.close()
 
def main():
    set_config()
    write_command_file_to_make_dir(command_file)
    os.system('putty -ssh -l ' + username + ' -pw ' + password + ' -P ' + str(port) + ' -m ' + command_file + ' ' + IP_address)
    transfer_all_files()
    write_command_file_to_run_visualizer(command_file)
    os.system('putty -ssh -2 -X -l ' + username + ' -pw ' + password + ' -P ' + str(port) + ' -m ' + command_file + ' ' + IP_address)

main()






