README

Building ALSA User Space LIBs
-----------------------------
1. Download ALSA User space libraries from http://www.alsa-project.org/main/index.php/Download
2. Compile and generate the ALSA lib from the above package.
    2.1 follow the steps to build the alsa lib for ARM arch
	    # CC=arm-arago-linux-gnueabi-gcc ./configure --target=arm-linux --host=i386 --prefix=<path to save alsa lib>
	2.2 # make
	2.3 # make install    <this will creart the ALSA libs in the path specified in --prefix>


Build Sample application
------------------------
3. Ensure that the "Rules.make" file in the root folder is updated with

    3.1  KERNEL_DIR = <your kernel dir>
    3.2  CROSS_COMPILE = <your cross compiler >

4. Ensure that the Makefile for audio is updated with
    4.1  ALSA_LIB = <path to alsa lib generated above, usually prefix/lib>
    4.2  ALSA_INC = <path to alsa user lib include directory, usually prefix/include>
