# update submodule
git submodule update --init --recursive

# install aws-lambda-cpp
cd aws-lambda-cpp
mkdir build
cd build
cmake3 .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=../../build
make && make install
cd ../..


# install libsodium
cd libsodium
PWD=$(pwd)
autoreconf --install
./configure --prefix=$PWD/../build
make && make check
make install
cd ..

# install aws-sdk-cpp
cd aws-sdk-cpp
mkdir build
cd build
cmake3 .. -DCMAKE_BUILD_TYPE=Release -DBUILD_ONLY="lambda" -DCMAKE_INSTALL_PREFIX=../../build -DBUILD_SHARED_LIBS=OFF -DS2N_NO_PQ_ASM=ON -DENABLE_UNITY_BUILD=ON -DENABLE_TESTING=OFF
make
sudo make install
cd ../..



# build discord-lambda
cd hello-lambda
mkdir build
cd build
cmake3 .. -DCMAKE_BUILD_TYPE=Release
mkdir tmp
cp -r ../config/ tmp/
make aws-lambda-package-discord-bot