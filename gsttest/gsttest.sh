#!/bin/bash

# Copyright (c) 2016 Nexell Co., Ltd.
# Author: Hyejung, Kwon <cjscld15@nexell.co.kr>
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
# the Software, and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


set -e

TEST_ALL=true
TEST_CAMERA=false
TEST_SCALER=false

function usage()
{
	echo "usage: $0 [ -t camera -t scaler ]"
        echo " -t camera : if you want to test camerasrc, specify this, default no"
        echo " -t scaler : if you want to test nxscaler, specify this, default no"
	exit 0
}

function vmsg()
{
        local verbose=${VERBOSE:-"false"}
        if [ ${verbose} == "true" ]; then
                echo "$@"
        fi
}

function parse_args()
{
        TEMP=`getopt -o "ht:" -- "$@"`
        eval set -- "$TEMP"

        while true; do
                case "$1" in
                        -t ) case "$2" in
                                camera    ) TEST_ALL=false; TEST_CAMERA=true ;;
                                scaler    ) TEST_ALL=false; TEST_SCALER=true ;;
                                none   ) TEST_ALL=false ;;
                             esac
                             shift 2 ;;
                        -h ) usage; exit 1 ;;
                        -- ) break ;;
                esac
        done
}

function print_args()
{

        vmsg "================================================="
        vmsg " print args"
        vmsg "================================================="
        if [ ${TEST_ALL} == "true" ]; then
                vmsg -e "TEST:ALL"
        else
                if [ ${TEST_CAMERA} == "true" ]; then
                        vmsg -e "TEST:CAMERASRC"
                fi
                if [ ${TEST_SCALER} == "true" ]; then
                        vmsg -e "TEST:SCALER"
                fi
        fi
        vmsg
}

function test_camerasrc()
{
        if [ ${TEST_ALL} == "true" ] || [ ${TEST_CAMERA} == "true" ]; then
                echo ""
                echo "================================================="
                echo "CAMERASRC TEST"
                echo "================================================="
		
		cd camerasrc
		find ./ -type f -print
		i="1"
		FILE='test_'$i'.sh'
		while [ -f $FILE ]
		do
			FILE='test_'$i'.sh'
			echo "Test Start[$FILE]"
			./$FILE &
			sleep 20
			echo "Test End[$FILE]"
			i=$[$i+1]
		done
        fi
}

function test_scaler()
{
        if [ ${TEST_ALL} == "true" ] || [ ${TEST_SCALER} == "true" ]; then
                echo ""
                echo "================================================="
                echo "SCALER TEST"
                echo "================================================="
		
		cd nxscaler
		find ./ -type f -print
		i="1"
		FILE='test_'$i'.sh'
		while [ -f $FILE ]
		do
			FILE='test_'$i'.sh'
			echo "Test Start[$FILE]"
			./$FILE &
			sleep 20
			echo "Test End[$FILE]"
			i=$[$i+1]
		done
        fi
}


parse_args $@
print_args
test_camerasrc
test_scaler
