# webnote

simple c++ web app

## how to use this project?
make sure you have installed conan and cmake.

install conan package:
```
conan install . --build=missing --output-folder=build
```
prepare build:
```
cd build && cmake .. -DCMAKE_TOOLCHAIN_FILE=build/Release/generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
```
build:
```
cmake --build .
```
run this project:
`./webnote`
