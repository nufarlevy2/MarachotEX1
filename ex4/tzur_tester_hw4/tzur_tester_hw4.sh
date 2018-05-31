#!/bin/bash
###Author: Alon Tzur

c=0

n=1

if [ ! -e "hw4" ]; then
	echo "The binary (hw4) is not present."
	
	if [ ! -e "hw4.c" ]; then
		echo "Your code (hw4.c) is not present!"
		exit 1
	fi
	
	echo "Compiling hw4 out of hw4.c"
	
	gcc -O3 -std=c99 -Wall -pthread -o hw4{,.c}
	if [ "$?" -ne "0" ]; then
		echo "Compilation error! debug and run again!"
		exit 1
	fi
	
	if [ ! -e "hw4" ]; then
		echo "Unexpected error!"
		exit 1
	fi
	
	echo ""
	echo ""
fi

###Test 1
ls tzur_tester_hw4_test${n}_recovery > /dev/null 2>&1 && rm tzur_tester_hw4_test${n}_recovery
EXPECTED_FILE_DIFF="costa_rica.jpg"

for i in {1..300}; do
	./hw4 tzur_tester_hw4_test${n}_recovery costa_rica.jpg | diff - tzur_tester_hw4_test${n}_expected
	if [ "$?" -ne "0" ]; then
		echo "Diff is not as expected! (program status output)"
		exit 1
	fi
	
	cmp -n $(stat ${EXPECTED_FILE_DIFF} | grep Size | cut -d: -f2 | awk '{print $1}') ${EXPECTED_FILE_DIFF} tzur_tester_hw4_test${n}_recovery
	if [ "$?" -ne "0" ]; then
		echo "Diff is not as expected! (program binary output)"
		exit 1
	fi
	
	if [ "$i" -eq "50" ]; then
		echo -e "\tPutting garbage on test file"
		echo "a" > tzur_tester_hw4_test${n}_recovery
	fi
	
	((c++))
done

echo "Passed test $n"
echo ""
((n++))

###Test 2
EXPECTED_FILE_DIFF="peter_pan.txt"

for i in {1..300}; do
	./hw4 tzur_tester_hw4_test${n}_recovery peter_pan.txt | diff - tzur_tester_hw4_test${n}_expected
	if [ "$?" -ne "0" ]; then
		echo "Diff is not as expected! (program status output)"
		exit 1
	fi
	
	cmp -n $(stat ${EXPECTED_FILE_DIFF} | grep Size | cut -d: -f2 | awk '{print $1}') ${EXPECTED_FILE_DIFF} tzur_tester_hw4_test${n}_recovery
	if [ "$?" -ne "0" ]; then
		echo "Diff is not as expected! (program binary output)"
		exit 1
	fi
	
	if [ "$i" -eq "50" ]; then
		echo -e "\tPutting garbage on test file"
		echo "a" > tzur_tester_hw4_test${n}_recovery
	fi
	
	((c++))
done

echo "Passed test $n"
echo ""
((n++))

###Test 3
EXPECTED_FILE_DIFF="peter_pan.txt"

echo -e "\tTesting on peter pan"
for i in {1..100}; do
	./hw4 tzur_tester_hw4_test${n}_recovery alice.txt xor_peter_pan_alice | diff - tzur_tester_hw4_test${n}_expected
	if [ "$?" -ne "0" ]; then
		echo "Diff is not as expected! (program status output)"
		exit 1
	fi
	
	cmp -n $(stat ${EXPECTED_FILE_DIFF} | grep Size | cut -d: -f2 | awk '{print $1}') ${EXPECTED_FILE_DIFF} tzur_tester_hw4_test${n}_recovery
	if [ "$?" -ne "0" ]; then
		echo "Diff is not as expected! (program binary output)"
		exit 1
	fi
	
	if [ "$i" -eq "50" ]; then
		echo -e "\tPutting garbage on test file"
		echo "a" > tzur_tester_hw4_test${n}_recovery
	fi
	
	((c++))
done

