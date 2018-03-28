cmd_fs/pmfs/pmfs_test.ko := ld -r -m elf_x86_64 -T /home/guest/iopro_pmfs/scripts/module-common.lds --build-id  -o fs/pmfs/pmfs_test.ko fs/pmfs/pmfs_test.o fs/pmfs/pmfs_test.mod.o
