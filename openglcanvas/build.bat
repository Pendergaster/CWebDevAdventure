emcc canvas.c -o index.html -s USE_WEBGL2=1 -s USE_GLFW=3 -s WASM=0 -s EXPORTED_FUNCTIONS="['_window_on_resize', '_main']"  -s EXTRA_EXPORTED_RUNTIME_METHODS="['ccall', 'cwrap']" 
