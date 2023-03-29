export PATH=/home/liushangyang/cmake-3.26.1-linux-x86_64/bin:$PATH
export PATH=/home/liushangyang/grpc/build/out/bin:$PATH
rm -rf out
mkdir -p out/build
cd out/build
cmake ../.. 
make -j 4
