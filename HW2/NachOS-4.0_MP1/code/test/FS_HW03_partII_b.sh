../build.linux/nachos -f
../build.linux/nachos -mkdir /t20000
../build.linux/nachos -mkdir /t30000
../build.linux/nachos -cp num_1000.txt /t20000/num_1000
# ../build.linux/nachos -cp num_12000.txt /t30000/num_12000
../build.linux/nachos -cp num_2000.txt /t30000/num_2000
echo "======================================="
../build.linux/nachos -lr /
echo "======================================="