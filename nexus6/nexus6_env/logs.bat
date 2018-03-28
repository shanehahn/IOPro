adb pull /sdcard/test .
adb shell "dmesg" > dmesg.txt
adb shell "dmesg | grep "AIOP"" > aiopro.txt
pscp -scp -pw gkstkddnr7 *.txt shanehahn@hyewon.snu.ac.kr:~/aiopro/nexus6/.
del *.txt