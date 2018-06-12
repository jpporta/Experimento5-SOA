#!/bin/bash
rm teste.txt

for x in {1..10}
do
    echo "teste ${x}" >> teste.txt
    ./exp5 >> teste.txt &
    sleep 5
    kill exp5
done