#!/bin/bash


# $1 = -x 10...
# $2 = -h 1...
function run_test()
{
	multi=$1
	hotness=$2
	ref_count=`echo 10*2^${multi} | bc`

	if [ -z ${hotness} ] ; then
		log_file=test3/pagesim_${ref_count}.log
	else
		log_file=test3/pagesim_${ref_count}_${hotness}.log
	fi

for i in {1..10}; do
	if [ -z ${hotness} ] ; then
		./pagesim  -x ${multi} >> ${log_file}
	else
		./pagesim  -x ${multi} -h ${hotness} >> ${log_file}
	fi
done


}

run_test 14 
run_test 14  10
run_test 14  15
run_test 14  20
run_test 14  25
run_test 14  30
run_test 14  35
run_test 14  40
run_test 14  45

