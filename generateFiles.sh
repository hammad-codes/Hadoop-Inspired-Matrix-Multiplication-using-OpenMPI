#!/bin/bash

# Check if integer argument is provided
if [ -z "$1" ]; then
  echo "Usage: $0 <integer>"
  exit 1
fi

# Assign integer argument to variable
input=$1

# Check if inputA.txt and inputB.txt files exist
# if [ -f A.txt ] && [ -f B.txt ]; then
#   echo "Both A.txt and B.txt files exist."
#   echo " "
# else
  gcc -o ArrayGenerator ArrayGenerator.c -std=c99
  ./ArrayGenerator $input
  echo "A.txt and B.txt files generated."
  echo " "
  # if output.txt file exists, remove it
  if [ -f output.txt ]; then
    rm -f output.txt
  fi