/*
 *  linux/fs/read_write.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <linux/slab.h> 
#include <linux/stat.h>
#include <linux/fcntl.h>
#include <linux/file.h>
#include <linux/uio.h>
#include <linux/aio.h>
#include <linux/fsnotify.h>
#include <linux/security.h>
#include <linux/export.h>
#include <linux/syscalls.h>
#include <linux/pagemap.h>
#include <linux/splice.h>
#include <linux/compat.h>
#include "internal.h"

#include <asm/uaccess.h>
#include <asm/unistd.h>
////////////////////////////////////aiopro part for logging file information
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>

static struct aiopro_file* file_info;

void init_file_info(void){
	unsigned int i;

	printk("{IOPro} Initializing file information... ");
	file_info = (struct aiopro_file*) vmalloc(sizeof(struct aiopro_file));
	file_info->ino_no = (unsigned long*)vmalloc(AIOFILE * sizeof(unsigned long));

	file_info->uid = (int*)vmalloc(AIOFILE * sizeof(int));
	file_info->pid = (int*)vmalloc(AIOFILE * sizeof(int));
	file_info->tid = (int*)vmalloc(AIOFILE * sizeof(int));

	file_info->size = 0;

	for(i=0 ; i<AIOFILE ; i++)
		file_info->ino_no[i] = 7;
		
	printk("Done\n");
}EXPORT_SYMBOL(init_file_info);

int last_finfo=0;

int add_aiopro_finfo(int type, char *fname, int uid, int pid, int tid, sector_t lba, size_t size, unsigned long ino_no){
	unsigned int i = 0, flag = 0;

	if(ino_no < 0 || ino_no > 5000000000 || strlen(fname) < 1)
		return 1;

	//first check if there is aleady in queue
	for( i = 0 ; i < file_info->size ;  i++){
		if(file_info->ino_no[i] == ino_no)
			break;
		if(strcmp(file_info->fname[i], fname) == 0)
			break;
	}

	//if there is nothing matched, rise flag
	if(i == file_info->size && file_info->ino_no[i] != ino_no)
		flag = 1;

	//if the size becomes larger than AIOFILE, go first
	if( file_info->size == AIOFILE && file_info->ino_no[AIOFILE] != ino_no){
		i = last_finfo;
		last_finfo++;
		if(last_finfo > 999)
			last_finfo = 0;
		if(last_finfo == 1)
			printk("{IOPro} ADD file info over the maximum size (%d)\n",AIOFILE);
	}

	if(uid > -1 && pid > -1 && tid > -1){
		file_info->uid[i] = uid;
		file_info->pid[i] = pid;
		file_info->tid[i] = tid;
		file_info->ino_no[i] = ino_no;
		strcpy(file_info->fname[i], fname);
	}else return 1;
		

	if(flag == 1) //add new file info
		file_info->size = i+1;
	
	return 1;
}

unsigned int get_file_info(sector_t lba, unsigned long ino_no){ // well, current version does not provide lba matching
	unsigned int i;
	if(file_info->size > 0)
		for( i = 0 ; i < file_info->size ; i++)
			if( ino_no == file_info->ino_no[i] )
				return i;
	return 9999;
}EXPORT_SYMBOL(get_file_info);

struct aiopro_file *get_finfo(void){
	return file_info; 
}EXPORT_SYMBOL(get_finfo);

int print_finfo(void){
	int i;
	for( i=0 ; i < file_info->size ; i++)
		printk("{IOPro}[FileInfo] name [%s] ino [%lu] uptid [%d:%d:%d] \n",
			file_info->fname[i], file_info->ino_no[i], file_info->uid[i], file_info->pid[i], file_info->tid[i]);
	return 1;
}EXPORT_SYMBOL(print_finfo);


/////////////////////////////////////aiopro part for system call
#ifdef aiopro_sys
#include <linux/time.h>


uint64_t sys_err = 0, sys2_err=0;
int64_t sys_size = -1, sys2_size = -1;
static int sys_flag = 0, sys2_flag = 0, sys_start = 0, flag1=0, flag2=0;
static struct aiopro_log* sys_log;
static struct aiopro_log* sys2_log;

uint64_t get_sys_err(int type){
	if( type == 0 )
		return sys_err;
	else
		return sys2_err;
}EXPORT_SYMBOL(get_sys_err);

void set_sys_size(int type, int64_t target){
	if( type == 0 ){
		sys_size = target;
		sys_log->size = target;
	}
	else{
		sys2_size = target;
		sys2_log->size = target;
	}
}EXPORT_SYMBOL(set_sys_size);

int64_t get_sys_size(int type){
	if(type == 0)
		return sys_size;
	else
		return sys2_size;
}EXPORT_SYMBOL(get_sys_size);

atomic_t aios1, aios2;
unsigned int s1_me, s2_me;

void init_sys_log(int type){
	int i;
printk("{IOPro}[fs/read_write.c] Initialize structure for SYStem call [%d] logging ... ", type);
	if(type == 0){
	sys_log = (struct aiopro_log *) vmalloc(sizeof(struct aiopro_log));
	sys_size = 0;
	sys_log->size = sys_size;
	sys_log->func = (unsigned int*)vmalloc(AIOMAX * sizeof(unsigned int));
	sys_log->fd = (unsigned int*)vmalloc(AIOMAX * sizeof(unsigned int));
	sys_log->flag = (int*)vmalloc(AIOMAX * sizeof(int));
	sys_log->ino_no = (unsigned long*)vmalloc(AIOMAX * sizeof(unsigned long));
	sys_log->count = (size_t*)vmalloc(AIOMAX * sizeof(size_t));
	sys_log->pos = (loff_t*)vmalloc(AIOMAX * sizeof(loff_t));
	sys_log->uuid = (int*)vmalloc(AIOMAX * sizeof(int));
	sys_log->pid = (int*)vmalloc(AIOMAX * sizeof(int));
	sys_log->tid = (int*)vmalloc(AIOMAX * sizeof(int));
//	sys_log->lba = (sector_t*)vmalloc(AIOMAX * sizeof(sector_t));
	sys_log->st_sec = (int*)vmalloc(AIOMAX * sizeof(int));
	sys_log->st_nsec = (long*)vmalloc(AIOMAX * sizeof(long));
	sys_log->itv_sec = (int*)vmalloc(AIOMAX * sizeof(int));
	sys_log->itv_nsec = (long*)vmalloc(AIOMAX * sizeof(long));

	atomic_set(&aios1, 0);
	s1_me = 0;

	for(i=0 ; i< AIOMAX; i++){
		sys_log->st_sec[i] = 0;
		sys_log->st_nsec[i] = 0;
		sys_log->itv_sec[i] = 0;
		sys_log->itv_nsec[i] = 0;
	}

	}
	else{
	sys2_log = (struct aiopro_log *) vmalloc(sizeof(struct aiopro_log));
	sys2_size = 0;
	sys2_log->size = sys2_size;
	sys2_log->func = (unsigned int*)vmalloc(AIOMAX * sizeof(unsigned int));
	sys2_log->fd = (unsigned int*)vmalloc(AIOMAX * sizeof(unsigned int));
	sys2_log->flag = (int*)vmalloc(AIOMAX * sizeof(int));
	sys2_log->ino_no = (unsigned long*)vmalloc(AIOMAX * sizeof(unsigned long));
	sys2_log->count = (size_t*)vmalloc(AIOMAX * sizeof(size_t));
	sys2_log->pos = (loff_t*)vmalloc(AIOMAX * sizeof(loff_t));
	sys2_log->uuid = (int*)vmalloc(AIOMAX * sizeof(int));
	sys2_log->pid = (int*)vmalloc(AIOMAX * sizeof(int));
	sys2_log->tid = (int*)vmalloc(AIOMAX * sizeof(int));
//	sys2_log->lba = (sector_t*)vmalloc(AIOMAX * sizeof(sector_t));
	sys2_log->st_sec = (int*)vmalloc(AIOMAX * sizeof(int));
	sys2_log->st_nsec = (long*)vmalloc(AIOMAX * sizeof(long));
	sys2_log->itv_sec = (int*)vmalloc(AIOMAX * sizeof(int));
	sys2_log->itv_nsec = (long*)vmalloc(AIOMAX * sizeof(long));

	atomic_set(&aios2, 0);
	s2_me = 0;


	for(i=0 ; i< AIOMAX; i++){
		sys2_log->st_sec[i] = 0;
		sys2_log->st_nsec[i] = 0;
		sys2_log->itv_sec[i] = 0;
		sys2_log->itv_nsec[i] = 0;
	}

	}
flag1=0;
flag2=0;
printk("Done\n");
}EXPORT_SYMBOL(init_sys_log);

static int set_sys_log(struct timespec st_time, struct timespec cur_time, unsigned int func, unsigned int fd, struct file *file, size_t count, loff_t pos, int uid, int pid, int tid){
	//logging after init, fd > 0 and size > 0
	if(sys_size != -1 && file!= NULL){  

	//at switch mode, logging only when aiopro debug var is 1
#ifdef aiopro_switch
		if(aiopro_init() != 1 && aiopro_init() != 3){
			if( (aiopro_init() == 2||aiopro_init() == 4) && flag1 == 0){
				set_se_log(1);
				printk("{IOPro}[SYS/READ] Stop Logging...[%lld](%u)\n",sys_size,s1_me);
				flag1 = 1;
			}
			return 1;
		}
		else{
			if(flag1 == 1){
				set_se_log(0);
				set_sys_size(0,0);
				s1_me = 0;
				printk("{IOPro}[SYS/READ] Restart Logging...[%lld]\n",sys_size);
				flag1 = 0;
			}
		}
#endif

		// within predefined size
		if(sys_size < AIOMAX){  
			struct address_space *mapping;
			struct inode *inode;

			int tflag, tuid, tpid, ttid, tcsec, tisec;//, tempv, alimit;
			long tcnsec, tinsec;
			unsigned long tino;
			char tfname[100], tdev[32];	

			//init start
			if(sys_start == 0){ printk("{IOPro}[fs/read_write.c] Start system call logging.\n"); sys_start=1; set_se_log(0);}

			// get inode info
			if(file->f_mapping != NULL){
				mapping = file->f_mapping;
				if(mapping->host != NULL){
					inode = mapping->host; 
					
					//only if target dev is SCSI block dev which we look for -- looking for inode number 
					tino = inode->i_ino;

					if(&file->f_path != NULL && strlen(file->f_path.dentry->d_iname) > 0) //temp
						strcpy( tfname , file->f_path.dentry->d_iname);
					else
						return 1;
					if( strlen(tfname) < 1 || strlen(tfname) > 99 )
						return 1;
					strcpy( tdev, "X");

					if(file->f_flags & O_DIRECT)
						tflag = 2;
					else if(file->f_flags & __O_SYNC)
						tflag = 1;
					else //buffered
						tflag = 0; 
				}
				else
					return 1;
			}
			else //otherwise, fill variable with garbage data to prevent read NULL data
				return 1;

			//logging uid pid tid
			if(uid > 0 )
				tuid = uid;	
			else
				tuid = 0;	
			if(pid > 0 )
				tpid = pid;	
			else
				tpid = 0;
			if(tid > 0 )	
				ttid = tid;	
			else
				ttid = 0;	

			if(strcmp(tfname, "fuse") == 0) return 1;
			if(strcmp(tfname, "UNIX") == 0) return 1;
			if(strcmp(tfname, "tasks") == 0) return 1;
			if(strcmp(tfname, "stat") == 0) return 1;
			if(strcmp(tfname, "smem_log") == 0) return 1;
			if(strcmp(tfname, "smaps") == 0) return 1;
			if(strcmp(tfname, "maps") == 0) return 1;
			if(strcmp(tfname, "cmdline") == 0) return 1;
			if(strcmp(tfname, "status") == 0) return 1;
			if(strcmp(tfname, "environ") == 0) return 1;
			if(strcmp(tfname, "enforce") == 0) return 1;
			if(strcmp(tfname, "current") == 0) return 1;
			if(strcmp(tfname, "current_now") == 0) return 1;
			if(strcmp(tfname, "online") == 0) return 1;
			if(strcmp(tfname, "type") == 0) return 1;
			if(strcmp(tfname, "event0") == 0) return 1;
			if(strcmp(tfname, "capacity") == 0) return 1;
			if(strcmp(tfname, "temp") == 0) return 1;
			if(strcmp(tfname, "voltage_now") == 0) return 1;
			if(strcmp(tfname, "health") == 0) return 1;
			if(strcmp(tfname, "present") == 0) return 1;
			if(strcmp(tfname, "ptmx") == 0) return 1;
			if(strcmp(tfname, "ep1") == 0) return 1;
			if(strcmp(tfname, "ep2") == 0) return 1;
			if(strcmp(tfname, "technology") == 0) return 1;
			if(strcmp(tfname, "chg_therm") == 0) return 1;
			if(strcmp(tfname, "quiet_therm") == 0) return 1;
			if(strcmp(tfname, "oom_score_adj") == 0) return 1;
			if(strcmp(tfname, "wake_lock") == 0) return 1;
			if(strcmp(tfname, "wake_unlock") == 0) return 1;
			if(strcmp(tfname, "scaling_min_freq") == 0) return 1;
			if(strcmp(tfname, "urandom") == 0) return 1;
			if(strcmp(tfname, "TCP") == 0) return 1;
			if(strcmp(tfname, "TCPv6") == 0) return 1;
			if(strcmp(tfname, "cgroup") == 0) return 1;
			if(strcmp(tfname, "net") == 0) return 1;
			if(strcmp(tfname, "iface_stat_fmt") == 0) return 1;
			if(strcmp(tfname, "iface_stat_all") == 0) return 1;
			if(strcmp(tfname, "temp1_input") == 0) return 1;
			if(strcmp(tfname, "temp_hotspot") == 0) return 1;
			if(strcmp(tfname, "loadavg") == 0) return 1;
			if(strcmp(tfname, "sync_fence") == 0) return 1;
			if(strcmp(tfname, "hosts") == 0) return 1;
			if(strcmp(tfname, "meminfo") == 0) return 1;
			if(strcmp(tfname, "cpuinfo") == 0) return 1;
			if(strcmp(tfname, "inotify") == 0) return 1;
			if(strcmp(tfname, ".timestamp") == 0) return 1;
			if(strcmp(tfname, "value") == 0) return 1;
			if(strcmp(tfname, "ctrl") == 0) return 1;
			if(strcmp(tfname, "seapp_contexts") == 0) return 1;
			if(strcmp(tfname, "brightness") == 0) return 1;
			if(strcmp(tfname, "dmabuf") == 0) return 1;
			if(strcmp(tfname, "arm") == 0) return 1;

			//logging time
			tcsec = st_time.tv_sec;
			tcnsec = st_time.tv_nsec;
			if(cur_time.tv_sec >= st_time.tv_sec)
				tisec = cur_time.tv_sec - st_time.tv_sec;	
			else	
				return 1;
			if( cur_time.tv_nsec < st_time.tv_nsec ){
				if(tisec < 1)
					return 1;
				tisec -= 1;
				tinsec = ( cur_time.tv_nsec + 1000000000 )  - st_time.tv_nsec;
			}
			else	
				tinsec = cur_time.tv_nsec - st_time.tv_nsec;

			//filter part
			if( tcsec > 0 ){	
				add_aiopro_finfo(0, tfname, tuid, tpid, ttid, 1, count, tino);

				atomic_inc(&aios1);
				if( atomic_read(&aios1) != 1){
					s1_me++;
					set_tclog(fd, 0, tflag, tino, func, count, pos, 1, 
					tuid, tpid, ttid, tfname, tcsec, tcnsec, tisec, tinsec);
				}
				else{
					sys_log->func[sys_size] = func; 
					sys_log->count[sys_size] = count;
					sys_log->flag[sys_size] = tflag;
					sys_log->ino_no[sys_size] = tino;
					sys_log->pos[sys_size] = pos;
					sys_log->fd[sys_size] = fd;

					sys_log->uuid[sys_size] = tuid;
					sys_log->pid[sys_size] = tpid;
					sys_log->tid[sys_size] = ttid;

					//sys_log->lba[sys_size] = 1;
					sys_log->st_sec[sys_size] = tcsec;
					sys_log->st_nsec[sys_size] = tcnsec;
					sys_log->itv_sec[sys_size] = tisec;
					sys_log->itv_nsec[sys_size] = tinsec;

					strcpy( sys_log->fname[sys_size] , tfname);
					strcpy( sys_log->devname[sys_size], "X");

					sys_size += 1;
					sys_log->size = sys_size;
				}
				atomic_dec(&aios1);

			}
			else
				return 1;

			sys_flag = 0;
		}
		else{
			if(sys_flag == 0){
				sys_err+=1;
				sys_flag = 1; 
				printk("{IOPro}[ERROR @ SYS/READ] log size becomes larger than predefined size %d\n",AIOMAX);
			}
		}
	}
	return 0;
} 

int set_sys2_log(struct timespec st_time, struct timespec cur_time, unsigned int func, unsigned int fd, struct file *file, size_t count, loff_t pos, int uid, int pid, int tid){
	if(sys2_size != -1 && file != NULL){ //after initialized 
#ifdef aiopro_switch 
		if(aiopro_init() != 1 && aiopro_init() != 3){
			if((aiopro_init() == 2 ||aiopro_init() ==4) && flag2 == 0){
				printk("{IOPro}[SYS/WRITE] Stop Logging...[%lld](%u)\n",sys2_size,s2_me);
				flag2 = 1;
			}
			return 1;
		}
		else{
			if(flag2 == 1){
				set_sys_size(1,0);
				printk("{IOPro}[SYS/WRITE] Restart Logging...[%lld]\n",sys2_size);
				flag2 = 0;
				s2_me = 0;
			}
		}
#endif
		if(sys2_size < AIOMAX){ // within predefined memory size 
			struct address_space *mapping;
			struct inode *inode;

			int tflag, tuid, tpid, ttid, tcsec, tisec;//, tempv, alimit;
			long tcnsec, tinsec;
			unsigned long tino;
			size_t tsize;
			char tfname[100], tdev[32];	

			if(sys_start == 0){ printk("{IOPro}[fs/read_write.c] Start system call logging.\n"); sys_start=1;}

			//manage fsync information 
			if(func == 418){
				tflag = count; //datasync parameter

				if(file->f_mapping != NULL){
					mapping = file->f_mapping;
					if(mapping->host != NULL){
						inode = mapping->host;

						if( inode->i_ino < 10000 ) //need to survey
							return 1; 
					
						tino = inode->i_ino;
						if(&file->f_path != NULL && strlen(file->f_path.dentry->d_iname) > 0) //temp
							strcpy( tfname , file->f_path.dentry->d_iname);
						else
							strcpy( tfname , "sync");
					}	
					else{
						tino = 0;
						strcpy( tfname , "sync");
					}
				}
				else{
					tsize = 0; //pos
					tino = 0;
					strcpy( tfname , "sync");
				}
				goto after;
			}

			// get inode info
			if(file->f_mapping != NULL){
				mapping = file->f_mapping;
				if(mapping->host != NULL){
					inode = mapping->host; 
						
					//only if target dev is SCSI block dev which we look for
					tino = inode->i_ino;
					if(&file->f_path != NULL && strlen(file->f_path.dentry->d_iname) > 0) //temp
						strcpy( tfname , file->f_path.dentry->d_iname);
					else
						return 1;
					if( strlen(tfname) < 1 || strlen(tfname) > 99 )
						return 1;

					strcpy( tdev, "X");

					if(file->f_flags & O_DIRECT)
						tflag = 2;
					else if(file->f_flags & __O_SYNC)
						tflag = 1;
					else
						tflag = 0; 
				}
				else
					return 1;
			}
			else //otherwise, fill variable with garbage data to prevent read NULL data
				return 1;
after:
			//logging uid pid tid
			if(uid > 0 )
				tuid = uid;	
			else
				tuid = 0;	
			if(pid > 0 )
				tpid = pid;	
			else
				tpid = 0;
			if(tid > 0 )	
				ttid = tid;	
			else
				ttid = 0;	

			if(strcmp(tfname, "fuse") == 0) return 1;
			if(strcmp(tfname, "UNIX") == 0) return 1;
			if(strcmp(tfname, "tasks") == 0) return 1;
			if(strcmp(tfname, "stat") == 0) return 1;
			if(strcmp(tfname, "smem_log") == 0) return 1;
			if(strcmp(tfname, "smaps") == 0) return 1;
			if(strcmp(tfname, "maps") == 0) return 1;
			if(strcmp(tfname, "cmdline") == 0) return 1;
			if(strcmp(tfname, "status") == 0) return 1;
			if(strcmp(tfname, "environ") == 0) return 1;
			if(strcmp(tfname, "enforce") == 0) return 1;
			if(strcmp(tfname, "current") == 0) return 1;
			if(strcmp(tfname, "current_now") == 0) return 1;
			if(strcmp(tfname, "online") == 0) return 1;
			if(strcmp(tfname, "type") == 0) return 1;
			if(strcmp(tfname, "event0") == 0) return 1;
			if(strcmp(tfname, "capacity") == 0) return 1;
			if(strcmp(tfname, "temp") == 0) return 1;
			if(strcmp(tfname, "voltage_now") == 0) return 1;
			if(strcmp(tfname, "health") == 0) return 1;
			if(strcmp(tfname, "present") == 0) return 1;
			if(strcmp(tfname, "ptmx") == 0) return 1;
			if(strcmp(tfname, "ep1") == 0) return 1;
			if(strcmp(tfname, "ep2") == 0) return 1;
			if(strcmp(tfname, "technology") == 0) return 1;
			if(strcmp(tfname, "chg_therm") == 0) return 1;
			if(strcmp(tfname, "oom_score_adj") == 0) return 1;
			if(strcmp(tfname, "wake_lock") == 0) return 1;
			if(strcmp(tfname, "wake_unlock") == 0) return 1;
			if(strcmp(tfname, "scaling_min_freq") == 0) return 1;
			if(strcmp(tfname, "urandom") == 0) return 1;
			if(strcmp(tfname, "TCP") == 0) return 1;
			if(strcmp(tfname, "TCPv6") == 0) return 1;
			if(strcmp(tfname, "cgroup") == 0) return 1;
			if(strcmp(tfname, "net") == 0) return 1;
			if(strcmp(tfname, "iface_stat_fmt") == 0) return 1;
			if(strcmp(tfname, "iface_stat_all") == 0) return 1;
			if(strcmp(tfname, "temp1_input") == 0) return 1;
			if(strcmp(tfname, "temp_hotspot") == 0) return 1;
			if(strcmp(tfname, "loadavg") == 0) return 1;
			if(strcmp(tfname, "sync_fence") == 0) return 1;
			if(strcmp(tfname, "hosts") == 0) return 1;
			if(strcmp(tfname, "meminfo") == 0) return 1;
			if(strcmp(tfname, "cpuinfo") == 0) return 1;
			if(strcmp(tfname, "inotify") == 0) return 1;
			if(strcmp(tfname, ".timestamp") == 0) return 1;
			if(strcmp(tfname, "value") == 0) return 1;
			if(strcmp(tfname, "ctrl") == 0) return 1;
			if(strcmp(tfname, "seapp_contexts") == 0) return 1;
			if(strcmp(tfname, "brightness") == 0) return 1;
			if(strcmp(tfname, "dmabuf") == 0) return 1;
			if(strcmp(tfname, "arm") == 0) return 1;




			//logging time
			tcsec = st_time.tv_sec;
			tcnsec = st_time.tv_nsec;
			if(cur_time.tv_sec >= st_time.tv_sec)
				tisec = cur_time.tv_sec - st_time.tv_sec;	
			else	
				return 1;
			if( cur_time.tv_nsec < st_time.tv_nsec ){
				if(tisec < 1)
					return 1;
				tisec -= 1;
				tinsec = ( cur_time.tv_nsec + 1000000000 )  - st_time.tv_nsec;
			}
			else	
				tinsec = cur_time.tv_nsec - st_time.tv_nsec;

			//filter part
			if( tcsec > 0 ){
				add_aiopro_finfo(1, tfname, tuid, tpid, ttid, 1, pos, tino); 

				atomic_inc(&aios2);
				if( atomic_read(&aios2) != 1){
					s2_me++;
					set_tclog(fd, 1, tflag, tino, func, count, pos, 1, 
					tuid, tpid, ttid, tfname, tcsec, tcnsec, tisec, tinsec);

				}
				else{
					sys2_log->func[sys2_size] = func; 
					sys2_log->count[sys2_size] = count;
					sys2_log->flag[sys2_size] = tflag;
					sys2_log->ino_no[sys2_size] = tino;
					sys2_log->pos[sys2_size] = pos;
					sys2_log->fd[sys2_size] = fd;

					sys2_log->uuid[sys2_size] = tuid;
					sys2_log->pid[sys2_size] = tpid;
					sys2_log->tid[sys2_size] = ttid;

					//sys2_log->lba[sys2_size] = 1;
					sys2_log->st_sec[sys2_size] = tcsec;
					sys2_log->st_nsec[sys2_size] = tcnsec;
					sys2_log->itv_sec[sys2_size] = tisec;
					sys2_log->itv_nsec[sys2_size] = tinsec;

					strcpy( sys2_log->fname[sys2_size] , tfname);
					strcpy( sys2_log->devname[sys2_size], "X");

					sys2_size += 1;
					sys2_log->size = sys2_size;
				}
				atomic_dec(&aios2);

			}
			else if(sys2_log->func[sys2_size] == 418){
				atomic_inc(&aios2);
				if( atomic_read(&aios2) != 1){
					s2_me++;
					set_tclog(fd, 1, tflag, tino, func, 0, 0, 1, 
					tuid, tpid, ttid, tfname, tcsec, tcnsec, tisec, tinsec);
				}
				else{
					sys2_log->func[sys2_size] = func; 
					sys2_log->count[sys2_size] = 0;
					sys2_log->flag[sys2_size] = tflag;
					sys2_log->ino_no[sys2_size] = tino;
					sys2_log->pos[sys2_size] = 0;
					sys2_log->fd[sys2_size] = fd;

					sys2_log->uuid[sys2_size] = tuid;
					sys2_log->pid[sys2_size] = tpid;
					sys2_log->tid[sys2_size] = ttid;

					//sys2_log->lba[sys2_size] = 1;
					sys2_log->st_sec[sys2_size] = tcsec;
					sys2_log->st_nsec[sys2_size] = tcnsec;
					sys2_log->itv_sec[sys2_size] = tisec;
					sys2_log->itv_nsec[sys2_size] = tinsec;

					strcpy( sys2_log->fname[sys2_size] , tfname);
					strcpy( sys2_log->devname[sys2_size], "X");

					sys2_size += 1;
					sys2_log->size = sys2_size;
				}
				atomic_dec(&aios2);
			}
			else
				return 1; 

			sys2_flag = 0;
		}
		else{
			if(sys2_flag == 0){
				sys2_err+=1; 
				sys2_flag = 1; 
				printk("{IOPro}[ERROR @ SYS/WRITE] log size becomes larger than predefined size %d\n",AIOMAX);
			}
		}
	}
	return 0;
} 

struct aiopro_log *get_sys_log(int type){
	if(type == 0)
		return sys_log;
	else 
		return sys2_log;
}EXPORT_SYMBOL(get_sys_log);
#endif
////////////////////////////////////////////////////////////////////////////////////////////

typedef ssize_t (*io_fn_t)(struct file *, char __user *, size_t, loff_t *);
typedef ssize_t (*iov_fn_t)(struct kiocb *, const struct iovec *,
		unsigned long, loff_t);

const struct file_operations generic_ro_fops = {
	.llseek		= generic_file_llseek,
	.read		= do_sync_read,
	.aio_read	= generic_file_aio_read,
	.mmap		= generic_file_readonly_mmap,
	.splice_read	= generic_file_splice_read,
};

EXPORT_SYMBOL(generic_ro_fops);

static inline int unsigned_offsets(struct file *file)
{
	return file->f_mode & FMODE_UNSIGNED_OFFSET;
}

/**
 * vfs_setpos - update the file offset for lseek
 * @file:	file structure in question
 * @offset:	file offset to seek to
 * @maxsize:	maximum file size
 *
 * This is a low-level filesystem helper for updating the file offset to
 * the value specified by @offset if the given offset is valid and it is
 * not equal to the current file offset.
 *
 * Return the specified offset on success and -EINVAL on invalid offset.
 */
