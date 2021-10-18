#!/bin/bash
out_file=data.csv

SESSIONS_MAX=10
SESSIONS_MIN=1
SESSIONS_STEP=3
BSIZE_MAX=160
BSIZE_MIN=32
BSIZE_STEP=32

for i in "$@"
do
case $i in
    --sessions-max=*)
    SESSIONS_MAX="${i#*=}"
    shift # past argument=value
    ;;
    --sessions-min=*)
    SESSIONS_MIN="${i#*=}"
    shift # past argument=value
    ;;
    --sessions-step=*)
    SESSIONS_STEP="${i#*=}"
    shift # past argument=value
    ;;
    --bsize-max=*)
    BSIZE_MAX="${i#*=}"
    shift # past argument=value
    ;;
    --bsize-min=*)
    BSIZE_MIN="${i#*=}"
    shift # past argument=value
    ;;
    --bsize-step=*)
    BSIZE_STEP="${i#*=}"
    shift # past argument=value
    ;;
	--help)
	echo "Usage: ./run_test.sh [OPTION]..."
	echo "Possible options:"
	echo "	--sessions-max=VALUE"
	echo ""
	echo "	--sessions-min=VALUE"
	echo ""
	echo "	--sessions-step=VALUE"
	echo ""
	echo "	--bsize-max=VALUE"
	echo ""
	echo "	--bsize-min=VALUE"
	echo ""
	echo "	--bsize-step=VALUE"
	echo ""
	exit
	;;
    *)
          # unknown option
    ;;
esac
done

max_progress=$(( ( ($SESSIONS_MAX - $SESSIONS_MIN) / $SESSIONS_STEP + 1) * ( ($BSIZE_MAX - $BSIZE_MIN) / $BSIZE_STEP + 1)  ))
progress=0

echo "Perfomance test started. Progress:"


for ses in $(seq $SESSIONS_MIN $SESSIONS_STEP $SESSIONS_MAX)
do
	for bs in $(seq $BSIZE_MIN $BSIZE_STEP $BSIZE_MAX)
	do
		./hse-asio-client -p 41483 -a '192.168.1.75' -s $ses -b $bs >> $out_file

		
		(( ++progress ))
		percent=$(( (100 * $progress) / $max_progress ))
		count_hash=$(($percent / 2))
		count_space=$((50 - $count_hash))
		count_dots=$((progress % 4))
		count_dot_space=$((3 - count_dots))
		hashes=`printf %${count_hash}s |tr ' ' '#'`
		space=`printf %${count_space}s`
		dots=`printf %${count_dots}s |tr ' ' '.'`
		dot_space=`printf %${count_space}s`
		
		echo -ne "[${hashes}${space}] (${percent}%) ${dots}${dot_space}\r"
		
	done
done

echo "[${hashes}${space}] (${percent}%) ${dots}${dot_space}"

echo "Test done. Aborting."
