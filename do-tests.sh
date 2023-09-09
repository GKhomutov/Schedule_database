#!/bin/bash

test_runner='python3 client.py'
tests_directory=./TESTING/tests
tmp=last_run_results.txt
nfailed=0

cleanup () {
  rm -f $tmp
}

for testfile in `find $tests_directory -type f -name "*.in"`
do
  testname=`basename $testfile .in`
  expected_answer=`dirname $testfile`/${testname}.res
  if [ ! -f $expected_answer ]; then
    echo "*** Test" $testname "does not contain corresponding .res file! ***"
    continue
  fi

  # Run the test
  test_passed=0
  echo -E -n $testname " "
  $test_runner <$testfile >$tmp

  # Check the answer
  diff -abBiq $tmp $expected_answer >/dev/null
  if [ $? -ne 0 ]; then
    test_passed=0
  else
    test_passed=1
  fi

  # Print status
  if [ $test_passed -eq 0 ]; then
    echo -e \\033[31m \\t "FAILED" \\033[0m
    ((nfailed++))
  fi
  if [ $test_passed -eq 1 ]; then
    echo -e \\033[32m \\t "PASSED" \\033[0m
  fi
done

cleanup
exit $nfailed

