CFLAGS="
    -I./deps/inc 
    -I./src 
    -I./src/config 
    -I./src/viewer 
    -I./src/server 
    -L./deps/lib 
    -lraylib 
    -lm 
    -lpthread
"

FILES="
    src/viewer/thbc-viewer.c
    src/config/thbc-config.c
    src/server/thbc-server.c
    deps/lib/qrcodegen.c
    src/main.c
"

# Compile and Run
gcc $FILES $CFLAGS -o export/thb-comm
cd export
./thb-comm
cd ..