loff_t vfs_setpos(struct file *file, loff_t offset, loff_t maxsize)
{
	if (offset < 0 && !unsigned_offsets(file))
		return -EINVAL;
	if (offset > maxsize)
		return -EINVAL;

	if (offset != file->f_pos) {
		file->f_pos = offset;
		file->f_version = 0;
	}
	return offset;
}
EXPORT_SYMBOL(vfs_setpos);

/**
 * generic_file_llseek_size - generic llseek implementation for regular files
 * @file:	file structure to seek on
 * @offset:	file offset to seek to
 * @whence:	type of seek
 * @size:	max size of this file in file system
 * @eof:	offset used for SEEK_END position
 *
 * This is a variant of generic_file_llseek that allows passing in a custom
 * maximum file size and a custom EOF position, for e.g. hashed directories
 *
 * Synchronization:
 * SEEK_SET and SEEK_END are unsynchronized (but atomic on 64bit platforms)
 * SEEK_CUR is synchronized against other SEEK_CURs, but not read/writes.
 * read/writes behave like SEEK_SET against seeks.
 */
loff_t
generic_file_llseek_size(struct file *file, loff_t offset, int whence,
		loff_t maxsize, loff_t eof)
{
	switch (whence) {
	case SEEK_END:
		offset += eof;
		break;
	case SEEK_CUR:
		/*
		 * Here we special-case the lseek(fd, 0, SEEK_CUR)
		 * position-querying operation.  Avoid rewriting the "same"
		 * f_pos value back to the file because a concurrent read(),
		 * write() or lseek() might have altered it
		 */
		if (offset == 0)
			return file->f_pos;
		/*
		 * f_lock protects against read/modify/write race with other
		 * SEEK_CURs. Note that parallel writes and reads behave
		 * like SEEK_SET.
		 */
		spin_lock(&file->f_lock);
		offset = vfs_setpos(file, file->f_pos + offset, maxsize);
		spin_unlock(&file->f_lock);
		return offset;
	case SEEK_DATA:
		/*
		 * In the generic case the entire file is data, so as long as
		 * offset isn't at the end of the file then the offset is data.
		 */
		if (offset >= eof)
			return -ENXIO;
		break;
	case SEEK_HOLE:
		/*
		 * There is a virtual hole at the end of the file, so as long as
		 * offset isn't i_size or larger, return i_size.
		 */
		if (offset >= eof)
			return -ENXIO;
		offset = eof;
		break;
	}

	return vfs_setpos(file, offset, maxsize);
}
EXPORT_SYMBOL(generic_file_llseek_size);

