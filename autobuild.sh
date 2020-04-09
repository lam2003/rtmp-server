mkdir ./build -p && cd ./build
cmake -DCMAKE_CXX_COMPILER=clang++-9 -DCMAKE_C_COMPILER=clang-9 ../
make -j8 && make rtmp_server -j8
cd ../ && cp ./build/app/rtmp_server ./