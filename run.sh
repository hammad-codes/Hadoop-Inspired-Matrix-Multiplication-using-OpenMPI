#!/bin/bash
if [ $# -ne 1 ]; then
  echo "Usage: $0 <integer>"
  exit 1
fi

input=$1

# Generate input files using generateFiles.sh script
# ./generateFiles.sh $input
./generateFiles.sh $input

# Calculate array size and number of processes
if [ $input -le 128 ]; then
  input=$((input+1))
else
  input=129
fi

# Compile hadoop.c with MPI library
mpicc -o hadoop hadoop.c -std=c99

# Create machinefile
if [ -f machinefile ]; then
  rm -f machinefile
fi
touch machinefile

# if input is odd then  Write master:(input/2)+1 and slaves:input/2 to machinefile
if [ $((input%2)) -ne 0 ]; then
  echo "master:$((input/2))" >> machinefile
  echo "slave:$((input/2+1))" >> machinefile
# else if even then Write master:input/2 and slaves:input/2 to machinefile
else
  echo "master:$((input/2-1))" >> machinefile
  echo "slave:$((input/2+1))" >> machinefile

fi
# Run hadoop.c with MPI library
mpiexec -n $input -f machinefile ./hadoop

#If C.txt and output.txt exist
if [ -f C.txt ] && [ -f output.txt ]; then
echo "------------------------"
  # Compare C.txt and output.txt if they are same print "Correct" else print "Wrong"
  if cmp -s C.txt output.txt; then
    echo "Matrix Comparison Function Returned: True"
  else
    echo "Matrix Comparison Function Returned: False"
  fi
fi