/**
 * generic_file_llseek - generic llseek implementation for regular files
 * @file:	file structure to seek on
 * @offset:	file offset to seek to
 * @whence:	type of seek
 *
 * This is a generic implemenation of ->llseek useable for all normal local
 * filesystems.  It just updates the file offset to the value specified by
 * @offset and @whence.
 */
loff_t generic_file_llseek(struct file *file, loff_t offset, int whence)
{
	struct inode *inode = file->f_mapping->host;

	return generic_file_llseek_size(file, offset, whence,
					inode->i_sb->s_maxbytes,
					i_size_read(inode));
}
EXPORT_SYMBOL(generic_file_llseek);

/**
 * fixed_size_llseek - llseek implementation for fixed-sized devices
 * @file:	file structure to seek on
 * @offset:	file offset to seek to
 * @whence:	type of seek
 * @size:	size of the file
 *
 */
loff_t fixed_size_llseek(struct file *file, loff_t offset, int whence, loff_t size)
{
	switch (whence) {
	case SEEK_SET: case SEEK_CUR: case SEEK_END:
		return generic_file_llseek_size(file, offset, whence,
						size, size);
	default:
		return -EINVAL;
	}
}
EXPORT_SYMBOL(fixed_size_llseek);

/**
 * noop_llseek - No Operation Performed llseek implementation
 * @file:	file structure to seek on
 * @offset:	file offset to seek to
 * @whence:	type of seek
 *
 * This is an implementation of ->llseek useable for the rare special case when
 * userspace expects the seek to succeed but the (device) file is actually not
 * able to perform the seek. In this case you use noop_llseek() instead of
 * falling back to the default implementation of ->llseek.
 */
