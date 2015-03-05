#/bin/sh
#$1: eth num 0/1
#$2: local ip
#$3: remote ip

echo ""
if [ -z $2 ] && [ -z $3 ]
	then
	echo "***Invaild Arg ***"
	echo "Example:"
	echo "     $0 eth0  [ local_ip ] remote_ip"
	echo ""
	exit 0
fi

if [ $2 ] && [ -z $3 ]
	then
	remote_ip=$2
else
	local_ip=$2
	remote_ip=$3
fi

if [ $1 = "eth0" ]
	then
	ifconfig eth1 down
	
	if [  ! -z  ${local_ip} ]
	then
	ifconfig eth0 ${local_ip}
	fi

	if [ ! $? = 0 ]  
		then
		#config Error
		echo "***ifconfig $1 IP Error***"
		echo ""
		exit -1
	else
		#ping OK
		ping ${remote_ip} -c 3 -W 5
		if [ $? = 0 ]  
		then
			echo "****netPort0 test Ok****"
		exit 0
		else
			echo "****netPort0 test Error****"
		exit -1
		fi
	fi
else
	ifconfig eth0 down

	if [ ! -z  ${local_ip} ]
	then
	ifconfig eth1 ${local_ip}
	fi
	
	if [ ! $? = 0 ]  
		then
		#config Error
		echo "***ifconfig $1 IP Error***"
		echo ""
		exit -1
	else
		#ping OK
		ping ${remote_ip} -c 3 -W 5
		if [ $? = 0 ]  
		then
			echo "****netPort1 test Ok****"
		exit 0
		else
			echo "****netPort1 test Error****"
		exit -1
		fi
		exit 0
	fi
fi
	
