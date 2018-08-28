if [[ "$OSTYPE" == "linux-gnu" ]]; then
  # Linux
  nproc=`nproc`
elif [[ "$OSTYPE" == "darwin"* ]]; then
  # MacOS
  nproc=`sysctl -n hw.ncpu`
else
  echo "OS not supported"
  exit
fi

wget https://cmake.org/files/v3.12/cmake-3.12.1.tar.gz
tar xvzf cmake-3.12.1.tar.gz
cd cmake-3.12.1
./configure --system-curl --parallel=$nproc
make -j${nproc}
sudo make install
cd ..
rm -rf cmake-3.12.1*
