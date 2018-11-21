#!/bin/sh

for file in data/*; do echo $file && ./idx2root $file && echo -e "\n\n"; done