loff_t noop_llseek(struct file *file, loff_t offset, int whence)
{
	return file->f_pos;
}
EXPORT_SYMBOL(noop_llseek);

loff_t no_llseek(struct file *file, loff_t offset, int whence)
{
	return -ESPIPE;
}
EXPORT_SYMBOL(no_llseek);

loff_t default_llseek(struct file *file, loff_t offset, int whence)
{
	struct inode *inode = file_inode(file);
	loff_t retval;

	mutex_lock(&inode->i_mutex);
	switch (whence) {
		case SEEK_END:
			offset += i_size_read(inode);
			break;
		case SEEK_CUR:
			if (offset == 0) {
				retval = file->f_pos;
				goto out;
			}
			offset += file->f_pos;
			break;
		case SEEK_DATA:
			/*
			 * In the generic case the entire file is data, so as
			 * long as offset isn't at the end of the file then the
			 * offset is data.
			 */
			if (offset >= inode->i_size) {
				retval = -ENXIO;
				goto out;
			}
			break;
		case SEEK_HOLE:
			/*
			 * There is a virtual hole at the end of the file, so
			 * as long as offset isn't i_size or larger, return
			 * i_size.
			 */
			if (offset >= inode->i_size) {
				retval = -ENXIO;
				goto out;
			}
			offset = inode->i_size;
			break;
	}
	retval = -EINVAL;
	if (offset >= 0 || unsigned_offsets(file)) {
		if (offset != file->f_pos) {
			file->f_pos = offset;
			file->f_version = 0;
		}
		retval = offset;
	}
out:
	mutex_unlock(&inode->i_mutex);
	return retval;
}
EXPORT_SYMBOL(default_llseek);

