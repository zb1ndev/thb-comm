CFLAGS="
    -I./dependencies/include 
    -I./source 
    -I./source/config 
    -I./source/viewer 
    -I./source/server 
    -L./dependencies/lib 
    -lraylib 
    -lm 
    -lpthread
"

FILES="
    source/viewer/thbc-viewer.c
    source/config/thbc-config.c
    source/server/thbc-server.c
    dependencies/lib/qrcodegen.c
    source/main.c
"

# Compile and Run
gcc $FILES $CFLAGS -o export/thb-comm
cd export
./thb-comm
cd ..