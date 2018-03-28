#./unmkbootimg -i boot/boot.img --ramdisk boot/ramdisk.cpio.gz #--kernel boot/zImage
cd mkbootimg
./unmkbootimg -i boot.img

cp ../arch/arm/boot/zImage-dtb .

./mkbootimg --base 0 --pagesize 2048 --kernel_offset 0x00008000 --ramdisk_offset 0x0000000b --second_offset 0x00f00000 --tags_offset 0x0000000b --cmdline 'console=ttyHSL0,115200,n8 androidboot.console=ttyHSL0 androidboot.hardware=shamu msm_rtb.filter=0x37 ehci-hcd.park=3 utags.blkdev=/dev/block/platform/msm_sdcc.1/by-name/utags utags.backup=/dev/block/platform/msm_sdcc.1/by-name/utagsBackup coherent_pool=8M' --kernel zImage-dtb --ramdisk ramdisk.cpio.gz -o myboot.img

echo "mkbootimg end..."
cd ..

echo "scp mkbootimg/myboot.img shanehahn@hyewon.snu.ac.kr:~/."
cd mkbootimg
sshpass -pgkstkddnr7 scp myboot.img shanehahn@hyewon.snu.ac.kr:~/.
cd ..

#adb reboot bootloader
#fastboot flash boot myboot.img
#sleep 1
#fastboot reboot
#cd ../
