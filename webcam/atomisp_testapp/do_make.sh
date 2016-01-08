#!/bin/bash

OTHER_CFLAGS=
TARGET_ARCH=32

function print_usage() {
	echo "Usage: $0 [mipi-way|mipi-x] {CFLAGS...}"
	echo 
	echo "Supported CFLAGS:"
	echo "-DCOLOR_CONVERSION	Allow the app to accept different input color format."
	echo
}

function print_pause() {
	echo
	echo "If you see errors, press CTRL + C to stop now..."
	echo
	echo -n "Packing files in T-" 
	for I in {0..4}
	do
		echo -n `expr 5 - $I`" "
		sleep 1
	done
	echo
}

function make_mipi_way() {
	make EXECUTABLE=isp-mipi-test clean
	make CC_ARCH="-m$TARGET_ARCH" CFLAGS+="-DI$TARGET_ARCH -DWAYLAND $OTHER_CFLAGS" LIBS+='-lwayland-client -lwayland-egl' EXECUTABLE=isp-mipi-test SOURCES+=src/isp-mipi-test.c all
}

function make_mipi_x() {
	make EXECUTABLE=isp-mipi-test clean
    make CC_ARCH="-m$TARGET_ARCH" CFLAGS+="-DI$TARGET_ARCH -DX11 $OTHER_CFLAGS" LIBS+='-lX11' EXECUTABLE=isp-mipi-test SOURCES+=src/isp-mipi-test.c all
}

#function make_fifo_way() {
#	make EXECUTABLE=isp-fifo-way clean
#	make CFLAGS+="-DWAYLAND $OTHER_CFLAGS" LIBS+='-lwayland-client -lwayland-egl' EXECUTABLE=isp-fifo-way SOURCES+=src/isp-fifo-way.c all
#}

if [ $# -le 0 ]; then
	print_usage
	exit 0
else
	if [ $# -gt 1 ]; then
		COUNT=0
		for ARG in $@
		do
			if [ $COUNT -ge 1 ]; then
				OTHER_CFLAGS="$OTHER_CFLAGS $ARG"
			fi
			COUNT=`expr $COUNT + 1`
		done
	fi
	
	if [ ! -z "${ARCH+1}" ]; then
		if [ $ARCH = "x86_64" ]; then
			TARGET_ARCH=64
		fi
	fi
	
	if [ $1 = "mipi-way" ]; then
		make_mipi_way
	elif [ $1 = "mipi-x" ]; then
		make_mipi_x
	elif [ $1 = "clean" ]; then
		make clean
	else
		echo -e "ERROR: Do not know what is \"$1\"."
		print_usage
		exit 0
	fi 
fi

#print_pause
#
#tar cjvf ~/overhaul_`date +"%Y%m%d"`.tar.bz2 ../overhaul \
#	--exclude=*~ --exclude=*.log \
#	--exclude=.settings --exclude=.cproject --exclude=.project

exit 0
