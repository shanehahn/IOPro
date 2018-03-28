#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/mman.h>
#include <linux/sched.h>
#include <linux/rwsem.h>
#include <linux/pagemap.h>
#include <linux/rmap.h>
#include <linux/spinlock.h>
#include <linux/jhash.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/rbtree.h>
#include <linux/memory.h>
#include <linux/mmu_notifier.h>
#include <linux/swap.h>
#include <linux/ksm.h>
#include <linux/hashtable.h>
#include <linux/freezer.h>
#include <linux/oom.h>
#include <linux/numa.h>

#include <linux/completion.h>
#include <linux/timer.h>
#include <linux/time.h>
#include <linux/vmalloc.h>
#include <asm/uaccess.h>
#include <asm/segment.h>
#include <linux/buffer_head.h>
#include <linux/string.h>

///////////////////////////aiopro part
#include <linux/debugfs.h>
#include <linux/fs.h>
///////////////////////////////////////
#define aiopro_s6
//#define aiopro_sdcard                                                                                                                                          
//#define aiopro_usb                                                                                                                                             
//#define no_write                                                                                                                                               
//#define aiopro_ubuntu

//////////////////////////////////////////////////////////////path for log                                                                                       
#ifdef aiopro_ubuntu
#define SYS_PATH "/home/shanelab/log_sys.txt"
#define SYS2_PATH "/home/shanelab/log_sys2.txt"
#define MM_PATH "/home/shanelab/log_mm.txt"
#define SE_PATH "/home/shanelab/log_se.txt"
#define FS_PATH "/home/shanelab/log_fs.txt"
#define BIO_PATH "/home/shanelab/log_bio.txt"
#define SCSI_PATH "/home/shanelab/log_scsi.txt"
#define UFS_PATH "/home/shanelab/log_ufs.txt"
#endif

#ifdef aiopro_s6
#define SYS_PATH "/sdcard/test/log_sys.txt"
#define SYS2_PATH "/sdcard/test/log_sys2.txt"
#define MM_PATH "/sdcard/test/log_mm.txt"
#define FS_PATH "/sdcard/test/log_fs.txt"
#define SE_PATH "/sdcard/test/log_se.txt"
#define BIO_PATH "/sdcard/test/log_bio.txt"
#define SCSI_PATH "/sdcard/test/log_scsi.txt"
#define UFS_PATH "/sdcard/test/log_ufs.txt"
#endif

#ifdef aiopro_sdcard
#define SYS_PATH "/storage/sdcard/log_sys.txt"
#define SYS2_PATH "/storage/sdcard/log_sys2.txt"
#define MM_PATH "/storage/sdcard/log_mm.txt"
#define FS_PATH "/storage/sdcard/log_fs.txt"
#define SE_PATH "/storage/sdcard/log_se.txt"
#define BIO_PATH "/storage/sdcard/log_bio.txt"
#define SCSI_PATH "/storage/sdcard/log_scsi.txt"
#define UFS_PATH "/storage/sdcard/log_ufs.txt"
#endif

#ifdef aiopro_usb
#define SYS_PATH "/storage/UsbDriveA/log/log_sys.txt"
#define SYS2_PATH "/storage/UsbDriveA/log/log_sys2.txt"
#define MM_PATH "/storage/UsbDriveA/log/log_mm.txt"
#define FS_PATH "/storage/UsbDriveA/loglog_fs.txt"
#define SE_PATH "/storage/UsbDriveA/loglog_se.txt"
#define BIO_PATH "/storage/UsbDriveA/log/log_bio.txt"
#define SCSI_PATH "/storage/UsbDriveA/log/log_scsi.txt"
#define UFS_PATH "/storage/UsbDriveA/log/log_ufs.txt"
#endif

#ifdef no_write
#define SYS_PATH "/storage/sdcard/log_sys.txt"
#define SYS2_PATH "/storage/sdcard/log_sys2.txt"
#define MM_PATH "/storage/sdcard/log_mm.txt"
#define SE_PATH "/storage/sdcard/log_fs.txt"
#define SE_PATH "/storage/sdcard/log_se.txt"
#define BIO_PATH "/storage/sdcard/log_bio.txt"
#define SCSI_PATH "/storage/sdcard/log_scsi.txt"
#define UFS_PATH "/storage/sdcard/log_ufs.txt"
#endif

#define MEMMAX 200000
#define TMPMAX 2000
#define LOGMAX (AIOMAX*8/10)
#define BLOGMAX (BIOMAX*8/10)
#define WAITV 10

//////////////////////////////////////////////////////////////global variables                                                                                   
struct task_struct *aiopro_sys_task=NULL;
struct task_struct *aiopro_mm_task=NULL;
struct task_struct *aiopro_se_task=NULL;
struct task_struct *aiopro_bio_task=NULL;
struct task_struct *aiopro_scsi_task=NULL;
struct task_struct *aiopro_ufs_task=NULL;
int sys_fo=0, mm_fo=0, se_fo=0, bio_fo=0, ufs_fo=0, scsi_fo=0;
struct timespec biocc, biobb, syscc, sysbb, sys2cc, sys2bb, mmcc, mmbb, secc, sebb, ufscc, ufsbb,scsicc, scsibb;
int biocnt=0, syscnt=0, sys2cnt=0, secnt=0, mmcnt=0, ufscnt=0,scsicnt=0;
int bioperr=0, sysperr=0, sys2perr=0, seperr=0, mmperr=0, ufsperr=0, scsiperr=0;
int64_t befsys1=0, befsys2=0, befmm=0, befse=0, befbio=0, befscsi=0, befufs=0;

