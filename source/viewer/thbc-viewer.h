#if !defined(THBC_VIEWER_H)
#define THBC_VIEWER_H

#include "thbc-base.h"
#include "thbc-server.h"
#include "thbc-config.h"

typedef struct thbc_server_t thbc_server_t;

typedef struct thbc_viewer_state_t thbc_viewer_state_t;
struct thbc_viewer_state_t {

    bool running;

    Font font;
    size_t font_width;

    Image logo;
    Texture2D logo_texture;
    int logo_width, logo_height;

};
#define empty_thbc_viewer_state_t                       (thbc_viewer_state_t){0}

int                     thbc_init_viewer                (thbc_viewer_state_t* viewer_state, thbc_server_t* server);
void                    thbc_update_viewer              (thbc_viewer_state_t* viewer_state, thbc_server_t* server);
void                    thbc_close_viewer               (thbc_viewer_state_t* viewer_state, thbc_server_t* server);

#endif // THBC_VIEWER_H