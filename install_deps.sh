#!/bin/bash

# @file     install_cmocka.sh
# @author   Piotr Gregor <piotrek.gregor@gmail.com>
# @date     21 June 2016 08:48 PM
# @brief    cmocka (and cmake) installation


set -e

pkg_check_install ()
{
    PKG_OK=$(dpkg-query -W --showformat='${Status}\n' "$pack"|grep "install ok installed")
    echo "--> checking for "$pack": $PKG_OK"
    if [ "" == "$PKG_OK" ]; then
      echo "--> no "$pack". Setting up "$pack"."
      sudo apt-get --force-yes --yes install "$pack"
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
	if [ ! -d build ]; then
		mkdir build
	fi
    cd build
    echo "--> building..."
    cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Debug ..
    make
    echo "--> installing..."
    sudo make install
}


dir=$1						# dir is the name of a new folder when to download cmocka
if [ $dir ]; then				# dir must be passed to this script
  echo "--> creating directory ["$dir"]"
else
    echo "--> please specify the directory for cmocka"
    exit 1
fi

if [ -d "$dir" ]; then
  echo "--> directory ["$1"] exists, do you want to reuse it? [y/Y/yes/Yes for YES]"
  read res
  if [ $res != "Y" ] && [ $res != "y" ] && [ $res != "yes" ] && [ $res != "Yes" &&  $res != "YES" ]; then
    exit 2
  fi
else
  sudo mkdir $dir
  if [ $? -ne 0 ]; then
    echo " --> can't create a directory [$dir]"
    exit 3
  fi
  sudo chown -R $USER $dir
  if [ $? -ne 0 ]; then
    echo " --> can't chown the directory [$dir]"
    exit 4
  fi
fi

# change directory
cd $dir

pack=cmake										# check/install cmake
pkg_check_install "$pack"
pack=uuid										# check/install uuid
pkg_check_install "$pack"
pack=uuid-dev									# check/install uuid-dev
pkg_check_install "$pack"
install_cmocka                                  # install cmocka
if [ ! -d "/usr/local/rsyncme/log" ]; then		# daemon's environment
	sudo mkdir -p /usr/local/rsyncme/log
fi
sudo chown -R $USER /usr/local/rsyncme
sudo chmod u+rw /usr/local/rsyncme
