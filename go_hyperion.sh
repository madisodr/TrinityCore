mkdir -p build
pushd build

cmake ../ \
	-DCMAKE_INSTALL_PREFIX=/home/sada/hyperion \
    -DWITH_WARNINGS=1 \
	-DSCRIPTS="dynamic"

make -j $(nproc --all)
make install
popd
