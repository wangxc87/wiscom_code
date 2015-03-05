#!/bin/sh
#$1 disk dev_name 
#$2 disk fdisk config file
#$3 disk part mount path

DISK_DEV=$1

echo ""
if [ -z $1 ] || [ -z $2 ]
then
	echo "***Invalid input***"
	exit -1
fi

if [ -f $2 ];then
	echo "***Format disk $1 mount on $3**"

	fdisk ${DISK_DEV} <$2  1>/dev/NULL
	if [ ! $? -eq 0 ];then
		echo "***fdisk Error***"
		exit -1
	fi

	mkfs.ext3 ${DISK_DEV}1
	if [ ! $? -eq 0 ];then
		echo "***mkfs.ext3 Error***"
		exit -1
	fi

	if [ ! -z $3 ];then
		mkdir -p $3		
		mount ${DISK_DEV}1 $3 1>/dev/NULL
		if [ $? = 0 ];then
			echo "****${DISK_DEV}1 mount on $3 ****"
		else
			echo "****${DISK_DEV}1 mount on $3 Fail***"
		fi		
	fi
	exit 0;
else
	echo "****There is no fdisk configure File****"
	echo ""
	exit -1;
fi

