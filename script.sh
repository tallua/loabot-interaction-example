# update submodule
git submodule update

# install aws-lambda-cpp
cd aws-lambda-cpp
mkdir build
cd build
cmake3 .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF \
   -DCMAKE_INSTALL_PREFIX=../../build
cd ../..


# install libsodium
cd libsodium
PWD=$(pwd)
autoreconf --install
./configure --prefix=$PWD/../build
make && make check
make install


