#!/bin/bash

# gnuplot


# config
MESSAGES=1000
CLIENTS=1
IP=127.0.0.1
SIZES="40 64 128 256 512 1024 $((1024*2)) $((1024*4)) $((1024*8)) $((1024*16)) $((1024*32)) $((1024*64)) $((1024*128)) $((1024*256)) $((1024*512)) $((1024*1024)) $((1024*2048)) $((1024*4096))"

# run server in loop
function run_server()
{
	# reset
	echo > result.log
	# loop
	for size in ${SIZES}
	do
		hwloc-bind --cpubind core:${CLIENTS} -- ./poc -m ${MESSAGES} -S ${size} -C ${CLIENTS} -s ${IP} | egrep '^Time' | tee -a result.log
	done
	# gen plot cmd
	cat > 'result.gnuplot' <<-EOF
	set output 'result.pdf'
	set logscale x 2
	set grid
	plot 'result.log' u 11:5 w lp
	EOF
	# plot
	gnuplot 'result.gnuplot'
}

# run client in loop
function run_client()
{
	# loop
	for size in ${SIZES}
	do
		if [ ${CLIENTS} == 1 ]; then
			hwloc-bind --cpubind core:0-$((${CLIENTS}-1)) -- ./poc -m ${MESSAGES} -S ${size} -t ${CLIENTS} -c ${IP}
		else
			hwloc-bind --cpubind core:$((${CLIENTS}-1)) -- ./poc -m ${MESSAGES} -S ${size} -t ${CLIENTS} -c ${IP}
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
