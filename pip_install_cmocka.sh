#!/bin/bash

# @file     install_cmocka.sh
# @author   Piotr Gregor <piotrek.gregor@gmail.com>
# @date     7 July 2016 11:59 PM
# @brief    cmocka (and cmake) installation for Pipelines automated build


pkg_check_install ()
{
    PKG_OK=$(dpkg-query -W --showformat='${Status}\n' "$pack"|grep "install ok installed")
    echo "--> checking for "$pack": $PKG_OK"
    if [ "" == "$PKG_OK" ]; then
      echo "--> no "$pack". Setting up "$pack"."
      apt-get --force-yes --yes install "$pack"
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


dir=$1						# dir is the name of a new folder when to download cmocka
if [ $dir ]; then				# dir must be passed to this script
  echo "--> creating directory ["$dir"]"
else
    echo "--> please specify the directory to create"
    exit 1
fi

if [ -d "$dir" ]; then
  echo "--> directory ["$1"] exists, please specify new folder"
  exit 2
fi

mkdir "$dir"
if [ $? -ne 0 ]; then
    echo " --> can't create a directory ["$dir"]"
    exit 3
fi
cd $dir											# change directory

pack=cmake										# check/install cmake
pkg_check_install "$pack"
install_cmocka

