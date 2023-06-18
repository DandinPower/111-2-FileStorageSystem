../build.linux/nachos -f
../build.linux/nachos -mkdir /d1
../build.linux/nachos -mkdir /d2
../build.linux/nachos -mkdir /d3
../build.linux/nachos -mkdir /d2/dd1
../build.linux/nachos -mkdir /d2/dd2
../build.linux/nachos -mkdir /d2/dd2/ddd1
../build.linux/nachos -mkdir /d2/dd2/ddd2
echo "=========== Recursive List ============"
../build.linux/nachos -lr /
echo "=========== Remove /d2 ============"
../build.linux/nachos -r /d2
echo "=========== Recursive List ============"
../build.linux/nachos -lr /