loff_t vfs_llseek(struct file *file, loff_t offset, int whence)
{
	loff_t (*fn)(struct file *, loff_t, int);

	fn = no_llseek;
	if (file->f_mode & FMODE_LSEEK) {
		if (file->f_op && file->f_op->llseek)
			fn = file->f_op->llseek;
	}
	return fn(file, offset, whence);
}
EXPORT_SYMBOL(vfs_llseek);

SYSCALL_DEFINE3(lseek, unsigned int, fd, off_t, offset, unsigned int, whence)
{
	off_t retval;
	struct fd f = fdget(fd);
	if (!f.file)
		return -EBADF;

	retval = -EINVAL;
	if (whence <= SEEK_MAX) {
		loff_t res = vfs_llseek(f.file, offset, whence);
		retval = res;
		if (res != (loff_t)retval)
			retval = -EOVERFLOW;	/* LFS: should only happen on 32 bit platforms */
	}
	fdput(f);
	return retval;
}

#ifdef CONFIG_COMPAT
COMPAT_SYSCALL_DEFINE3(lseek, unsigned int, fd, compat_off_t, offset, unsigned int, whence)
{
	return sys_lseek(fd, offset, whence);
}
#endif

#ifdef __ARCH_WANT_SYS_LLSEEK
SYSCALL_DEFINE5(llseek, unsigned int, fd, unsigned long, offset_high,
		unsigned long, offset_low, loff_t __user *, result,
		unsigned int, whence)
{
	int retval;
	struct fd f = fdget(fd);
	loff_t offset;

	if (!f.file)
		return -EBADF;

	retval = -EINVAL;
	if (whence > SEEK_MAX)
		goto out_putf;

	offset = vfs_llseek(f.file, ((loff_t) offset_high << 32) | offset_low,
			whence);

	retval = (int)offset;
	if (offset >= 0) {
		retval = -EFAULT;
		if (!copy_to_user(result, &offset, sizeof(offset)))
			retval = 0;
	}
out_putf:
	fdput(f);
	return retval;
}
#endif

/*
 * rw_verify_area doesn't like huge counts. We limit
 * them to something that fits in "int" so that others
 * won't have to do range checks all the time.
 */
int rw_verify_area(int read_write, struct file *file, const loff_t *ppos, size_t count)
{
	struct inode *inode;
	loff_t pos;
	int retval = -EINVAL;

	inode = file_inode(file);
	if (unlikely((ssize_t) count < 0))
		return retval;
	pos = *ppos;
	if (unlikely(pos < 0)) {
		if (!unsigned_offsets(file))
			return retval;
		if (count >= -pos) /* both values are in 0..LLONG_MAX */
			return -EOVERFLOW;
	} else if (unlikely((loff_t) (pos + count) < 0)) {
		if (!unsigned_offsets(file))
			return retval;
	}

	if (unlikely(inode->i_flock && mandatory_lock(inode))) {
		retval = locks_mandatory_area(
			read_write == READ ? FLOCK_VERIFY_READ : FLOCK_VERIFY_WRITE,
			inode, file, pos, count);
		if (retval < 0)
			return retval;
	}
	retval = security_file_permission(file,
				read_write == READ ? MAY_READ : MAY_WRITE);
	if (retval)
		return retval;
	return count > MAX_RW_COUNT ? MAX_RW_COUNT : count;
}

ssize_t do_sync_read(struct file *filp, char __user *buf, size_t len, loff_t *ppos)
{
	struct iovec iov = { .iov_base = buf, .iov_len = len };
	struct kiocb kiocb;
	ssize_t ret;

	init_sync_kiocb(&kiocb, filp);
	kiocb.ki_pos = *ppos;
	kiocb.ki_left = len;
	kiocb.ki_nbytes = len;

	ret = filp->f_op->aio_read(&kiocb, &iov, 1, kiocb.ki_pos);
	if (-EIOCBQUEUED == ret)
		ret = wait_on_sync_kiocb(&kiocb);
	*ppos = kiocb.ki_pos;
	return ret;
}

EXPORT_SYMBOL(do_sync_read);

ssize_t vfs_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
	ssize_t ret;

	if (!(file->f_mode & FMODE_READ))
		return -EBADF;
	if (!file->f_op || (!file->f_op->read && !file->f_op->aio_read))
		return -EINVAL;
	if (unlikely(!access_ok(VERIFY_WRITE, buf, count)))
		return -EFAULT;

	ret = rw_verify_area(READ, file, pos, count);
	if (ret >= 0) {
		count = ret;
		if (file->f_op->read)
			ret = file->f_op->read(file, buf, count, pos);
		else
			ret = do_sync_read(file, buf, count, pos);
		if (ret > 0) {
			fsnotify_access(file);
			add_rchar(current, ret);
		}
		inc_syscr(current);
	}

	return ret;
}

EXPORT_SYMBOL(vfs_read);

ssize_t do_sync_write(struct file *filp, const char __user *buf, size_t len, loff_t *ppos)
{
	struct iovec iov = { .iov_base = (void __user *)buf, .iov_len = len };
	struct kiocb kiocb;
	ssize_t ret;

	init_sync_kiocb(&kiocb, filp);
	kiocb.ki_pos = *ppos;
	kiocb.ki_left = len;
	kiocb.ki_nbytes = len;

	ret = filp->f_op->aio_write(&kiocb, &iov, 1, kiocb.ki_pos);
	if (-EIOCBQUEUED == ret)
		ret = wait_on_sync_kiocb(&kiocb);
	*ppos = kiocb.ki_pos;
	return ret;
}

EXPORT_SYMBOL(do_sync_write);

