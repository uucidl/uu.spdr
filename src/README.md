source files

the sources are split into separate .c files, however the ones you
should compile are those ending with the `_unit` suffix.

Suffixes:
* `_unit` denotes compilation units, which may link together multiple `.c` files
* `_osx`, `_win32` denote OSX/WIN32/... specific files
