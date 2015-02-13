#!/bin/bash

arguments=$@
sr620path="/DATA/LSU-TIC/"
defaultDev=/dev/ttyUSB0

PID=$(pgrep -x sr620)

for process in $PID; do
#	echo "$process"
	procargs=$(ps -o args -p $process)
	argarray=( $procargs )
	n=$((${#argarray[@]}-1))
	for (( i=0; i<=n; i++ ))
	do
#		echo "${argarray[i]}"
		if [ ${argarray[i]} == "-d" ] || [ ${argarray[i]} == "--device" ] && [ ${argarray[i+1]} == "${defaultDev}" ]
		then
                    isRunning="true"
		fi
	done
done

if [ $1 == "bootmode" ]
then
    if [ "${isRunning}" == "true" ]
    then
	echo "sr620 is already running with the device ${defaultDev}"
	exit 1
    else
	cd $sr620path
	screen -d -m -S sr620screen bash -c "./sr620 -d ${defaultDev} -l NU1 -a OT -b PT_Sept -t LSUTIC --calibrate --rotate 2>&1 | tee -a sr620.log"
	exit $?
    fi
fi

if [ $1 == "checkmode" ]
then
    if [ "${isRunning}" == "true" ]
    then
	echo "sr620 is running with the device ${defaultDev}"
	exit 0
    else
	exit 1
   fi
fi

if [ "${isRunning}" == "true" ]
then
    userargs=( $arguments )
    n=$((${#userargs[@]}-1))
    for (( i=0; i<=n; i++ ))
    do
#		echo "${userargs[i]}"
	if [ ${userargs[i]} == "-d" ] || [ ${userargs[i]} == "--device" ] && [ ${userargs[i+1]} == "${defaultDev}" ]
	then
	    echo "sr620 is already running with device ${defaultDev}"
	    exit 1
	fi
    done
fi
./sr620 $arguments | tee -a "sr620.log"
exit $?
