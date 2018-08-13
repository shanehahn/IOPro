/////////////////////////////////////////////////////////////////////include header part
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

#include <linux/debugfs.h>
#include <linux/fs.h>

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


//////////////////////////////////////////////////////print log for each layer part 
#define SYS_PATH "/home/userid/log_sys.txt"

int print_sys_result(void){
	struct file* fp = NULL;
	struct aiopro_log *sys_log; 
	char *temp;
	int64_t size, i=0;
	uint64_t fpl=0;

	if ((fp = aio_file_open ( SYS_PATH , O_CREAT | O_APPEND | O_WRONLY, 0660)) == NULL){
		printk (KERN_INFO "{IOPro}[Print/SYS] fwrite: file_open error (%s)\n", SYS_PATH);
		return 1;
	}

	temp = (char*)vmalloc( 4096 * sizeof(char));
	memset(temp, 0, 4096);

	//get system call log and its size from /fs/read_write.c
	sys_log = get_sys_log(0); 
	size = sys_log->size; 

	while( i < size){
		//exmaple of real IOPro Log Format
		sprintf(temp, "%d %ld %llu %d %d %d 4 0 %u %u %u %lld %zu %lu 1 %s\n",
			sys_log->st_sec[i], sys_log->st_nsec[i], itime,  
			sys_log->uuid[i], sys_log->pid[i], sys_log->tid[i],
			sys_log->func[i], sys_log->flag[i], sys_log->fd[i],
			sys_log->pos[i], sys_log->count[i], 
			sys_log->ino_no[i], sys_log->fname[i]);

		aio_file_write (fp, fpl, temp, strlen(temp)); //issue buffered write
		fpl += strlen(temp);  //move pointer
		i++
	}

	vfree(temp);
	aio_file_sync(fp); //flush(sync) buffered writes to storage device
	aio_file_close (fp);

	return 0;
}
