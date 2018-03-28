cp *.sh 			../change_for_aiopro/nexus6_aiopro/.
cp init/main.c 			../change_for_aiopro/nexus6_aiopro/init/.
cp fs/aiopro.c 			../change_for_aiopro/nexus6_aiopro/fs/.
cp fs/read_write.c 		../change_for_aiopro/nexus6_aiopro/fs/.
cp fs/sync.c 			../change_for_aiopro/nexus6_aiopro/fs/.
cp fs/direct-io.c 		../change_for_aiopro/nexus6_aiopro/fs/.
cp fs/internal.h 		../change_for_aiopro/nexus6_aiopro/fs/.
cp fs/Makefile 			../change_for_aiopro/nexus6_aiopro/fs/.
cp mm/filemap.c 		../change_for_aiopro/nexus6_aiopro/mm/.
cp block/blk-core.c 		../change_for_aiopro/nexus6_aiopro/block/.
cp drivers/mmc/card/block.c 	../change_for_aiopro/nexus6_aiopro/drivers/mmc/card/.
cp drivers/mmc/core/core.c 	../change_for_aiopro/nexus6_aiopro/drivers/mmc/core/.
cp drivers/scsi/scsi.c	 	../change_for_aiopro/nexus6_aiopro/drivers/scsi/.
cp include/linux/fs.h 		../change_for_aiopro/nexus6_aiopro/include/linux/.
cp include/linux/blk_types.h 	../change_for_aiopro/nexus6_aiopro/include/linux/.
cd ../change_for_aiopro
7za a 6nexus_aiopro.7z nexus6_aiopro
sshpass -pgkstkddnr7 scp 6nexus_aiopro.7z shanehahn@hyewon.snu.ac.kr:~/.
#rm 6nexus_aiopro.7z
#sshpass -pgkstkddnr7 scp boot.7z shanehahn@hyewon.snu.ac.kr:~/.
#rm change_for_aiopro.7z boot.tar boot.7z 
