#!/bin/bash


# $1 = -x 10...
# $2 = -h 1...
function run_test()
{
	multi=$1
	hotness=$2
	window=$3
	ref_count=`echo 10*2^${multi} | bc`

	log_file=test2/pagesim_${ref_count}_${hotness}_${window}.log

for i in {1..10}; do
		./pagesim  -x ${multi} -h ${hotness} -w ${window} >> ${log_file}
done


}

run_test 14 10 2
run_test 14 10 4
run_test 14 10 9
run_test 14 10 18
run_test 14 10 27
 
run_test 14 15 5
run_test 14 15 10
run_test 14 15 21
run_test 14 15 40
run_test 14 15 60
 
run_test 14 20 10
run_test 14 20 20
run_test 14 20 40
run_test 14 20 80
run_test 14 20 120
 
run_test 14 25 20
run_test 14 25 40
run_test 14 25 79
run_test 14 25 160
run_test 14 25 240

run_test 14 30 35
run_test 14 30 70
run_test 14 30 139
run_test 14 30 280
run_test 14 30 420
 
run_test 14 35 55
run_test 14 35 110
run_test 14 35 223
run_test 14 35 440
run_test 14 35 660

run_test 14 40 90
run_test 14 40 180
run_test 14 40 363
run_test 14 40 720
run_test 14 40 1080


run_test 14 45 135
run_test 14 45 275
run_test 14 45 553
run_test 14 45 1100
run_test 14 45 1650
