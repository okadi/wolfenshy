# wolfenshy
Small raycaster written in C with SDL3. This name sucks.

DDA algorithm from Lode's tutorial: https://lodev.org/cgtutor/raycasting.html

## Building
You need CMake and a C compiler.

Clone with submodules:

```
git clone --recursive https://github.com/okadi/wolfenshy.git
```

Run `git submodule update --init` if you didn't use `--recursive`.

```
cmake -B build
cmake --build build
```
