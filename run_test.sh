#!/bin/bash


echo "running ./pagesim -a \"LRU\" -x 10"
for i in {1..10}; do
	./pagesim -a " LRU" -x 10 >> pagesim_1024.log
done

echo "running ./pagesim -a \"LRU\" -x 11"
for i in {1..10}; do
	./pagesim -a " LRU" -x 11 >> pagesim_2048.log
done

echo "running ./pagesim -a \"LRU\" -x 12"
for i in {1..10}; do
	./pagesim -a " LRU" -x 12 >> pagesim_4096.log
done

echo "running ./pagesim -a \"LRU\" -x 13"
for i in {1..10}; do
	./pagesim -a "LRU" -x 13 >> pagesim_8192.log
done


echo "running ./pagesim -a \"LRU\" -x 14"
for i in {1..10}; do
	./pagesim -a "LRU" -x 14 >> pagesim_16384.log
done


echo "running ./pagesim -a \"LRU\" -x 15"
for i in {1..10}; do
	./pagesim -a "LRU" -x 15 >> pagesim_32768.log
done

echo "running ./pagesim -a \"LRU\" -x 16"
for i in {1..10}; do
	./pagesim -a "LRU" -x 16 >> pagesim_65536.log
done

echo "running ./pagesim -a \"LRU\" -x 17"
for i in {1..10}; do
	./pagesim -a "LRU" -x 17 >> pagesim_131072.log
done

echo "running ./pagesim -a \"LRU\" -x 18"
for i in {1..10}; do
	./pagesim -a "LRU" -x 18 >> pagesim_262144.log
done
