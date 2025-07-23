# FAT32 Emulator

Small project to learn how FAT32 works internally. Main purpose of this project is to access(get list of files, read files...) FAT32 filesystem from a file, and be able to modify it(create a folder for example).

# How to run
```
mkdir build
cmake -S . -B build
cmake --build build
./build/fat32_emulator fat_filesystem.bin
```

# Commands

cd <path> - change directory, path can be relative or absolute 
ls - show files in current directory