ssize_t __kernel_write(struct file *file, const char *buf, size_t count, loff_t *pos)
{
	mm_segment_t old_fs;
	const char __user *p;
	ssize_t ret;

	if (!file->f_op || (!file->f_op->write && !file->f_op->aio_write))
		return -EINVAL;

	old_fs = get_fs();
	set_fs(get_ds());
	p = (__force const char __user *)buf;
	if (count > MAX_RW_COUNT)
		count =  MAX_RW_COUNT;
	if (file->f_op->write)
		ret = file->f_op->write(file, p, count, pos);
	else
		ret = do_sync_write(file, p, count, pos);
	set_fs(old_fs);
	if (ret > 0) {
		fsnotify_modify(file);
		add_wchar(current, ret);
	}
	inc_syscw(current);
	return ret;
}

ssize_t vfs_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
	ssize_t ret;

	if (!(file->f_mode & FMODE_WRITE))
		return -EBADF;
	if (!file->f_op || (!file->f_op->write && !file->f_op->aio_write))
		return -EINVAL;
	if (unlikely(!access_ok(VERIFY_READ, buf, count)))
		return -EFAULT;

	ret = rw_verify_area(WRITE, file, pos, count);
	if (ret >= 0) {
		count = ret;
		file_start_write(file);
		if (file->f_op->write)
			ret = file->f_op->write(file, buf, count, pos);
		else
			ret = do_sync_write(file, buf, count, pos);
		if (ret > 0) {
			fsnotify_modify(file);
			add_wchar(current, ret);
		}
		inc_syscw(current);
		file_end_write(file);
	}

	return ret;
}

EXPORT_SYMBOL(vfs_write);

static inline loff_t file_pos_read(struct file *file)
{
	return file->f_pos;
}

static inline void file_pos_write(struct file *file, loff_t pos)
{
	file->f_pos = pos;
}

SYSCALL_DEFINE3(read, unsigned int, fd, char __user *, buf, size_t, count)
{
	struct fd f = fdget(fd);
	ssize_t ret = -EBADF;

#ifdef aiopro_sys
struct timespec bbtime, cctime;
do_posix_clock_monotonic_gettime(&bbtime);
#endif

	if (f.file) {
		loff_t pos = file_pos_read(f.file);
		ret = vfs_read(f.file, buf, count, &pos);
		if (ret >= 0)
			file_pos_write(f.file, pos);

#ifdef aiopro_sys
//if(aiopro_init() == 1){
	do_posix_clock_monotonic_gettime(&cctime);
	set_sys_log(bbtime, cctime, 400, fd, f.file, count, pos, current_uid(), current->pid, current->tgid);
//}
#endif

		fdput(f);
	}
	return ret;
}

SYSCALL_DEFINE3(write, unsigned int, fd, const char __user *, buf,
		size_t, count)
{
	struct fd f = fdget(fd);
	ssize_t ret = -EBADF;

#ifdef aiopro_sys
struct timespec bbtime, cctime;
do_posix_clock_monotonic_gettime(&bbtime);
#endif

	if (f.file) {
		loff_t pos = file_pos_read(f.file);
		ret = vfs_write(f.file, buf, count, &pos);
		if (ret >= 0)
			file_pos_write(f.file, pos);
		fdput(f);

#ifdef aiopro_sys
//if(aiopro_init() == 1){
	do_posix_clock_monotonic_gettime(&cctime);
	set_sys2_log(bbtime, cctime, 410, fd, f.file, count, pos, current_uid(), current->pid, current->tgid); 
//}
#endif

	}

	return ret;
}

SYSCALL_DEFINE4(pread64, unsigned int, fd, char __user *, buf,
			size_t, count, loff_t, pos)
{
	struct fd f;
	ssize_t ret = -EBADF;

	if (pos < 0)
		return -EINVAL;

	f = fdget(fd);
	if (f.file) {
		ret = -ESPIPE;
		if (f.file->f_mode & FMODE_PREAD)
			ret = vfs_read(f.file, buf, count, &pos);
		fdput(f);
	}

	return ret;
}

SYSCALL_DEFINE4(pwrite64, unsigned int, fd, const char __user *, buf,
			 size_t, count, loff_t, pos)
{
	struct fd f;
	ssize_t ret = -EBADF;

	if (pos < 0)
		return -EINVAL;

	f = fdget(fd);
	if (f.file) {
		ret = -ESPIPE;
		if (f.file->f_mode & FMODE_PWRITE)  
			ret = vfs_write(f.file, buf, count, &pos);
		fdput(f);
	}

	return ret;
}

/*
 * Reduce an iovec's length in-place.  Return the resulting number of segments
 */
unsigned long iov_shorten(struct iovec *iov, unsigned long nr_segs, size_t to)
{
	unsigned long seg = 0;
	size_t len = 0;

	while (seg < nr_segs) {
		seg++;
		if (len + iov->iov_len >= to) {
			iov->iov_len = to - len;
			break;
		}
		len += iov->iov_len;
		iov++;
	}
	return seg;
}
EXPORT_SYMBOL(iov_shorten);

static ssize_t do_sync_readv_writev(struct file *filp, const struct iovec *iov,
		unsigned long nr_segs, size_t len, loff_t *ppos, iov_fn_t fn)
{
	struct kiocb kiocb;
	ssize_t ret;

	init_sync_kiocb(&kiocb, filp);
	kiocb.ki_pos = *ppos;
	kiocb.ki_left = len;
	kiocb.ki_nbytes = len;

	ret = fn(&kiocb, iov, nr_segs, kiocb.ki_pos);
	if (ret == -EIOCBQUEUED)
		ret = wait_on_sync_kiocb(&kiocb);
	*ppos = kiocb.ki_pos;
	return ret;
}

/* Do it by hand, with file-ops */
static ssize_t do_loop_readv_writev(struct file *filp, struct iovec *iov,
		unsigned long nr_segs, loff_t *ppos, io_fn_t fn)
{
	struct iovec *vector = iov;
	ssize_t ret = 0;

	while (nr_segs > 0) {
		void __user *base;
		size_t len;
		ssize_t nr;

		base = vector->iov_base;
		len = vector->iov_len;
		vector++;
		nr_segs--;

		nr = fn(filp, base, len, ppos);

		if (nr < 0) {
			if (!ret)
				ret = nr;
			break;
		}
		ret += nr;
		if (nr != len)
			break;
	}

	return ret;
}

/* A write operation does a read from user space and vice versa */
#define vrfy_dir(type) ((type) == READ ? VERIFY_WRITE : VERIFY_READ)

ssize_t rw_copy_check_uvector(int type, const struct iovec __user * uvector,
			      unsigned long nr_segs, unsigned long fast_segs,
			      struct iovec *fast_pointer,
			      struct iovec **ret_pointer)
{
	unsigned long seg;
	ssize_t ret;
	struct iovec *iov = fast_pointer;

	/*
	 * SuS says "The readv() function *may* fail if the iovcnt argument
	 * was less than or equal to 0, or greater than {IOV_MAX}.  Linux has
	 * traditionally returned zero for zero segments, so...
	 */
	if (nr_segs == 0) {
		ret = 0;
		goto out;
	}

	/*
	 * First get the "struct iovec" from user memory and
	 * verify all the pointers
	 */
	if (nr_segs > UIO_MAXIOV) {
		ret = -EINVAL;
		goto out;
	}
	if (nr_segs > fast_segs) {
		iov = kmalloc(nr_segs*sizeof(struct iovec), GFP_KERNEL);
		if (iov == NULL) {
			ret = -ENOMEM;
			goto out;
		}
	}
	if (copy_from_user(iov, uvector, nr_segs*sizeof(*uvector))) {
		ret = -EFAULT;
		goto out;
	}

	/*
	 * According to the Single Unix Specification we should return EINVAL
	 * if an element length is < 0 when cast to ssize_t or if the
	 * total length would overflow the ssize_t return value of the
	 * system call.
	 *
	 * Linux caps all read/write calls to MAX_RW_COUNT, and avoids the
	 * overflow case.
	 */
	ret = 0;
	for (seg = 0; seg < nr_segs; seg++) {
		void __user *buf = iov[seg].iov_base;
		ssize_t len = (ssize_t)iov[seg].iov_len;

		/* see if we we're about to use an invalid len or if
		 * it's about to overflow ssize_t */
		if (len < 0) {
			ret = -EINVAL;
			goto out;
		}
		if (type >= 0
		    && unlikely(!access_ok(vrfy_dir(type), buf, len))) {
			ret = -EFAULT;
			goto out;
		}
		if (len > MAX_RW_COUNT - ret) {
			len = MAX_RW_COUNT - ret;
			iov[seg].iov_len = len;
		}
		ret += len;
	}
out:
	*ret_pointer = iov;
	return ret;
}

