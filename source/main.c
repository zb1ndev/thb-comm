#define CFRAME_IMPLEMENTATION
#include <cframe.h>

#include "thbc-base.h"
#include "thbc-config.h"
#include "thbc-viewer.h"
#include "thbc-server.h"

int main(void) {

    thbc_server_t server = empty_thbc_server_t;
    if (thbc_start_server(&server) < 0) {
        perror("main: thbc_start_server");
        return EXIT_FAILURE;
    }

    thbc_viewer_state_t viewer_state = empty_thbc_viewer_state_t;
    if (thbc_init_viewer(&viewer_state, &server) < 0) {
        perror("main: thbc_init_viewer");
        return EXIT_FAILURE;
    }

    while (viewer_state.running && server.http_server.running) 
        thbc_update_viewer(&viewer_state, &server);
    thbc_close_viewer(&viewer_state, &server);

    thbc_stop_server(&server);
    return EXIT_SUCCESS;

}