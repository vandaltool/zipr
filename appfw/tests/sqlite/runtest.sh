$TEST_HARNESS_HOME/run_one_test.sh sqlite.test.sh 2>&1 | tee out

echo "Test results"
grep -i TestResult out
