# Z Portable Database

TODO

## Building

```shell
source env.sh

mkdir build
cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../cmake/cosmo.cmake
ninja

./src/z_pdb
```
