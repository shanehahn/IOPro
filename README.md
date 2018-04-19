# IOPro #

***

- Maintainer: Sangwook Shane Hahn (shanehahn7@gmail.com)
- Reference 1: Sangwook Shane Hahn, Inhyuk Yee, Donguk Ryu and Jihong Kim "AIOPro: A Fully-Integrated Storage I/O Profiler for Android Smartphones", Korea Computer Congress (KCC), 2016. (**Best Papaer Award**)
[(Paper Download)](http://cares.snu.ac.kr/?view=boardF&fileN=102865424058621e58c6f75.data)
- Reference 2: Sangwook Shane Hahn, Inhyuk Yee, Donguk Ryu and Jihong Kim "AIOPro: A Fully-Integrated Storage I/O Profiler for Android Smartphones", Journal of KIISE (JOK), Vol. 44, no. 3, pp. 232-238, 2017.
[(Paper Download)](http://cares.snu.ac.kr/?view=boardF&fileN=65733309659ce01e588cdb.data)
- Reference 3: Sangwook Shane Hahn and Jihong Kim "PMOPro: A Fully-Integrated Storage I/O Profiler for Next-Generation Non-Volatile Memory-Based Storage", Korea Software Congress (KSC), 2017.
[(Paper Download)](http://cares.snu.ac.kr/?view=boardF&fileN=7488647945a1d3d6ca89b2.data)

---

**IOPro** is a fully-integrated storage I/O profiler for Linux. IOPro traces storage I/O from application - system call - virtual file system – native file system - page cache - block layer - device driver. IOPro combines the storage I/O information from I/O layers by linking them with file information and physical address. User can start to log storage I/O information by using sysfs interface. All of collected traces are held in memory buffer temporally until logging process is finished.
IOPro can track storage I/O information from all I/O layers without any data loss under 0.1% system overheads.
IOPro can import the profiling results to the any database. IOPro can also visualize the profiling results with waveform.

All information what IOPro collects are as following:

- System call: I/O start/finish time, I/O type, FD, I/O size, offset, UID, PID, TID
- Virtual/Native file system: I/O start/finish time,  I/O type, file name, inode id, I/O size
- Block I/O layer: I/O start/finish time, I/O type, logical block address, I/O size
- Device driver: I/O start/finish time,  I/O type, logical block address, I/O size, Queue Depth, Interrupt/Polling, Device I/O start/finish time

***

## Overall Architecture of IOPro #

![](https://postfiles.pstatic.net/MjAxODAzMjlfMTEw/MDAxNTIyMjkyMjcxOTAw.dGm4jIzitIntbOuFbWxRl7nn03EkcIqZm7TOTe5IIr0g.sEDOehnPhhROghO_9Z4veJaQT3_bz1E9_qPutCXIRPEg.JPEG.shanehahn/IOPro_OA.jpg?type=w773)

***

## Results of IOPro #

### Visualize I/O profile results using GTKwave #

![](https://postfiles.pstatic.net/MjAxODAzMjlfNjYg/MDAxNTIyMjk0NDMwNDUz.27liGhdTgm0Qi3Dhs2qyDDZl-kdHJf3hVJ_vDw-u4Hgg.XMJVXWc4BNd-wStbYqK5C8x_Uf2qreKFJ7AoNf8DenMg.JPEG.shanehahn/iopro_vis.jpg?type=w773)

![](https://postfiles.pstatic.net/MjAxODAzMjlfMTM5/MDAxNTIyMjk0NjYwOTY0.qAYauQi9XbIA0sZ6mgBqSjPr9Xx6qsh3QM0JxsTe60kg.ctveQhRiNVPpZYjZTPg3p38GI5gAEO_OOH2eeayQJRQg.JPEG.shanehahn/iopro_vis2.jpg?type=w773)


### Import I/O profile results to MySQL #

![](https://postfiles.pstatic.net/MjAxODAzMjlfMzkg/MDAxNTIyMjk0NDI2MjYw.KkwJws3TP3n_jH2wGcdHz6CcCNy0l33IsMRtYCOwJdMg.Z_DVLvufUafFgfjpSAud6SUnK10ZO0h-Bs1p_-aP7Eog.JPEG.shanehahn/iopro_db.jpg?type=w773)

 


***
## Kernel Modification ###

- Support Linux Kernel Version: 3.10 (Desktop, Galaxy S6, Nexus 5, Nexus 6), 3.11 (Desktop, pmfs), 3.19 (Desktop), 4.4 (Desktop)
- System call: 
  + fs/read_write.c 
  + fs/sync.c 
  + fs/internal.h
- Virtual/Native file system: 
  + fs/direct-io.c 
  + fs/ext4/file.c 
  + fs/sdcardfs/file.c 
  + mm/filemap.c
- Block I/O layer: 
  + block/blk-core.c 
  + block/blk-softirq.c 
  + block/blk.h 
  + block/cfq-iosched.c
- Device driver: 
  + drivers/scsi/scsi.c 
  + drivers/scsi/scsi.h 
  + drivers/scsi/scsi_lib.c
  + drivers/mmc/card/block.c (for eMMC driver)
  + drivers/mmc/core/core.c (for eMMC driver)
  + driver/scsi/ufs/ufshcd.c (for UFS driver)
  + drivers/block/nvme-core.c (for Linux 3.x NVMe driver)
  + drivers/nvme/host/pci.c (for Linux 4.x NVMe driver)
- IOPro main module: 
  + fs/aiopro.c (newly added file) 
  + fs/Makefile 
  + init/main.c
- Library and header: 
  + include/linux/fs.h 
  + include/linux/blk_types.h 

***

## How to use IOPro ###

- Set location for log files at fs/aiopro.c
- Kernel compile and reboot 
- Start IOPro: echo 1 > /sys/kernel/debug/iopro
- Stop IOPro: echo 0 > /sys/kernel/debug/iopro


2018-03-28, Sangwook Shane Hahn, shanehahn7@gmail.com
