#!/bin/bash

PWD=`pwd`
TEST_DIR=$PWD/tests/
PROGRAM=$1

INVALID_INPUTS=`ls $TEST_DIR | grep INVALID`
VALID_INPUTS=`ls $TEST_DIR | grep -v INVALID`

for INPUT in $INVALID_INPUTS
do
    echo -n "test $INPUT... "
    RESULT=`$PROGRAM < $TEST_DIR/$INPUT`
    if [ "$RESULT" == "INVALID" ]
    then
        echo "passed"
    else
        echo "failed"
        echo "program returns $RESULT, but INVALID expected"
        exit 0
    fi
done

for INPUT in $VALID_INPUTS
do
    echo -n "test $INPUT... "
    EXPECTED=`echo "$INPUT" | tr '[:lower:]' '[:upper:]'`
    RESULT=`$PROGRAM < $TEST_DIR/$INPUT | tr '[:lower:]' '[:upper:]'`
    if [ "$EXPECTED" == "$RESULT" ]
    then
        echo "passed"
    else
        echo "failed"
        echo "program returns $RESULT, but $EXPECTED expected"
        exit 0
    fi
done
