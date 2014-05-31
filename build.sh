
cd `dirname $0`

rm -rf cmake_tmp
mkdir -p cmake_tmp
cd cmake_tmp &&

cmake .. -DCMAKE_INSTALL_PREFIX=/usr &&
make &&
echo "Done."
