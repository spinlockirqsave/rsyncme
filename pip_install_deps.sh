#!/bin/bash

# @file     install_cmocka.sh
# @author   Piotr Gregor <piotrek.gregor@gmail.com>
# @date     7 July 2016 11:59 PM
# @brief    cmocka (and cmake) installation for Bitbucket's Pipelines automated build


pkg_check_install ()
{
	PKG_OK=$(dpkg-query -W --showformat='${Status}\n' "$1" | grep "install")
	echo "Checking for [$1] : [$PKG_OK]"
	if [ "$PKG_OK" != "install ok installed" ]; then
		echo "Installing [$1]..."
		apt-get install --force-yes --yes $1
	fi
}

install_cmocka ()
{
    echo "--> checking for cmocka sources..."
    if [ -f "cmocka-1.0.1.tar.xz" ]; then
      # tar exists
      echo "--> using existing tar file cmocka-1.0.1.tar.xz pack" 
    else
      echo "--> downloading source files..."
      wget https://cmocka.org/files/1.0/cmocka-1.0.1.tar.xz				# wget
    fi
    echo "--> unpacking..."
    tar xf cmocka-1.0.1.tar.xz
    cd cmocka-1.0.1
    echo "--> creating build directory..."
    mkdir build
    cd build
    echo "--> building..."
    cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Debug ..
    make
    echo "--> installing..."
    make install
}

if [ $# != 1 ]; then
    echo "--> please specify the directory for cmocka"
    exit 1
else
	dir=$1						# dir is the name of the folder when to download cmocka
	if [ -d "$dir" ]; then
		echo "--> directory ["$1"] exists, do you want to reuse it? [y/Y/yes/Yes for YES]"
		read res
		if [ $res != "Y" ] && [ $res != "y" ] && [ $res != "yes" ] && [ $res != "Yes" &&  $res != "YES" ]; then
			exit 2
		fi
	else
		mkdir $dir
		if [ $? -ne 0 ]; then
			echo " --> can't create a directory [$dir]"
			exit 3
		fi
		chown -R $USER $dir
		if [ $? -ne 0 ]; then
			echo " --> can't chown the directory [$dir]"
			exit 4
		fi
	fi
fi

cd $dir											# change directory
pkg_check_install cmake
pkg_check_install uuid
pkg_check_install uuid-dev
install_cmocka                                  # install cmocka
if [ ! -d "/usr/local/rsyncme/log" ]; then
	mkdir -p /usr/local/rsyncme/log
fi
chown -R $USER /usr/local/rsyncme
chmod u+rw /usr/local/rsyncme

