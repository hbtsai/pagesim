#!/bin/bash


# $1 = -x 10...
# $2 = -h 1...
function run_test()
{
	multi=$1
	hotness=$2
	ref_count=`echo 10*2^${multi} | bc`

	if [ -z ${hotness} ] ; then
		log_file=test1/pagesim_${ref_count}.log
	else
		log_file=test1/pagesim_${ref_count}_${hotness}.log
	fi

for i in {1..10}; do
	if [ -z ${hotness} ] ; then
		./pagesim  -x ${multi} >> ${log_file}
	else
		./pagesim  -x ${multi} -h ${hotness} >> ${log_file}
	fi
done


}

run_test 10 
run_test 10  10
run_test 10  20
run_test 10  30
run_test 10  40

run_test 11 
run_test 11  10
run_test 11  20
run_test 11  30
run_test 11  40

run_test 12 
run_test 12  10
run_test 12  20
run_test 12  30
run_test 12  40

run_test 13 
run_test 13  10
run_test 13  20
run_test 13  30
run_test 13  40

run_test 14 
run_test 14  10
run_test 14  20
run_test 14  30
run_test 14  40

run_test 15 
run_test 15  10
run_test 15  20
run_test 15  30
run_test 15  40


run_test 16 
run_test 16  10
run_test 16  20
run_test 16  30
run_test 16  40