static ssize_t do_readv_writev(int type, struct file *file,
			       const struct iovec __user * uvector,
			       unsigned long nr_segs, loff_t *pos)
{
	size_t tot_len;
	struct iovec iovstack[UIO_FASTIOV];
	struct iovec *iov = iovstack;
	ssize_t ret;
	io_fn_t fn;
	iov_fn_t fnv;

	if (!file->f_op) {
		ret = -EINVAL;
		goto out;
	}

	ret = rw_copy_check_uvector(type, uvector, nr_segs,
				    ARRAY_SIZE(iovstack), iovstack, &iov);
	if (ret <= 0)
		goto out;

	tot_len = ret;
	ret = rw_verify_area(type, file, pos, tot_len);
	if (ret < 0)
		goto out;

	fnv = NULL;
	if (type == READ) {
		fn = file->f_op->read;
		fnv = file->f_op->aio_read;
	} else {
		fn = (io_fn_t)file->f_op->write;
		fnv = file->f_op->aio_write;
		file_start_write(file);
	}

	if (fnv)
		ret = do_sync_readv_writev(file, iov, nr_segs, tot_len,
						pos, fnv);
	else
		ret = do_loop_readv_writev(file, iov, nr_segs, pos, fn);

	if (type != READ)
		file_end_write(file);

out:
	if (iov != iovstack)
		kfree(iov);
	if ((ret + (type == READ)) > 0) {
		if (type == READ)
			fsnotify_access(file);
		else
			fsnotify_modify(file);
	}
	return ret;
}

ssize_t vfs_readv(struct file *file, const struct iovec __user *vec,
		  unsigned long vlen, loff_t *pos)
{
	if (!(file->f_mode & FMODE_READ))
		return -EBADF;
	if (!file->f_op || (!file->f_op->aio_read && !file->f_op->read))
		return -EINVAL;

	return do_readv_writev(READ, file, vec, vlen, pos);
}

EXPORT_SYMBOL(vfs_readv);

ssize_t vfs_writev(struct file *file, const struct iovec __user *vec,
		   unsigned long vlen, loff_t *pos)
{
	if (!(file->f_mode & FMODE_WRITE))
		return -EBADF;
	if (!file->f_op || (!file->f_op->aio_write && !file->f_op->write))
		return -EINVAL;

	return do_readv_writev(WRITE, file, vec, vlen, pos);
}

EXPORT_SYMBOL(vfs_writev);

SYSCALL_DEFINE3(readv, unsigned long, fd, const struct iovec __user *, vec,
		unsigned long, vlen)
{
	struct fd f = fdget(fd);
	ssize_t ret = -EBADF;

	if (f.file) {
		loff_t pos = file_pos_read(f.file);
		ret = vfs_readv(f.file, vec, vlen, &pos);
		if (ret >= 0)
			file_pos_write(f.file, pos);
		fdput(f);
	}

	if (ret > 0)
		add_rchar(current, ret);
	inc_syscr(current);
	return ret;
}

SYSCALL_DEFINE3(writev, unsigned long, fd, const struct iovec __user *, vec,
		unsigned long, vlen)
{
	struct fd f = fdget(fd);
	ssize_t ret = -EBADF;

	if (f.file) {
		loff_t pos = file_pos_read(f.file);
		ret = vfs_writev(f.file, vec, vlen, &pos);
		if (ret >= 0)
			file_pos_write(f.file, pos);
		fdput(f);
	}

	if (ret > 0)
		add_wchar(current, ret);
	inc_syscw(current);
	return ret;
}

static inline loff_t pos_from_hilo(unsigned long high, unsigned long low)
{
#define HALF_LONG_BITS (BITS_PER_LONG / 2)
	return (((loff_t)high << HALF_LONG_BITS) << HALF_LONG_BITS) | low;
}

SYSCALL_DEFINE5(preadv, unsigned long, fd, const struct iovec __user *, vec,
		unsigned long, vlen, unsigned long, pos_l, unsigned long, pos_h)
{
	loff_t pos = pos_from_hilo(pos_h, pos_l);
	struct fd f;
	ssize_t ret = -EBADF;

	if (pos < 0)
		return -EINVAL;

	f = fdget(fd);
	if (f.file) {
		ret = -ESPIPE;
		if (f.file->f_mode & FMODE_PREAD)
			ret = vfs_readv(f.file, vec, vlen, &pos);
		fdput(f);
	}

	if (ret > 0)
		add_rchar(current, ret);
	inc_syscr(current);
	return ret;
}

SYSCALL_DEFINE5(pwritev, unsigned long, fd, const struct iovec __user *, vec,
		unsigned long, vlen, unsigned long, pos_l, unsigned long, pos_h)
{
	loff_t pos = pos_from_hilo(pos_h, pos_l);
	struct fd f;
	ssize_t ret = -EBADF;

	if (pos < 0)
		return -EINVAL;

	f = fdget(fd);
	if (f.file) {
		ret = -ESPIPE;
		if (f.file->f_mode & FMODE_PWRITE)
			ret = vfs_writev(f.file, vec, vlen, &pos);
		fdput(f);
	}

	if (ret > 0)
		add_wchar(current, ret);
	inc_syscw(current);
	return ret;
}

#ifdef CONFIG_COMPAT

static ssize_t compat_do_readv_writev(int type, struct file *file,
			       const struct compat_iovec __user *uvector,
			       unsigned long nr_segs, loff_t *pos)
{
	compat_ssize_t tot_len;
	struct iovec iovstack[UIO_FASTIOV];
	struct iovec *iov = iovstack;
	ssize_t ret;
	io_fn_t fn;
	iov_fn_t fnv;

	ret = -EINVAL;
	if (!file->f_op)
		goto out;

	ret = -EFAULT;
	if (!access_ok(VERIFY_READ, uvector, nr_segs*sizeof(*uvector)))
		goto out;

	ret = compat_rw_copy_check_uvector(type, uvector, nr_segs,
					       UIO_FASTIOV, iovstack, &iov);
	if (ret <= 0)
		goto out;

	tot_len = ret;
	ret = rw_verify_area(type, file, pos, tot_len);
	if (ret < 0)
		goto out;

	fnv = NULL;
	if (type == READ) {
		fn = file->f_op->read;
		fnv = file->f_op->aio_read;
	} else {
		fn = (io_fn_t)file->f_op->write;
		fnv = file->f_op->aio_write;
		file_start_write(file);
	}

	if (fnv)
		ret = do_sync_readv_writev(file, iov, nr_segs, tot_len,
						pos, fnv);
	else
		ret = do_loop_readv_writev(file, iov, nr_segs, pos, fn);

	if (type != READ)
		file_end_write(file);

out:
	if (iov != iovstack)
		kfree(iov);
	if ((ret + (type == READ)) > 0) {
		if (type == READ)
			fsnotify_access(file);
		else
			fsnotify_modify(file);
	}
	return ret;
}

static size_t compat_readv(struct file *file,
			   const struct compat_iovec __user *vec,
			   unsigned long vlen, loff_t *pos)
{
	ssize_t ret = -EBADF;

	if (!(file->f_mode & FMODE_READ))
		goto out;

	ret = -EINVAL;
	if (!file->f_op || (!file->f_op->aio_read && !file->f_op->read))
		goto out;

	ret = compat_do_readv_writev(READ, file, vec, vlen, pos);

out:
	if (ret > 0)
		add_rchar(current, ret);
	inc_syscr(current);
	return ret;
}

COMPAT_SYSCALL_DEFINE3(readv, unsigned long, fd,
		const struct compat_iovec __user *,vec,
		unsigned long, vlen)
{
	struct fd f = fdget(fd);
	ssize_t ret;
	loff_t pos;

	if (!f.file)
		return -EBADF;
	pos = f.file->f_pos;
	ret = compat_readv(f.file, vec, vlen, &pos);
	if (ret >= 0)
		f.file->f_pos = pos;
	fdput(f);
	return ret;
}

