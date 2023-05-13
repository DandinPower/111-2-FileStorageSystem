../build.linux/nachos -f
../build.linux/nachos -mkdir /t20000
../build.linux/nachos -mkdir /t30000
../build.linux/nachos -mkdir /t40000
../build.linux/nachos -mkdir /t50000
../build.linux/nachos -mkdir /t60000
../build.linux/nachos -mkdir /t70000
../build.linux/nachos -mkdir /t20000/t20001
../build.linux/nachos -mkdir /t20000/t20002
../build.linux/nachos -mkdir /t20000/t20003
../build.linux/nachos -mkdir /t20000/t20004
../build.linux/nachos -cp FS_test4 /t20000/t20004/FS_test4
../build.linux/nachos -e /t20000/t20004/FS_test4
echo "=========== Print File3 ============"
../build.linux/nachos -p /t20000/t20004/file3
../build.linux/nachos -cp num_1000.txt /t20000/num_1000
../build.linux/nachos -cp num_12000.txt /t30000/num_12000
echo "=========== Recursive List ============"
../build.linux/nachos -lr /