echo -e "\tTesting on alice"
for i in {1..100}; do
EXPECTED_FILE_DIFF="alice.txt"

	./hw4 tzur_tester_hw4_test${n}_recovery peter_pan.txt xor_peter_pan_alice | diff - tzur_tester_hw4_test${n}_expected
	if [ "$?" -ne "0" ]; then
		echo "Diff is not as expected! (program status output)"
		exit 1
	fi
	
	cmp -n $(stat ${EXPECTED_FILE_DIFF} | grep Size | cut -d: -f2 | awk '{print $1}') ${EXPECTED_FILE_DIFF} tzur_tester_hw4_test${n}_recovery
	if [ "$?" -ne "0" ]; then
		echo "Diff is not as expected! (program binary output)"
		exit 1
	fi
	
	if [ "$i" -eq "50" ]; then
		echo -e "\tPutting garbage on test file"
		echo "a" > tzur_tester_hw4_test${n}_recovery
	fi
	
	((c++))
done

echo "Passed test $n"
echo ""
((n++))

###Test 4
#Bad filename test
./hw4 tzur_tester_hw4_test${n}_recovery peter_pan.txt non_existant_file alice.txt xor_peter_pan_alice > /dev/null 2>&1
if [ "$?" -eq "0" ]; then
	echo "No non-zero return code, when supplying bad filename!"
	exit 1
fi

echo "Passed test $n"
echo ""
((n++))

###Test 5

echo -e "\tTesting on alice"
for i in {1..100}; do
EXPECTED_FILE_DIFF="alice.txt"

	./hw4 tzur_tester_hw4_test${n}_recovery peter_pan.txt harry_potter.txt xor_peter_pan_alice_harry_potter | diff - tzur_tester_hw4_test${n}_expected
	if [ "$?" -ne "0" ]; then
		echo "Diff is not as expected! (program status output)"
		exit 1
	fi
	
	cmp -n $(stat ${EXPECTED_FILE_DIFF} | grep Size | cut -d: -f2 | awk '{print $1}') ${EXPECTED_FILE_DIFF} tzur_tester_hw4_test${n}_recovery
	if [ "$?" -ne "0" ]; then
		echo "Diff is not as expected! (program binary output)"
		exit 1
	fi
	
	if [ "$i" -eq "50" ]; then
		echo -e "\tPutting garbage on test file"
		echo "a" > tzur_tester_hw4_test${n}_recovery
	fi
	
	((c++))
done

echo -e "\tTesting on peter_pan"
for i in {1..100}; do
EXPECTED_FILE_DIFF="peter_pan.txt"

	./hw4 tzur_tester_hw4_test${n}_recovery alice.txt harry_potter.txt xor_peter_pan_alice_harry_potter | diff - tzur_tester_hw4_test${n}_expected
	if [ "$?" -ne "0" ]; then
		echo "Diff is not as expected! (program status output)"
		exit 1
	fi
	
	cmp -n $(stat ${EXPECTED_FILE_DIFF} | grep Size | cut -d: -f2 | awk '{print $1}') ${EXPECTED_FILE_DIFF} tzur_tester_hw4_test${n}_recovery
	if [ "$?" -ne "0" ]; then
		echo "Diff is not as expected! (program binary output)"
		exit 1
	fi
	
	if [ "$i" -eq "50" ]; then
		echo -e "\tPutting garbage on test file"
		echo "a" > tzur_tester_hw4_test${n}_recovery
	fi
	
	((c++))
done

echo -e "\tTesting on harry potter"
for i in {1..100}; do
EXPECTED_FILE_DIFF="harry_potter.txt"

	./hw4 tzur_tester_hw4_test${n}_recovery alice.txt peter_pan.txt xor_peter_pan_alice_harry_potter | diff - tzur_tester_hw4_test${n}_expected
	if [ "$?" -ne "0" ]; then
		echo "Diff is not as expected! (program status output)"
		exit 1
	fi
	
	cmp -n $(stat ${EXPECTED_FILE_DIFF} | grep Size | cut -d: -f2 | awk '{print $1}') ${EXPECTED_FILE_DIFF} tzur_tester_hw4_test${n}_recovery
	if [ "$?" -ne "0" ]; then
		echo "Diff is not as expected! (program binary output)"
		exit 1
	fi
	
	if [ "$i" -eq "50" ]; then
		echo -e "\tPutting garbage on test file"
		echo "a" > tzur_tester_hw4_test${n}_recovery
	fi
	
	((c++))
done