COMPAT_SYSCALL_DEFINE4(preadv64, unsigned long, fd,
		const struct compat_iovec __user *,vec,
		unsigned long, vlen, loff_t, pos)
{
	struct fd f;
	ssize_t ret;

	if (pos < 0)
		return -EINVAL;
	f = fdget(fd);
	if (!f.file)
		return -EBADF;
	ret = -ESPIPE;
	if (f.file->f_mode & FMODE_PREAD)
		ret = compat_readv(f.file, vec, vlen, &pos);
	fdput(f);
	return ret;
}

COMPAT_SYSCALL_DEFINE5(preadv, unsigned long, fd,
		const struct compat_iovec __user *,vec,
		unsigned long, vlen, u32, pos_low, u32, pos_high)
{
	loff_t pos = ((loff_t)pos_high << 32) | pos_low;
	return compat_sys_preadv64(fd, vec, vlen, pos);
}

static size_t compat_writev(struct file *file,
			    const struct compat_iovec __user *vec,
			    unsigned long vlen, loff_t *pos)
{
	ssize_t ret = -EBADF;

	if (!(file->f_mode & FMODE_WRITE))
		goto out;

	ret = -EINVAL;
	if (!file->f_op || (!file->f_op->aio_write && !file->f_op->write))
		goto out;

	ret = compat_do_readv_writev(WRITE, file, vec, vlen, pos);

out:
	if (ret > 0)
		add_wchar(current, ret);
	inc_syscw(current);
	return ret;
}

COMPAT_SYSCALL_DEFINE3(writev, unsigned long, fd,
		const struct compat_iovec __user *, vec,
		unsigned long, vlen)
{
	struct fd f = fdget(fd);
	ssize_t ret;
	loff_t pos;

	if (!f.file)
		return -EBADF;
	pos = f.file->f_pos;
	ret = compat_writev(f.file, vec, vlen, &pos);
	if (ret >= 0)
		f.file->f_pos = pos;
	fdput(f);
	return ret;
}

COMPAT_SYSCALL_DEFINE4(pwritev64, unsigned long, fd,
		const struct compat_iovec __user *,vec,
		unsigned long, vlen, loff_t, pos)
{
	struct fd f;
	ssize_t ret;

	if (pos < 0)
		return -EINVAL;
	f = fdget(fd);
	if (!f.file)
		return -EBADF;
	ret = -ESPIPE;
	if (f.file->f_mode & FMODE_PWRITE)
		ret = compat_writev(f.file, vec, vlen, &pos);
	fdput(f);
	return ret;
}

COMPAT_SYSCALL_DEFINE5(pwritev, unsigned long, fd,
		const struct compat_iovec __user *,vec,
		unsigned long, vlen, u32, pos_low, u32, pos_high)
{
	loff_t pos = ((loff_t)pos_high << 32) | pos_low;
	return compat_sys_pwritev64(fd, vec, vlen, pos);
}
#endif

static ssize_t do_sendfile(int out_fd, int in_fd, loff_t *ppos,
		  	   size_t count, loff_t max)
{
	struct fd in, out;
	struct inode *in_inode, *out_inode;
	loff_t pos;
	loff_t out_pos;
	ssize_t retval;
	int fl;

	/*
	 * Get input file, and verify that it is ok..
	 */
	retval = -EBADF;
	in = fdget(in_fd);
	if (!in.file)
		goto out;
	if (!(in.file->f_mode & FMODE_READ))
		goto fput_in;
	retval = -ESPIPE;
	if (!ppos) {
		pos = in.file->f_pos;
	} else {
		pos = *ppos;
		if (!(in.file->f_mode & FMODE_PREAD))
			goto fput_in;
	}
	retval = rw_verify_area(READ, in.file, &pos, count);
	if (retval < 0)
		goto fput_in;
	count = retval;

	/*
	 * Get output file, and verify that it is ok..
	 */
	retval = -EBADF;
	out = fdget(out_fd);
	if (!out.file)
		goto fput_in;
	if (!(out.file->f_mode & FMODE_WRITE))
		goto fput_out;
	retval = -EINVAL;
	in_inode = file_inode(in.file);
	out_inode = file_inode(out.file);
	out_pos = out.file->f_pos;
	retval = rw_verify_area(WRITE, out.file, &out_pos, count);
	if (retval < 0)
		goto fput_out;
	count = retval;

	if (!max)
		max = min(in_inode->i_sb->s_maxbytes, out_inode->i_sb->s_maxbytes);

	if (unlikely(pos + count > max)) {
		retval = -EOVERFLOW;
		if (pos >= max)
			goto fput_out;
		count = max - pos;
	}

	fl = 0;
#if 0
	/*
	 * We need to debate whether we can enable this or not. The
	 * man page documents EAGAIN return for the output at least,
	 * and the application is arguably buggy if it doesn't expect
	 * EAGAIN on a non-blocking file descriptor.
	 */
	if (in.file->f_flags & O_NONBLOCK)
		fl = SPLICE_F_NONBLOCK;
#endif
	file_start_write(out.file);
	retval = do_splice_direct(in.file, &pos, out.file, &out_pos, count, fl);
	file_end_write(out.file);

	if (retval > 0) {
		add_rchar(current, retval);
		add_wchar(current, retval);
		fsnotify_access(in.file);
		fsnotify_modify(out.file);
		out.file->f_pos = out_pos;
		if (ppos)
			*ppos = pos;
		else
			in.file->f_pos = pos;
	}

	inc_syscr(current);
	inc_syscw(current);
	if (pos > max)
		retval = -EOVERFLOW;

fput_out:
	fdput(out);
fput_in:
	fdput(in);
out:
	return retval;
}

SYSCALL_DEFINE4(sendfile, int, out_fd, int, in_fd, off_t __user *, offset, size_t, count)
{
	loff_t pos;
	off_t off;
	ssize_t ret;

	if (offset) {
		if (unlikely(get_user(off, offset)))
			return -EFAULT;
		pos = off;
		ret = do_sendfile(out_fd, in_fd, &pos, count, MAX_NON_LFS);
		if (unlikely(put_user(pos, offset)))
			return -EFAULT;
		return ret;
	}

	return do_sendfile(out_fd, in_fd, NULL, count, 0);
}

SYSCALL_DEFINE4(sendfile64, int, out_fd, int, in_fd, loff_t __user *, offset, size_t, count)
{
	loff_t pos;
	ssize_t ret;

	if (offset) {
		if (unlikely(copy_from_user(&pos, offset, sizeof(loff_t))))
			return -EFAULT;
		ret = do_sendfile(out_fd, in_fd, &pos, count, 0);
		if (unlikely(put_user(pos, offset)))
			return -EFAULT;
		return ret;
	}

	return do_sendfile(out_fd, in_fd, NULL, count, 0);
}

#ifdef CONFIG_COMPAT
COMPAT_SYSCALL_DEFINE4(sendfile, int, out_fd, int, in_fd,
		compat_off_t __user *, offset, compat_size_t, count)
{
	loff_t pos;
	off_t off;
	ssize_t ret;

	if (offset) {
		if (unlikely(get_user(off, offset)))
			return -EFAULT;
		pos = off;
		ret = do_sendfile(out_fd, in_fd, &pos, count, MAX_NON_LFS);
		if (unlikely(put_user(pos, offset)))
			return -EFAULT;
		return ret;
	}

	return do_sendfile(out_fd, in_fd, NULL, count, 0);
}

COMPAT_SYSCALL_DEFINE4(sendfile64, int, out_fd, int, in_fd,
		compat_loff_t __user *, offset, compat_size_t, count)
{
	loff_t pos;
	ssize_t ret;

	if (offset) {
		if (unlikely(copy_from_user(&pos, offset, sizeof(loff_t))))
			return -EFAULT;
		ret = do_sendfile(out_fd, in_fd, &pos, count, 0);
		if (unlikely(put_user(pos, offset)))
			return -EFAULT;
		return ret;
	}

	return do_sendfile(out_fd, in_fd, NULL, count, 0);
}
#endif
