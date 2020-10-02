#!/bin/bash

# gnuplot


# config
MESSAGES=500000
CLIENTS=2
IP=10.1.3.26
SIZES="40 64 128 256 512 1024 $((1024*2)) $((1024*4)) $((1024*8)) $((1024*16)) $((1024*32)) $((1024*64)) $((1024*128)) $((1024*256)) $((1024*512)) $((1024*1024)) $((1024*2048)) $((1024*4096))"
DIVIDE_AT=$((64*1024))
DIVIDE_AT2=$((1024*1024))

set -e

function gen_chart()
{
	# grep driver mode
	mode="$(cat client.log | egrep '^NET:' | head -n 1 | cut -d ' ' -f 2 | sed -e 's/_/-/g')"
	# gen plot cmd
	cat > 'result.gnuplot' <<-EOF
	set term pdf
	set output 'result.pdf'
	set logscale x 2
	set xlabel 'Message size (kBytes)'
	set ylabel 'Rate (kMsg/s)'
	set ytics tc lt 1
	set y2tics tc lt 2
	set y2label 'Bandwidth (Gbits/s)' tc lt 2
	set title "Libfaric ping pong (${mode})"
	set grid
	plot 'result.log' u (\$11/1024):5 w lp title "Message rate", '' u (\$11/1024):8 axes x1y2 w lp title "Bandwidth"
	EOF
	# plot
	gnuplot 'result.gnuplot'
}

# run server in loop
function run_server()
{
	# reset
	echo > result.log
	# loop
	for size in ${SIZES}
	do
		if [ ${size} == $DIVIDE_AT ]; then MESSAGES=$(($MESSAGES/10)); fi
		if [ ${size} == $DIVIDE_AT2 ]; then MESSAGES=$(($MESSAGES/10)); fi
		hwloc-bind --cpubind core:${CLIENTS} -- ./poc -m ${MESSAGES} -S ${size} -C ${CLIENTS} -s ${IP} | egrep '^Time' | tee -a result.log
	done
	gen_chart
}

# run client in loop
function run_client()
{
	#reset
	echo > client.log

	# loop
	for size in ${SIZES}
	do
		if [ ${size} == $DIVIDE_AT ]; then MESSAGES=$(($MESSAGES/10)); fi
		if [ ${size} == $DIVIDE_AT2 ]; then MESSAGES=$(($MESSAGES/10)); fi
		if [ ${CLIENTS} == 1 ]; then
			hwloc-bind --cpubind core:0-$((${CLIENTS}-1)) -- ./poc -m ${MESSAGES} -S ${size} -t ${CLIENTS} -C ${CLIENTS} -c ${IP} | tee -a client.log
		else
			hwloc-bind --cpubind core:$((${CLIENTS}-1)) -- ./poc -m ${MESSAGES} -S ${size} -t ${CLIENTS} -C ${CLIENTS} -c ${IP} | tee -a client.log
		fi
		sleep 2
	done
}

# select
case $1 in
	's')
		run_server
		;;
	'c')
		run_client
		;;
	*)
		echo "Invalid mode, please give 's' or 'c' as parameter !" 1>&2
		exit 1
		;;
esac