echo "Passed test $n"
echo ""
((n++))

###Test 6

echo -e "\tTesting on alice"
for i in {1..100}; do
EXPECTED_FILE_DIFF="alice.txt"

	./hw4 tzur_tester_hw4_test${n}_recovery peter_pan.txt harry_potter.txt xor_all costa_rica.jpg | diff - tzur_tester_hw4_test${n}_expected
	if [ "$?" -ne "0" ]; then
		echo "Diff is not as expected! (program status output)"
		exit 1
	fi
	
	cmp -n $(stat ${EXPECTED_FILE_DIFF} | grep Size | cut -d: -f2 | awk '{print $1}') ${EXPECTED_FILE_DIFF} tzur_tester_hw4_test${n}_recovery
	if [ "$?" -ne "0" ]; then
		echo "Diff is not as expected! (program binary output)"
		exit 1
	fi
	
	if [ "$i" -eq "50" ]; then
		echo -e "\tPutting garbage on test file"
		echo "a" > tzur_tester_hw4_test${n}_recovery
	fi
	
	((c++))
done

echo -e "\tTesting on peter pan"
for i in {1..100}; do
EXPECTED_FILE_DIFF="peter_pan.txt"

	./hw4 tzur_tester_hw4_test${n}_recovery alice.txt harry_potter.txt xor_all costa_rica.jpg | diff - tzur_tester_hw4_test${n}_expected
	if [ "$?" -ne "0" ]; then
		echo "Diff is not as expected! (program status output)"
		exit 1
	fi
	
	cmp -n $(stat ${EXPECTED_FILE_DIFF} | grep Size | cut -d: -f2 | awk '{print $1}') ${EXPECTED_FILE_DIFF} tzur_tester_hw4_test${n}_recovery
	if [ "$?" -ne "0" ]; then
		echo "Diff is not as expected! (program binary output)"
		exit 1
	fi
	
	if [ "$i" -eq "50" ]; then
		echo -e "\tPutting garbage on test file"
		echo "a" > tzur_tester_hw4_test${n}_recovery
	fi
	
	((c++))
done

echo -e "\tTesting on harry potter"
for i in {1..100}; do
EXPECTED_FILE_DIFF="harry_potter.txt"

	./hw4 tzur_tester_hw4_test${n}_recovery peter_pan.txt alice.txt xor_all costa_rica.jpg | diff - tzur_tester_hw4_test${n}_expected
	if [ "$?" -ne "0" ]; then
		echo "Diff is not as expected! (program status output)"
		exit 1
	fi
	
	cmp -n $(stat ${EXPECTED_FILE_DIFF} | grep Size | cut -d: -f2 | awk '{print $1}') ${EXPECTED_FILE_DIFF} tzur_tester_hw4_test${n}_recovery
	if [ "$?" -ne "0" ]; then
		echo "Diff is not as expected! (program binary output)"
		exit 1
	fi
	
	if [ "$i" -eq "50" ]; then
		echo -e "\tPutting garbage on test file"
		echo "a" > tzur_tester_hw4_test${n}_recovery
	fi
	
	((c++))
done

echo -e "\tTesting on costa rica"
for i in {1..100}; do
EXPECTED_FILE_DIFF="costa_rica.jpg"

	./hw4 tzur_tester_hw4_test${n}_recovery peter_pan.txt harry_potter.txt xor_all alice.txt | diff - tzur_tester_hw4_test${n}_expected
	if [ "$?" -ne "0" ]; then
		echo "Diff is not as expected! (program status output)"
		exit 1
	fi
	
	cmp -n $(stat ${EXPECTED_FILE_DIFF} | grep Size | cut -d: -f2 | awk '{print $1}') ${EXPECTED_FILE_DIFF} tzur_tester_hw4_test${n}_recovery
	if [ "$?" -ne "0" ]; then
		echo "Diff is not as expected! (program binary output)"
		exit 1
	fi
	
	if [ "$i" -eq "50" ]; then
		echo -e "\tPutting garbage on test file"
		echo "a" > tzur_tester_hw4_test${n}_recovery
	fi
	
	((c++))
done

echo "Passed test $n"
echo "~~~"
echo "Your program passed $c tests in total!"
echo "~~~" 
echo "Kol hakavod alufim!!!"
