#!/bin/sh
boxname=prefixbox

mkdir src
cd src
for i in $(cat ../downloads.txt); do
    wget -Nc $i
done
rm -rf gcc-4.9.1
tar xfj gcc-4.9.1.tar.bz2
cp ../bpt-rules/gcc-4.9.1 gcc-4.9.1/bpt-rules
rm -rf boost_1_56_0
tar xfj boost_1_56_0.tar.bz2
cp ../bpt-rules/boost-1.56.0 boost_1_56_0/bpt-rules
rm -rf cmake-3.0.2
tar xfz cmake-3.0.2.tar.gz
cp ../bpt-rules/cmake-3.0.2 cmake-3.0.2/bpt-rules
tar xfz openssl-1.0.1i.tar.gz
cp ../bpt-rules/openssl-1.0.1i openssl-1.0.1i/bpt-rules
rm -rf Python-2.7.8
tar xfz Python-2.7.8.tgz
cp ../bpt-rules/python-2.7.8 Python-2.7.8/bpt-rules
rm -rf pigz-2.3.1
tar xvfz pigz-2.3.1.tar.gz
cp ../bpt-rules/pigz-2.3.1 pigz-2.3.1/bpt-rules
cd ..
~/bpt/box create prefixbox
~/bpt/box -b prefixbox autobuild src/gmp-6.0.0a.tar.bz2 || exit 1
~/bpt/box -b prefixbox autobuild src/mpfr-3.1.2.tar.bz2 || exit 1
~/bpt/box -b prefixbox autobuild src/mpc-1.0.2.tar.gz || exit 1
~/bpt/box -b prefixbox autobuild src/isl-0.13.tar.bz2 || exit 1
~/bpt/box -b prefixbox autobuild src/cloog-0.18.1.tar.gz || exit 1
~/bpt/box -b prefixbox build src/gcc-4.9.1 || exit 1
~/bpt/box -b prefixbox autobuild src/ncurses-5.9.tar.gz || exit 1
~/bpt/box -b prefixbox autobuild src/readline-6.3.tar.gz || exit 1
~/bpt/box -b prefixbox build src/openssl-1.0.1i || exit 1
MYINCLUDEDIR="$(pwd)/prefixbox/include/" MYLIBDIR="$(pwd)/prefixbox/lib" ~/bpt/box -b prefixbox build src/Python-2.7.8 || exit 1
prefixbox/env ~/bpt/box -b prefixbox build ~/bpt || exit 1
prefixbox/env box autobuild src/setuptools-6.1.tar.gz || exit 1
~/bpt/box -b prefixbox build src/boost_1_56_0 || exit 1
~/bpt/box -b prefixbox build src/cmake-3.0.2 || exit 1
~/bpt/box -b prefixbox autobuild src/libevent-2.0.21-stable.tar.gz || exit 1
CFLAGS="-I$(pwd)/prefixbox/include/ncurses" ~/bpt/box -b prefixbox autobuild src/tmux-1.9a.tar.gz || exit 1
~/bpt/box -b prefixbox autobuild --configure_options="--disable-unicode" src/htop-1.0.3.tar.gz || exit 1
~/bpt/box -b prefixbox autobuild src/tig-2.0.3.tar.gz || exit 1
~/bpt/box -b prefixbox autobuild src/pv-1.5.7.tar.gz || exit 1
~/bpt/box -b prefixbox build src/pigz-2.3.1 || exit 1
~/bpt/box -b prefixbox autobuild src/coreutils-8.9.tar.gz || exit 1
