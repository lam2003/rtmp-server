mkdir ./build -p && cd ./build
cmake -DCMAKE_CXX_COMPILER=clang++-9 -DCMAKE_C_COMPILER=clang-9 ../src
#cmake ../src
make -j8 && make rtmp_server -j8
cd ../ && cp ./build/app/rtmp_server ./
# env PPROF_PATH=/usr/local/bin/pprof HEAPCHECK=normal ./rtmp_server