////////////////////////////////////////////////////////////////////file io function part 
struct file* aio_file_open(const char* path, int flags, int rights) {
        struct file* filp = NULL;
        mm_segment_t oldfs;
        int err = 0;
        oldfs = get_fs();
        set_fs(get_ds());
        filp = filp_open(path, flags, rights);
        set_fs(oldfs);
        if(IS_ERR(filp)) {
                err = PTR_ERR(filp); printk("err code is [%d]\n",err);
                return NULL;    }
        return filp;
}
void aio_file_close(struct file* file) {
	filp_close(file, NULL);
}
int aio_file_read(struct file* file, unsigned long long offset, unsigned char* data, unsigned int size) {
        mm_segment_t oldfs;
        int ret;
        oldfs = get_fs();
        set_fs(get_ds());
        ret = vfs_read(file, data, size, &offset);
        set_fs(oldfs);
        return ret;
}
int aio_file_write(struct file* file, unsigned long long offset, unsigned char* data, unsigned int size) {
        mm_segment_t oldfs;
        int ret;
#ifdef no_write
        return 1;
#endif
        oldfs = get_fs();
        set_fs(get_ds());
        ret = vfs_write(file, data, size, &offset);
        set_fs(oldfs);
        return ret;
}
int aio_file_sync(struct file* file) {
        vfs_fsync(file, 0);
        return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////start end time logging part
struct aiopro_log *se_log; 
int se_size;

void init_se_log(void){
printk("{AIOPro}[fs/aiopro.c] Initialize structure for start end time logging ... ");
	se_log = (struct aiopro_log *) vmalloc(sizeof(struct aiopro_log));
	se_size = 0;
	se_log->flag = (int*)vmalloc(20 * sizeof(int));
	se_log->st_sec = (int*)vmalloc(20 * sizeof(int));
printk("Done\n");
}EXPORT_SYMBOL(init_se_log);

int set_se_log(int when){
	struct timespec st_time;
	do_posix_clock_monotonic_gettime(&st_time);
	se_log->st_sec[se_size] = st_time.tv_sec;

	if(when == 0) //when aiporo starts
		se_log->flag[se_size] = 0;
	else //when aiopro ends
		se_log->flag[se_size] = 1;
	
	se_size++;
	if(se_size >= 20){
		se_size = 19;
		printk("{AIOPro}[PRINT/SE] Start end log becomes over max\n");
	}
	return 0;
}EXPORT_SYMBOL(set_se_log);

int print_se_result(void){
	struct file* fp = NULL;
	char *temp, tmp[TMPMAX];
	int i=0, len=0;
	
	msleep(5000);
	if( aiopro_init() != 1 && se_size > 0 ){
	
		printk("{AIOPro}[PRINT/SE] Start writing data to file (%d)\n", se_size);
		//set_se_log(1);

#ifdef aiopro_ubuntu
		if ((fp = aio_file_open ( SE_PATH , O_CREAT | O_APPEND | O_WRONLY, 0666)) == NULL)
#endif 
#ifdef aiopro_s6
		if ((fp = aio_file_open ( SE_PATH , O_CREAT | O_APPEND | O_WRONLY, 0666)) == NULL)
#endif 
#ifdef aiopro_sdcard
		if ((fp = aio_file_open ( SE_PATH , O_CREAT | O_APPEND | O_WRONLY, 0666)) == NULL) 
#endif
#ifdef aiopro_usb
		if ((fp = aio_file_open ( SE_PATH , O_CREAT | O_APPEND | O_WRONLY, 0660)) == NULL)
#endif
		{ 
		printk (KERN_INFO "{AIOPro}[Print/SE] fwrite: file_open error (%s)\n", SE_PATH);
			se_fo=1;
			return 1;
		}

		temp = (char*)vmalloc( TMPMAX * sizeof(char));
		memset(temp, 0, TMPMAX);

		while(i < se_size){
			memset(tmp, 0, TMPMAX);
			sprintf(tmp, "%d %d\n", 
				se_log->flag[i], se_log->st_sec[i]); 
			memcpy(temp+len, &tmp, strlen(tmp)+1);
			len += (strlen(tmp));
			i++;
		}

		aio_file_write (fp, 0, temp, strlen(temp));
                printk("{AIOPro}[Print/SE] Finish writing logs after stopping loggging (%d)\n",i);
		vfree(temp);
	if(fp != NULL) aio_file_sync(fp);
		aio_file_close (fp);
		se_size = 0;
	}
	return 0;
}

///////////////////////////////////////////////////print log for tc
///////////////////////////////////////////////////////TClog part///////////////////////////
int print_tc(int jj, int64_t ii, int layer){
        struct file* fp = NULL;
        struct aiopro_log *tc_log;
        char tmp[TMPMAX];
	unsigned long long itime;

        if(layer == 4){
		if((fp = aio_file_open ( SYS_PATH , O_CREAT | O_APPEND | O_WRONLY, 0666)) == NULL){
			printk (KERN_INFO "{AIOPro}[Print/TC] fwrite: file_open error (%s)\n", SYS_PATH);
			return 1;
		}
	}
        else if(layer == 5){
		if((fp = aio_file_open ( MM_PATH , O_CREAT | O_APPEND | O_WRONLY, 0666)) == NULL){
			printk (KERN_INFO "{AIOPro}[Print/TC] fwrite: file_open error (%s)\n", MM_PATH);
			return 1;
		}
	}
        else if(layer == 6){
		if((fp = aio_file_open ( FS_PATH , O_CREAT | O_APPEND | O_WRONLY, 0666)) == NULL){
			printk (KERN_INFO "{AIOPro}[Print/TC] fwrite: file_open error (%s)\n", FS_PATH);
			return 1;
		}
	}
        else if(layer == 7){
		if((fp = aio_file_open ( BIO_PATH , O_CREAT | O_APPEND | O_WRONLY, 0666)) == NULL){
			printk (KERN_INFO "{AIOPro}[Print/TC] fwrite: file_open error (%s)\n", BIO_PATH);
			return 1;
		}
	}
        else if(layer == 8){
		if((fp = aio_file_open ( SCSI_PATH , O_CREAT | O_APPEND | O_WRONLY, 0666)) == NULL){
			printk (KERN_INFO "{AIOPro}[Print/TC] fwrite: file_open error (%s)\n", SCSI_PATH);
			return 1;
		}
	}
        else{
		if((fp = aio_file_open ( UFS_PATH , O_CREAT | O_APPEND | O_WRONLY, 0666)) == NULL){
			printk (KERN_INFO "{AIOPro}[Print/TC] fwrite: file_open error (%s)\n", UFS_PATH);
			return 1;
		}
	}

        //get memory space for string                                                                                                                    
        memset(tmp, 0, TMPMAX);
	tc_log = get_tclog(jj);

	itime = tc_log->itv_sec[ii] * 1000000000;
	itime += tc_log->itv_nsec[ii];


	if(tc_log->st_sec[ii] > 0){
		sprintf(tmp, "%d %ld %llu %d %d %d %d %u %u %u %u %lld %lu %lu %lu %s\n",
			tc_log->st_sec[ii], tc_log->st_nsec[ii], itime, 
			tc_log->uuid[ii],   tc_log->pid[ii],     tc_log->tid[ii],
			layer,              tc_log->rw[ii],      tc_log->func[ii], 
			tc_log->flag[ii],   tc_log->fd[ii],      tc_log->pos[ii],
			tc_log->count[ii],  tc_log->ino_no[ii], 
			tc_log->lba[ii],    tc_log->fname[ii] );
		
		aio_file_write (fp, 0, tmp, strlen(tmp));
	}
	if(fp != NULL) aio_file_sync(fp);
       	aio_file_close (fp);
	return 0;
}
/////////////////////////////////////////////////////////////////////////////////////




//////////////////////////////////////////////////////print log for each layer part                                                                              
int print_scsi_result(void){
#ifdef aiopro_scsi
        struct file* fp = NULL;
        struct aiopro_log *scsi_log;
        char tmp[TMPMAX], *temp;
        int64_t size, i=0;
        uint64_t len=0, fpl=0, err=0;
	int j;	
	unsigned long long itime;

        //sleep with predifined interval - memory mgmt : 1 sec                                                                                                   
        msleep(1000);

        //get log and its size                                                                                                                                   
        scsi_log = get_scsi_log();
        size = scsi_log->size;

#ifdef aiopro_switch
        //if logging stopped before                                                                                                                              
        if(aiopro_init() != 1){
                // and its size is smaller than 2, then it was finished normally                                                                                 
                if( size < 2)
                        return 1;
                else{
                        printk("{AIOPro}[PRINT/UFS] Prepare for writing data to file after stopping logging [%lld]\n",size);
			scsicnt = -1;
			msleep(3000);
		}
        }
#endif

        //if size is larger than predefined value 1000 or thread waited more than predefined value WAITV, start writing                                          
        if( size > LOGMAX || scsicnt == -1){
		printk("{AIOPro}[PRINT/UFS] Start writing data to file (%lld)\n",size);

                //check if log size was too small to store previous occured I/O log                                                                              
                err = get_scsi_err();
                if(err > 0 && err > scsiperr){
                        printk("{AIOPro}[Print/UFS] logging size is over predefined maximum log size~!!! (%llu)\n", size);
                        scsiperr = err;
                }

                //open file
#ifdef aiopro_ubuntu
                if ((fp = aio_file_open ( UFS_PATH , O_CREAT | O_APPEND | O_WRONLY, 0666)) == NULL)
#endif
#ifdef aiopro_s6
                if ((fp = aio_file_open ( UFS_PATH , O_CREAT | O_APPEND | O_WRONLY, 0666)) == NULL)
#endif
#ifdef aiopro_sdcard
		if ((fp = aio_file_open ( UFS_PATH , O_CREAT | O_APPEND | O_WRONLY, 0666)) == NULL)
#endif
#ifdef aiopro_usb
		if ((fp = aio_file_open ( UFS_PATH , O_CREAT | O_APPEND | O_WRONLY, 0660)) == NULL)
#endif
		{
			printk (KERN_INFO "{AIOPro}[Print/UFS] fwrite: file_open error (%s)\n", UFS_PATH);
			scsi_fo=1;
			return 1;
		}

                //get memory space for string                                                                                                                    
                temp = (char*)vmalloc( MEMMAX * sizeof(char));
                memset(temp, 0, MEMMAX);

                //until printed log size becomes full size                                                                                                       
                while(i < size){
                        //make a single line of log at tmp string                                                                                                
                        memset(tmp, 0, TMPMAX);

			itime = scsi_log->itv_sec[i] * 1000000000;
			itime += scsi_log->itv_nsec[i];

		if(scsi_log->st_sec[i] > 0){
			if(scsi_log->func[i] == 920 || scsi_log->func[i] == 921)
			sprintf(tmp, "%d %ld %llu %d %d %d 9 %u %u 3 %u 0 %lu %lu %lu %s\n",
				scsi_log->st_sec[i], scsi_log->st_nsec[i], itime,
				scsi_log->uuid[i], scsi_log->pid[i], scsi_log->tid[i],
				scsi_log->rw[i], scsi_log->func[i], scsi_log->fd[i],
				scsi_log->count[i], scsi_log->ino_no[i], 
				scsi_log->lba[i], scsi_log->fname[i] );
				
			else
			sprintf(tmp, "%d %ld %llu %d %d %d 8 %u %u 3 0 0 %lu %lu %lu %s\n",
				scsi_log->st_sec[i], scsi_log->st_nsec[i], itime,
				scsi_log->uuid[i], scsi_log->pid[i], scsi_log->tid[i],
				scsi_log->rw[i], scsi_log->func[i], 
				scsi_log->count[i], scsi_log->ino_no[i], 
				scsi_log->lba[i], scsi_log->fname[i]);
		}
                        //if tmp + temp is larger than MEMMAX, then flush temp and initialize it                                                                 
                        if( (len + strlen(tmp)) > MEMMAX ){
                                aio_file_write (fp, fpl, temp, strlen(temp));
                                fpl += (strlen(temp));
                                len = 0;
                                memset(temp, 0, MEMMAX);
                        }

                        //copy tmp to temp at the tail part                                                                                                      
                        memcpy(temp+len, &tmp, strlen(tmp)+1);
                        len += (strlen(tmp));
                        i++;

#ifdef aiopro_switch
                        if(aiopro_init() != 1){
                                if( i%50000 == 0)
                                        printk("{AIOPro}[Print/UFS] Writing logs after stopping loggging is processing (%lld/%lld)\n",i,(size-1));
                		if( i == (size-1))
                                        printk("{AIOPro}[Print/UFS] Finish writing logs after stopping loggging (%lld)\n",i);
                                continue;
                        } //skip update MEM log info part, cuz AIOPro stopped logging                                                                            
#endif
                        //update MEM log and its size 
                        scsi_log = get_scsi_log();
                        size = scsi_log->size;
                }

                //Arrive at the end of logs, so flush stacked logs at once
		aio_file_write (fp, fpl, temp, strlen(temp));
       		//set log size at zero
                set_scsi_size(0);
                //this is for wait interval
		scsicnt = 0;
       	       //free string 
               	vfree(temp);
                //close file 
	if(fp != NULL) aio_file_sync(fp);
       	        aio_file_close (fp);

        	if(aiopro_init() == 1)
			return 1;

		for(j=0 ; j<8 ; j++){
			i = 0;
			scsi_log = get_tclog(j);
			size = scsi_log->size;
			printk("{AIOPro}[PRINT/TC%d] Start writing data to file (%lld)\n",j,size);
	
        	        while(i < size){
				if(scsi_log->func[i] < 500) print_tc(j,i,4);
				else if(scsi_log->func[i] < 600) print_tc(j,i,5);
				else if(scsi_log->func[i] < 700) print_tc(j,i,6);
				else if(scsi_log->func[i] < 800) print_tc(j,i,7);
				else if(scsi_log->func[i] < 900) print_tc(j,i,8);
				else print_tc(j,i,9);

				scsi_log = get_tclog(j);
				size = scsi_log->size;
				i++;
			}
			printk("{AIOPro}[PRINT/TC%d] Finish writing data to file (%lld)\n",j,size);
		}
		set_tc_size();

        }
    	else{
        	if(scsicnt == 0){
                	do_posix_clock_monotonic_gettime(&scsibb);
		        befscsi = size;
                        scsicnt = 1;
                }
                else{
                        do_posix_clock_monotonic_gettime(&scsicc);
                        if( (scsicc.tv_sec - scsibb.tv_sec) > WAITV){
                                if(  (size - befscsi) < 10 ){
                                        printk("{AIOPro}[Print/UFS] Waiting time is over predefined maximum time[%d]. Start writing log at file. (%lld)\n",WAITV, size);
                                        scsicnt = -1;
                		}
				else{
	                        	printk("{AIOPro}[Print/UFS] Logging... (%lld)\n", size);
         			        scsicnt = 0;
		                }
                        }
                }
        }
#endif
        return 0;
}

int print_ufs_result(void){
#ifdef aiopro_ufs
	struct file* fp = NULL;
	struct aiopro_log *ufs_log; 
	char tmp[TMPMAX], *temp;
	int64_t size, i=0;
	uint64_t len=0, fpl=0, err=0;
	unsigned long long itime = 0;

	//sleep with predifined interval - memory mgmt : 1 sec
	msleep(1000);

	//get log and its size 
	ufs_log = get_ufs_log(); 
	size = ufs_log->size;
 
#ifdef aiopro_switch
	//if logging stopped before
	if(aiopro_init() != 1){ 
		// and its size is smaller than 2, then it was finished normally
		if( size < 2)
			return 1;
		else{
			printk("{AIOPro}[PRINT/FS] Prepare for writing data to file after stopping logging [%lld]\n",size);
		        ufscnt = -1;
	        	msleep(2500);
        	}
	}
#endif

	//if size is larger than predefined value 1000 or thread waited more than predefined value WAITV, start writing
	if( size > LOGMAX || ufscnt == -1){
		printk("{AIOPro}[PRINT/FS] Start writing data to file (%lld)\n",size);
	
		//check if log size was too small to store previous occured I/O log
		err = get_ufs_err();
		if(err > 0 && err > ufsperr){ 
			printk("{AIOPro}[Print/FS] logging size is over predefined maximum log size~!!! (%llu)\n", size);
			ufsperr = err;
		}

		//open file 
#ifdef aiopro_ubuntu
                if ((fp = aio_file_open ( FS_PATH , O_CREAT | O_APPEND | O_WRONLY, 0666)) == NULL)
#endif
#ifdef aiopro_s6
		if ((fp = aio_file_open ( FS_PATH , O_CREAT | O_APPEND | O_WRONLY, 0666)) == NULL)
#endif 
#ifdef aiopro_sdcard
		if ((fp = aio_file_open ( FS_PATH , O_CREAT | O_APPEND | O_WRONLY, 0666)) == NULL) 
#endif
#ifdef aiopro_usb
		if ((fp = aio_file_open ( FS_PATH , O_CREAT | O_APPEND | O_WRONLY, 0660)) == NULL)
#endif
		{ 
			printk (KERN_INFO "{AIOPro}[Print/FS] fwrite: file_open error (%s)\n", FS_PATH);
			ufs_fo=1;
			return 1;
		}

		//get memory space for string  
		temp = (char*)vmalloc( MEMMAX * sizeof(char));
		memset(temp, 0, MEMMAX);

		//until printed log size becomes full size
		while(i < size){	
			//make a single line of log at tmp string
			memset(tmp, 0, TMPMAX);
			itime = ufs_log->itv_sec[i] * 1000000000;
			itime += ufs_log->itv_nsec[i];

			if( strlen(ufs_log->fname[i]) > 0 && ufs_log->st_sec[i] > 0){
				if(ufs_log->func[i] > 700)
				sprintf(tmp, "%d %ld %llu %d %d %d 9 %u %u %d 0 %lld %lu %lu %lu %s\n",
					ufs_log->st_sec[i], ufs_log->st_nsec[i], itime, 
					ufs_log->uuid[i],   ufs_log->pid[i],     ufs_log->tid[i],
					ufs_log->rw[i], ufs_log->func[i],
					ufs_log->flag[i], ufs_log->pos[i], 
					ufs_log->count[i], ufs_log->ino_no[i], 
					ufs_log->lba[i], ufs_log->fname[i]);
				else
				sprintf(tmp, "%d %ld %llu %d %d %d 6 %u %u %d 0 %lld %lu %lu %lu %s\n",
					ufs_log->st_sec[i], ufs_log->st_nsec[i], itime,
					ufs_log->uuid[i],   ufs_log->pid[i],     ufs_log->tid[i],
					ufs_log->rw[i], ufs_log->func[i],
					ufs_log->flag[i], ufs_log->pos[i], 
					ufs_log->count[i], ufs_log->ino_no[i], 
					ufs_log->lba[i], ufs_log->fname[i]);
			}

			//if tmp + temp is larger than MEMMAX, then flush temp and initialize it
			if( (len + strlen(tmp)) > MEMMAX ){
				aio_file_write (fp, fpl, temp, strlen(temp));
				fpl += (strlen(temp));
				len = 0;
				memset(temp, 0, MEMMAX);
			} 
			
			//copy tmp to temp at the tail part	
			memcpy(temp+len, &tmp, strlen(tmp)+1);
			len += (strlen(tmp));
			i++;

#ifdef aiopro_switch
			if(aiopro_init() != 1){
				if( i%50000 == 0)
					printk("{AIOPro}[Print/FS] Writing logs after stopping loggging is processing (%lld/%lld)\n",i,(size-1));
		                if( i == (size-1)){
					printk("{AIOPro}[Print/FS] Finish writing logs after stopping loggging (%lld)\n",i);
					//print_finfo();
				}
				continue;
			} //skip update MEM log info part, cuz AIOPro stopped logging
#endif				

			//update MEM log and its size	
			ufs_log = get_ufs_log(); 
			size = ufs_log->size; 
		}

		//Arrive at the end of logs, so flush stacked logs at once
		aio_file_write (fp, fpl, temp, strlen(temp));
		//set log size at zero 
		set_ufs_size(0);
		//free string
		vfree(temp);
		//this is for wait interval
		ufscnt = 0;
		//if we need to flush log with sync, enable below line
		//if ((ret = file_sync (fp)) != 0) printk (KERN_INFO "{AIOPro} file sync error (%s)\n", SYS_PATH);
		//close file
	if(fp != NULL) aio_file_sync(fp);
		aio_file_close (fp);
	}
    else{
        if(ufscnt == 0){
			do_posix_clock_monotonic_gettime(&ufsbb);
            befufs = size;
			ufscnt = 1;
		}
		else{
			do_posix_clock_monotonic_gettime(&ufscc);	
			if( (ufscc.tv_sec - ufsbb.tv_sec) > WAITV){
                if(  (size - befufs) < 10 ){
    				printk("{AIOPro}[Print/FS] Waiting time is over predefined maximum time[%d]. Start writing log at file. (%lld)\n",WAITV, size);
	    			ufscnt = -1;
                }
                else{
    				printk("{AIOPro}[Print/FS] Logging... (%lld)\n", size);
                    ufscnt = 0;
                } 
			}
		}
	}
#endif
	return 0;
}

int print_bio_result(void){
	int64_t size, i=0;
	uint64_t len, fpl, err;
	char tmp[TMPMAX];
	unsigned long long itime;

#ifdef aiopro_bio
	struct aiopro_log *bio_log; 
	struct file* fp = NULL;
	char *temp;
#endif
	size = i = len = fpl = err = 0;
	strcpy(tmp, "a");
	msleep(1000);

#ifdef aiopro_bio
	bio_log = get_bio_log(); 
	size = bio_log->size; 

#ifdef aiopro_switch
	//if logging stopped before
	if(aiopro_init() != 1){ 
		// and its size is smaller than 2, then it was finished normally
		if( size < 2 )
			return 1;
		else{
			printk("{AIOPro}[PRINT/BIO] Prepare for writing data to file after stopping logging [%lld]\n",size);
            biocnt = -1;
	        msleep(1500);
        }
	}
#endif

	if( size > BLOGMAX || biocnt == -1){	
			printk("{AIOPro}[PRINT/BIO] Start writing data to file (%lld)\n",size);

		err = get_bio_err();
		if(err > 0 && err > bioperr){ 
			printk("{AIOPro}[Print/BIO] logging size is over predefined maximum log size~!!! (%llu)\n", size);
			bioperr = err;
		}
#ifdef aiopro_ubuntu
                if ((fp = aio_file_open ( BIO_PATH , O_CREAT | O_APPEND | O_WRONLY, 0666)) == NULL)
#endif
#ifdef aiopro_s6
		if ((fp = aio_file_open ( BIO_PATH , O_CREAT | O_APPEND | O_WRONLY, 0666)) == NULL)
#endif 
#ifdef aiopro_sdcard
		if ((fp = aio_file_open ( BIO_PATH , O_CREAT | O_APPEND | O_WRONLY, 0666)) == NULL) 
#endif
#ifdef aiopro_usb
		if ((fp = aio_file_open ( BIO_PATH , O_CREAT | O_APPEND | O_WRONLY, 0660)) == NULL)
#endif 
		{
			printk (KERN_INFO "{AIOPro}[Print/BIO] fwrite: file_open error (%s)\n", BIO_PATH);
			bio_fo=1;
			return 1;
		}

		temp = (char*)vmalloc( MEMMAX * sizeof(char));
		memset(temp, 0, MEMMAX);

		while( i < size ){
			
			memset(tmp, 0, TMPMAX);
			itime = bio_log->itv_sec[i] * 1000000000;
			itime += bio_log->itv_nsec[i];

			if(strlen (bio_log->fname[i]) > 0 && bio_log->st_sec[i] > 0)
			sprintf(tmp, "%d %ld %llu %d %d %d 7 %u %u %u 0 0 %lu %lu %lu %s\n",
				bio_log->st_sec[i], bio_log->st_nsec[i], itime,
				bio_log->uuid[i],   bio_log->pid[i],     bio_log->tid[i],
				bio_log->rw[i], bio_log->func[i], bio_log->flag[i],
				bio_log->count[i], bio_log->ino_no[i], 
				bio_log->lba[i], bio_log->fname[i]);

			if( (len + (strlen(tmp)+1)) > MEMMAX ){
				aio_file_write (fp, fpl, temp, (strlen(temp)));
				fpl += (strlen(temp));
				len = 0;
				memset(temp, 0, MEMMAX);
			} 
				
			//copy tmp to temp at the tail part	
			memcpy(temp+len, &tmp, strlen(tmp)+1);
			len += (strlen(tmp));
			i++;

#ifdef aiopro_switch
			if(aiopro_init() != 1){
				if( i%50000 == 0)
					printk("{AIOPro}[Print/BIO] Writing logs after stopping loggging is processing (%lld/%lld)\n",i,(size-1));
                if( i == (size-1))
					printk("{AIOPro}[Print/BIO] Finish writing logs after stopping loggging (%lld)\n",i);
				continue;
			} //skip update MEM log info part, cuz AIOPro stopped logging
#endif				

			//update MEM log and its size	
			bio_log = get_bio_log(); 
			size = bio_log->size; 

		}
		aio_file_write (fp, fpl, temp, (strlen(temp)));
		fpl += strlen(temp);  //for bio2
		len=0;                //for bio2
		set_bio_size(0);
		vfree(temp);
		biocnt = 0;
	if(fp != NULL) aio_file_sync(fp);
		aio_file_close (fp);
	}
    else{
        if(biocnt == 0){
			do_posix_clock_monotonic_gettime(&biobb);
            befbio = size;
			biocnt = 1;
		}
		else{
			do_posix_clock_monotonic_gettime(&biocc);	
			if( (biocc.tv_sec - biobb.tv_sec) > WAITV){
                if(  (size - befbio) < 10 ){
    				printk("{AIOPro}[Print/BIO] Waiting time is over predefined maximum time[%d]. Start writing log at file. (%lld)\n",WAITV, size);
	    			biocnt = -1;
                }
                else{
    				printk("{AIOPro}[Print/BIO] Logging... (%lld)\n", size);
                    biocnt = 0;
                }
			}
		}
	}
#endif

	return 0;
}


int print_mm_result(void){
#ifdef aiopro_mm
	struct file* fp = NULL;
	struct aiopro_log *mm_log; 
	char tmp[TMPMAX], *temp;
	int64_t size, i=0;
	uint64_t len=0, fpl=0, err=0;
	int temp_type;
	unsigned long long itime;

	//sleep with predifined interval - memory mgmt : 1 sec
	msleep(1000);

	//get log and its size 
	mm_log = get_mm_log(); 
	size = mm_log->size;
 
#ifdef aiopro_switch
	//if logging stopped before
	if(aiopro_init() != 1){ 
		// and its size is smaller than 2, then it was finished normally
		if( size < 2)
			return 1;
		else{
			printk("{AIOPro}[PRINT/MM] Prepare for writing data to file after stopping logging [%lld]\n",size);
            	mmcnt = -1;
	        msleep(1000);
        	}

	}
#endif

	//if size is larger than predefined value 1000 or thread waited more than predefined value WAITV, start writing
	if( size > LOGMAX || mmcnt == -1){	
			printk("{AIOPro}[PRINT/MM] Start writing data to file (%lld)\n",size);

		//check if log size was too small to store previous occured I/O log
		err = get_mm_err();
		if(err > 0 && err > mmperr){ 
			printk("{AIOPro}[Print/MM] logging size is over predefined maximum log size~!!! (%llu)\n", size);
			mmperr = err;
		}

		//open file 
#ifdef aiopro_ubuntu
		if ((fp = aio_file_open ( MM_PATH , O_CREAT | O_APPEND | O_WRONLY, 0666)) == NULL)
#endif 
#ifdef aiopro_s6
		if ((fp = aio_file_open ( MM_PATH , O_CREAT | O_APPEND | O_WRONLY, 0666)) == NULL)
#endif 
#ifdef aiopro_sdcard
		if ((fp = aio_file_open ( MM_PATH , O_CREAT | O_APPEND | O_WRONLY, 0666)) == NULL) 
#endif
#ifdef aiopro_usb
		if ((fp = aio_file_open ( MM_PATH , O_CREAT | O_APPEND | O_WRONLY, 0660)) == NULL)
#endif
		{ 
			printk (KERN_INFO "{AIOPro}[Print/MM] fwrite: file_open error (%s)\n", MM_PATH);
			mm_fo=1;
			return 1;
		}

		//get memory space for string  
		temp = (char*)vmalloc( MEMMAX * sizeof(char));
		memset(temp, 0, MEMMAX);

		//until printed log size becomes full size
		while(i < size){	
			//convert I/O type from function
			if(mm_log->func[i] >= 505)
				temp_type = 1;
			else
				temp_type = 0;

			//make a single line of log at tmp string
			memset(tmp, 0, TMPMAX);
			itime = mm_log->itv_sec[i] * 1000000000;
			itime += mm_log->itv_nsec[i];

			if(strlen(mm_log->fname[i]) > 0 && mm_log->st_sec[i] > 0)
			sprintf(tmp, "%d %ld %llu %d %d %d 5 %u %u %u 0 %lld %lu %lu 1 %s\n",
				mm_log->st_sec[i], mm_log->st_nsec[i], itime,
				mm_log->uuid[i],   mm_log->pid[i],     mm_log->tid[i],
				temp_type, mm_log->func[i], mm_log->flag[i],
				mm_log->pos[i], mm_log->count[i], mm_log->ino_no[i], 
				mm_log->fname[i]);

			//if tmp + temp is larger than MEMMAX, then flush temp and initialize it
			if( (len + strlen(tmp)) > MEMMAX ){
				aio_file_write (fp, fpl, temp, strlen(temp));
				fpl += (strlen(temp));
				len = 0;
				memset(temp, 0, MEMMAX);
			} 
			
			//copy tmp to temp at the tail part	
			memcpy(temp+len, &tmp, strlen(tmp)+1);
			len += (strlen(tmp));
			i++;

#ifdef aiopro_switch
			if(aiopro_init() != 1){
				if( i%50000 == 0)
					printk("{AIOPro}[Print/MM] Writing logs after stopping loggging is processing (%lld/%lld)\n",i,(size-1));
		                if( i == (size-1))
					printk("{AIOPro}[Print/MM] Finish writing logs after stopping loggging (%lld)\n",i);
				continue;
			} //skip update MEM log info part, cuz AIOPro stopped logging
#endif				

			//update MEM log and its size	
			mm_log = get_mm_log(); 
			size = mm_log->size; 
		}

		//Arrive at the end of logs, so flush stacked logs at once
		aio_file_write (fp, fpl, temp, strlen(temp));
		//set log size at zero 
		set_mm_size(0);
		//free string
		vfree(temp);
		//this is for wait interval
		mmcnt = 0;
		//if we need to flush log with sync, enable below line
		//if ((ret = file_sync (fp)) != 0) printk (KERN_INFO "{AIOPro} file sync error (%s)\n", SYS_PATH);
		//close file
	if(fp != NULL) aio_file_sync(fp);
		aio_file_close (fp);
	}
    else{
        if(mmcnt == 0){
			do_posix_clock_monotonic_gettime(&mmbb);
            befmm = size;
			mmcnt = 1;
		}
		else{
			do_posix_clock_monotonic_gettime(&mmcc);	
			if( (mmcc.tv_sec - mmbb.tv_sec) > WAITV){
                if(  (size - befmm) < 10 ){
    				printk("{AIOPro}[Print/MM] Waiting time is over predefined maximum time[%d]. Start writing log at file. (%lld)\n",WAITV, size);
	    			mmcnt = -1;
                }
                else{
    				printk("{AIOPro}[Print/MM] Logging... (%lld)\n", size);
                    mmcnt = 0;
                }
			}
		}
	}
#endif
	return 0;
}

int print_sys_result(void){
#ifdef aiopro_sys
	struct file* fp = NULL;
	struct file* fp2 = NULL;
	struct aiopro_log *sys_log; 
	struct aiopro_log *sys2_log; 
	char tmp[TMPMAX], *temp, *temp2;
	int64_t size, i=0;
	uint64_t len=0, fpl=0, err=0;
	unsigned long long itime;

	msleep(500);

	sys_log = get_sys_log(0); 
	size = sys_log->size; 

#ifdef aiopro_switch
	//if logging stopped before
	if(aiopro_init() != 1){ 
		// and its size is smaller than 2, then it was finished normally
		if( size < 2)
			goto SYSW;
		else{
			printk("{AIOPro}[PRINT/SYSR] Prepare for writing data to file after stopping logging [%lld]\n",size);
            syscnt = -1;
        }
	}
#endif

	if( size > LOGMAX || syscnt == -1 ){	
			printk("{AIOPro}[PRINT/SYSR] Start writing data to file (%lld)\n",size);

		err = get_sys_err(0);
		if(err > 0 && err > sysperr){ 
			printk("{AIOPro}[Print/SYSR] logging size is over predefined maximum log size~!!! (%llu)\n", size);
			sysperr = err;
		}
#ifdef aiopro_ubuntu
		if ((fp = aio_file_open ( SYS_PATH , O_CREAT | O_APPEND | O_WRONLY, 0666)) == NULL)
#endif 
#ifdef aiopro_s6
		if ((fp = aio_file_open ( SYS_PATH , O_CREAT | O_APPEND | O_WRONLY, 0666)) == NULL)
#endif 
#ifdef aiopro_sdcard
		if ((fp = aio_file_open ( SYS_PATH , O_CREAT | O_APPEND | O_WRONLY, 0666)) == NULL) 
#endif
#ifdef aiopro_usb
		if ((fp = aio_file_open ( SYS_PATH , O_CREAT | O_APPEND | O_WRONLY, 0660)) == NULL)
#endif
		{
			printk (KERN_INFO "{AIOPro}[Print/SYSR] fwrite: file_open error (%s)\n", SYS_PATH);
			sys_fo=1;
			return 1;
		}

		temp = (char*)vmalloc( MEMMAX * sizeof(char));
		memset(temp, 0, MEMMAX);

		while( i < size ){
		memset(tmp, 0, TMPMAX);

		itime = sys_log->itv_sec[i] * 1000000000;
		itime += sys_log->itv_nsec[i];

		if(strlen(sys_log->fname[i]) > 0 && sys_log->st_sec[i] > 0)
			sprintf(tmp, "%d %ld %llu %d %d %d 4 0 %u %u %u %lld %lu %lu 1 %s\n",
				sys_log->st_sec[i], sys_log->st_nsec[i], itime,  
				sys_log->uuid[i], sys_log->pid[i], sys_log->tid[i],
				sys_log->func[i], sys_log->flag[i], sys_log->fd[i],
				sys_log->pos[i], sys_log->count[i], 
				sys_log->ino_no[i], sys_log->fname[i]);

			if( (len + strlen(tmp) + 2) > MEMMAX ){
				aio_file_write (fp, fpl, temp, strlen(temp));
				fpl += (strlen(temp));
//printk("{AIOPro}[SYS/READ] Write log data size [%ld](%d)\n ",strlen(temp),i);
				len = 0;
				memset(temp, 0, MEMMAX);
			} 

			//copy tmp to temp at the tail part	
			memcpy(temp+len, &tmp, strlen(tmp)+1);
			len += (strlen(tmp));
	    	i++;

#ifdef aiopro_switch
			if(aiopro_init() != 1){
				if( i%50000 == 0)
					printk("{AIOPro}[Print/SYSR] Writing logs after stopping loggging is processing (%lld/%lld)\n",i,(size-1));
                if( i == (size-1))
					printk("{AIOPro}[Print/SYSR] Finish writing logs after stopping loggging (%lld)\n",i);

				continue;
			} //skip update MEM log info part, cuz AIOPro stopped logging
#endif				

			//update MEM log and its size	
			sys_log = get_sys_log(0); 
			size = sys_log->size; 
		}

		aio_file_write (fp, fpl, temp, strlen(temp));
		set_sys_size(0, 0);
		fpl += strlen(temp);  //for sys2
		len=0;                //for sys2
		syscnt = 0;
		vfree(temp);
	if(fp != NULL) aio_file_sync(fp);
		aio_file_close (fp);

	}
	else{
		if(syscnt == 0){
			do_posix_clock_monotonic_gettime(&sysbb);
            befsys1 = size;
			syscnt = 1;
		}
		else{
			do_posix_clock_monotonic_gettime(&syscc);	
			if( (syscc.tv_sec - sysbb.tv_sec) > WAITV){
                if(  (size - befsys1) < 10 ){
    				printk("{AIOPro}[Print/SYSR] Waiting time is over predefined maximum time[%d]. Start writing log at file. (%lld)\n",WAITV, size);
	    			syscnt = -1;
                }
                else{
    				printk("{AIOPro}[Print/SYSR] Logging... (%lld)\n", size);
                    syscnt = 0;
                }
			}
		}
	}
SYSW:
	sys2_log = get_sys_log(1); 
	size = sys2_log->size; 

#ifdef aiopro_switch
	//if logging stopped before
	if(aiopro_init() != 1){ 
		// and its size is smaller than 2, then it was finished normally
		if( size < 2)
			return 1;
		else{
			printk("{AIOPro}[PRINT/SYSW] Prepare for writing data to file after stopping logging [%lld]\n",size);
            sys2cnt = -1;
        }
	}
#endif

	if( size > LOGMAX || sys2cnt == -1 ){	
			printk("{AIOPro}[PRINT/SYSW] Start writing data to file (%lld)\n",size);

		err = get_sys_err(1);
		if(err > 0 && err > sys2perr){ 
			printk("{AIOPro}[Print/SYSW] logging size is over predefined maximum log size~!!! (%llu)\n", size);
			sys2perr = err;
		}
#ifdef aiopro_ubuntu
		if ((fp2 = aio_file_open ( SYS2_PATH , O_CREAT | O_APPEND | O_WRONLY, 0666)) == NULL)
#endif 
#ifdef aiopro_s6
		if ((fp2 = aio_file_open ( SYS2_PATH , O_CREAT | O_APPEND | O_WRONLY, 0666)) == NULL)
#endif 
#ifdef aiopro_sdcard
		if ((fp2 = aio_file_open ( SYS2_PATH , O_CREAT | O_APPEND | O_WRONLY, 0666)) == NULL) 
#endif
#ifdef aiopro_usb
		if ((fp2 = aio_file_open ( SYS2_PATH , O_CREAT | O_APPEND | O_WRONLY, 0660)) == NULL)
#endif
		{
			printk (KERN_INFO "{AIOPro}[Print/SYSW] fwrite: file_open error (%s)\n", SYS_PATH);
			sys_fo=1;
			return 1;
		}

		temp2 = (char*)vmalloc( MEMMAX * sizeof(char));
		memset(temp2, 0, MEMMAX);
        
        	i=0;
		while( i < size ){
		memset(tmp, 0, TMPMAX);

		itime = sys2_log->itv_sec[i] * 1000000000;
		itime += sys2_log->itv_nsec[i];

		if(sys2_log->func[i] == 418 && sys2_log->st_sec[i] > 0)
			sprintf(tmp, "%d %ld %llu %d %d %d 4 %lu %u 3 %u %lld 0 %lu 1 %s\n",
				sys2_log->st_sec[i], sys2_log->st_nsec[i], itime,  
				sys2_log->uuid[i], sys2_log->pid[i], sys2_log->tid[i],
				sys2_log->count[i],
				sys2_log->func[i], sys2_log->fd[i], 
				sys2_log->pos[i],  
				sys2_log->ino_no[i], sys2_log->fname[i]);
		else if( strlen(sys2_log->fname[i]) > 0 && sys2_log->st_sec[i] > 0 )
			sprintf(tmp, "%d %ld %llu %d %d %d 4 1 %u %u %u %lld %lu %lu 1 %s\n",
				sys2_log->st_sec[i], sys2_log->st_nsec[i], itime,  
				sys2_log->uuid[i], sys2_log->pid[i], sys2_log->tid[i],
				sys2_log->func[i], sys2_log->flag[i], sys2_log->fd[i], 
				sys2_log->pos[i], sys2_log->count[i], 
				sys2_log->ino_no[i], sys2_log->fname[i]);

			if( (len + strlen(tmp)) > MEMMAX ){
				aio_file_write (fp2, fpl, temp2, strlen(temp2));
				fpl += (strlen(temp2));
//printk("{AIOPro}[SYS/WRITE] Write log data size [%ld] ",strlen(temp));
				len = 0;
				memset(temp2, 0, MEMMAX);
			} 
				
			//copy tmp to temp at the tail part	
			memcpy(temp2+len, &tmp, strlen(tmp)+1);
			len += (strlen(tmp));
			i++;

#ifdef aiopro_switch
			if(aiopro_init() != 1){
				if( i%50000 == 0)
					printk("{AIOPro}[Print/SYSW] Writing logs after stopping loggging is processing (%lld/%lld)\n",i,(size-1));
                if( i == (size-1))
					printk("{AIOPro}[Print/SYSW] Finish writing logs after stopping loggging (%lld)\n",i);
				continue;
			} //skip update SYSW log info part, cuz AIOPro stopped logging
#endif				

			//update MEM log and its size	
			sys2_log = get_sys_log(1); 
			size = sys2_log->size; 
		}

		aio_file_write (fp2, fpl, temp2, strlen(temp2));
		set_sys_size(1, 0);
		sys2cnt = 0;
		vfree(temp2);
		//	if ((ret = file_sync (fp)) != 0) printk (KERN_INFO "{AIOPro} file sync error (%s)\n", SYS_PATH);
	if(fp2 != NULL) aio_file_sync(fp2);
		aio_file_close (fp2);
	}
	else{
        if(sys2cnt == 0){
			do_posix_clock_monotonic_gettime(&sys2bb);
            befsys2 = size;
			sys2cnt = 1;
		}
		else{
			do_posix_clock_monotonic_gettime(&sys2cc);	
			if( (sys2cc.tv_sec - sys2bb.tv_sec) > WAITV){
                if(  (size - befsys2) < 10 ){
    				printk("{AIOPro}[Print/SYSW] Waiting time is over predefined maximum time[%d]. Start writing log at file. (%lld)\n",WAITV, size);
	    			sys2cnt = -1;
                }
                else{
    				printk("{AIOPro}[Print/SYSW] Logging... (%lld)\n", size);
                    sys2cnt = 0;
                }
			}
		}
	}
#endif
	return 0;
}

///////////////////////////////////////////////////////////kthread mgmt part -- STOP KTHREAD

void stop_aiopro(int type){
	switch(type){
		case 0:
			if(aiopro_sys_task != NULL){
				printk("{AIOPro}[Kthread] >>> TERMINATE <<< kthread for System Call\n");
				kthread_stop(aiopro_sys_task);
				aiopro_sys_task = NULL;
			}
			else
				printk("{AIOPro}[ERROR @ Kthread] sys_task is already NULL?????\n");
			break;
		case 1:
			if(aiopro_mm_task != NULL){
				printk("{AIOPro}[Kthread] >>> TERMINATE <<< kthread for Memory Mgmt\n");
				kthread_stop(aiopro_mm_task);
				aiopro_mm_task = NULL;
			}
			else
				printk("{AIOPro}[ERROR @ Kthread] mm_task is already NULL?????\n");
			break;
		case 2:
			if(aiopro_se_task != NULL){
				printk("{AIOPro}[Kthread] >>> TERMINATE <<< kthread for Start End Logging\n");
				kthread_stop(aiopro_se_task);
				aiopro_se_task = NULL;
			}
			else
				printk("{AIOPro}[ERROR @ Kthread] se_task is already NULL?????\n");
			break;
		case 3:
			if(aiopro_bio_task != NULL){
				printk("{AIOPro}[Kthread] >>> TERMINATE <<< kthread for Block I/O\n");
				kthread_stop(aiopro_bio_task);
				aiopro_bio_task = NULL;
			}
			else
				printk("{AIOPro}[ERROR @ Kthread] bio_task is already NULL?????\n");
			break;
		case 4:
			if(aiopro_scsi_task != NULL){
				printk("{AIOPro}[Kthread] >>> TERMINATE <<< kthread for SCSI\n");
				kthread_stop(aiopro_scsi_task);
				aiopro_scsi_task = NULL;
			}
			else
				printk("{AIOPro}[ERROR @ Kthread] scsi_task is already NULL?????\n");
			break;
		case 5:
			if(aiopro_ufs_task != NULL){
				printk("{AIOPro}[Kthread] >>> TERMINATE <<< kthread for UFS\n");
				kthread_stop(aiopro_ufs_task);
				aiopro_ufs_task = NULL;
			}
			else
				printk("{AIOPro}[ERROR @ Kthread] ufs_task is already NULL?????\n");
			break;

		default:
			printk("{AIOPro}[ERROR @ Kthread] Wrong input for stopping kernel thread!\n");
	}
}EXPORT_SYMBOL(stop_aiopro);
///////////////////////////////////////////////////////////kthread mgmt part -- EXECUTION PART
static int aiopro_sys_thread(void *args){
	msleep(2000);
	printk("{AIOPro}[Kthread] Starting kernel thread for System Call\n");
	while( kthread_should_stop() != 1 && sys_fo != 1 ){
		print_sys_result();
	}
	printk("{AIOPro}[Kthread] <<< STOP >>> kthread for System Call\n");
	return 0;
}

static int aiopro_mm_thread(void *args){
	msleep(2000);
	printk("{AIOPro}[Kthread] Starting kernel thread for Memory mgmt\n");
	while( kthread_should_stop() != 1 && mm_fo != 1 ){
		print_mm_result();
	}
	printk("{AIOPro}[Kthread] <<< STOP >>> kthread for Memory Mgmt\n");
	return 0;
}

static int aiopro_se_thread(void *args){
	msleep(2000);
	printk("{AIOPro}[Kthread] Starting kernel thread for Start End Logging\n");
	while( kthread_should_stop() != 1 && se_fo != 1 ){
		print_se_result();
	}
	printk("{AIOPro}[Kthread] <<< STOP >>> kthread for Start End Logging\n");
	return 0;
}

static int aiopro_bio_thread(void *args){
	msleep(2000);
	printk("{AIOPro}[Kthread] Starting kernel thread for Block I/O\n");
	while( kthread_should_stop() != 1 && bio_fo != 1 ){
		print_bio_result();
	}
	printk("{AIOPro}[Kthread] <<< STOP >>> kthread for Block I/O [%d][%d]\n",kthread_should_stop(),bio_fo);
	return 0;
}

static int aiopro_scsi_thread(void *args){
	msleep(2000);
	printk("{AIOPro}[Kthread] Starting kernel thread for SCSI\n");
	while( kthread_should_stop() != 1 && scsi_fo != 1 ){
		print_scsi_result();
	}
	printk("{AIOPro}[Kthread] <<< STOP >>> kthread for SCSI\n");
	return 0;
}

static int aiopro_ufs_thread(void *args){
	msleep(2000);
	printk("{AIOPro}[Kthread] Starting kernel thread for UFS\n");
	while( kthread_should_stop() != 1 && ufs_fo != 1 ){
		print_ufs_result();
	}
	printk("{AIOPro}[Kthread] <<< STOP >>> kthread for UFS\n");
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////// -- RUN KTHREAD
int init_aiopro_sys(void){   ///////////////////////
	if(aiopro_sys_task == NULL){		
		aiopro_sys_task = kthread_run(aiopro_sys_thread,NULL,"AIOPro_SYS");
		return 1;
	}
	
//	printk("{AIOPro}[ERROR @ Kthread] FAIL to create kernel thread for system call~! It already exists.\n");
	return 0;
}EXPORT_SYMBOL(init_aiopro_sys);

int init_aiopro_mm(void){   /////////////////////// 
	if(aiopro_mm_task == NULL){
		aiopro_mm_task = kthread_run(aiopro_mm_thread,NULL,"AIOPro_MM");
		return 1;
	}

//	printk("{AIOPro}[ERROR @ Kthread] FAIL to create kernel thread for memory mgmt~! It already exists.\n");
	return 0;
}EXPORT_SYMBOL(init_aiopro_mm);

int init_aiopro_se(void){   ///////////////////////
	if(aiopro_se_task == NULL){
		aiopro_se_task = kthread_run(aiopro_se_thread,NULL,"AIOPro_SE");
		return 1;
	}
	
//	printk("{AIOPro}[ERROR @ Kthread] FAIL to create kernel thread for SE~! It already exists.\n");
	return 0;
}EXPORT_SYMBOL(init_aiopro_se);

int init_aiopro_bio(void){   ///////////////////////
	if(aiopro_bio_task == NULL){
		aiopro_bio_task = kthread_run(aiopro_bio_thread,NULL,"AIOPro_BIO");
		return 1;
	}
	
//	printk("{AIOPro}[ERROR @ Kthread] FAIL to create kernel thread for block I/O~! It already exists.\n");
	return 0;
}EXPORT_SYMBOL(init_aiopro_bio);

int init_aiopro_scsi(void){   ///////////////////////
	if(aiopro_scsi_task == NULL){
		aiopro_scsi_task = kthread_run(aiopro_scsi_thread,NULL,"AIOPro_SCSI");
		return 1;
	}
	
//	printk("{AIOPro}[ERROR @ Kthread] FAIL to create kernel thread for SCSI~! It already exists.\n");
	return 0;
}EXPORT_SYMBOL(init_aiopro_scsi);

int init_aiopro_ufs(void){   ///////////////////////
	if(aiopro_ufs_task == NULL){
		aiopro_ufs_task = kthread_run(aiopro_ufs_thread,NULL,"AIOPro_UFS");
		return 1;
	}
	
//	printk("{AIOPro}[ERROR @ Kthread] FAIL to create kernel thread for UFS~! It already exists.\n");
	return 0;
}EXPORT_SYMBOL(init_aiopro_ufs);




