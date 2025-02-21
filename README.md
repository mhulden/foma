# foma

Finite-state transducer technology.

## Compilation Instructions

1. Ensure GNU readline library is installed.
2. `cd foma`
3. `cmake CMakeLists.txt`
4. `make`
5. `make install` (if you want to install it)

## Compiling to Web Assembly (wasm) using Emscripten

The `readline` and `zlib` dependencies are not needed for the wasm build.

```
# Install and activate Emscripten SDK
# (You may want to choose a different place in your filesystem to put emsdk)
git clone https://github.com/emscripten-core/emsdk.git  # only needed the first time
cd emsdk
git pull
./emsdk install latest
./emsdk activate latest
# Follow printed instructions
cd ..

# Build foma to WebAssembly using Emscripten
cd foma
emcmake cmake
emmake make

# Start a local web server:
python3 -m http.server 8000
```

Open a web browser and navigate to http://localhost:8000/demo.html.
The demo page allows you to test Foma regular expressions directly in your browser.
