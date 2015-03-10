#!/bin/sh

for test in ex_test1 ex_test2 broken TEST1 TEST2 TEST3 TEST4
do
    ./$1 <$test | ./sch_ch $test
    if [ $? -eq 0 ]; then
	echo $test -- OK
    else
	echo $test -- Failed
	exit
    fi
done
