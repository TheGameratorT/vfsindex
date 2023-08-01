# vfsindex
A tool to generate a VFS (Virtual FileSystem) "~INDEX" file for the game Sakura Dungeon.

## Building

### Windows
Make sure that CMake, gengetopt and a compatible compiler (MSVC, GCC or Clang) is installed. MSVC is recommended. \
Then run the following commands:

Example for MSVC (2022):
```bat
git clone https://github.com/TheGameratorT/vfsindex.git
mkdir build && cd build
cmake ../ -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```
Note: You might need to change the version or the architecture used in the example. \
The output files can be found in the `build` directory.

### Linux and MacOS
Make sure that the following packages `cmake`, `gengetopt` and `build-essential` are installed on the system and run these commands:
```sh
git clone https://github.com/TheGameratorT/vfsindex.git
mkdir build && cd build
cmake ../ -DCMAKE_BUILD_TYPE=Release
make
```
The output files can be found in the `build` directory.

## Running

If you want to dump the contents of an existing `~INDEX` file, run the command:
```bat
vfsindex -i PATH_TO_INDEX_FILE > PATH_TO_OUTPUT_FILE
```

If you want to create your own `~INDEX` file, run the command:
```bat
vfsindex -i PATH_TO_INPUT_DIRECTORY -o PATH_TO_OUTPUT_FILE
```
Optionally --override (-y) can be used to overwrite without confirmation.
