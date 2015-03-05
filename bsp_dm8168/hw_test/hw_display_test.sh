#/bin/
#$1: run mode 1:ivs 2:dvr

usage()
{
	echo ""
	echo "***Invalid arg***"
	echo "example: $0  [ ivs/dvr/dec ] [ check sii9022 1/0] ]"
	echo ""
}

dir_suffix=ti816x_orig
dvr_rdk_path=/opt/dvr_rdk/${dir_suffix}
ini_file=/opt/dvr_rdk/common_ini/1920x1080_test_6CH.ini

if [ $1 = 'h' ]
then
	usage
	exit 0
fi

#arg is Null
if [ -z $1 ]  
then
	cd ${dvr_rdk_path}
	./init.sh ;./load.sh
	./bin/dvr_rdk_demo_mcfw_api.out
	./unload.sh
	exit 0;
fi

if [ ! -z $2 ]
then
	if [ ! $2 = '1' ] && [  ! $2 = '0' ]
	then
		usage
		exit 0;
	fi
fi

if [ $1 = "dec" ]
then
	dir_suffix=ti816x_dec
	dvr_rdk_path=/opt/dvr_rdk/${dir_suffix}

	cd ${dvr_rdk_path}
	./init.sh ;./load.sh
	./bin/dvr_rdk_demo_mcfw_api.out -m 1 -f ${ini_file}
	./unload.sh
	exit 0;
fi


if [ $1 = "ivs" ]
then
	cd ${dvr_rdk_path}
	./init.sh ;./load.sh
	if [ ! -z $2 ]
	then
		./bin/dvr_rdk_demo_mcfw_api.out -m 1 -f ${ini_file} -s $2
	else
		./bin/dvr_rdk_demo_mcfw_api.out -m 1 -f ${ini_file}
	fi
	./unload.sh
	exit 0;
else if [ $1 = "dvr" ]
	then
		cd ${dvr_rdk_path}
		./init.sh ;./load.sh
		if [ ! -z $2 ]
		then
			./bin/dvr_rdk_demo_mcfw_api.out -m 2 -s $2
		else
			./bin/dvr_rdk_demo_mcfw_api.out -m 2
		fi
		./unload.sh
		exit 0;
else
	usage
	exit -1
fi
fi
