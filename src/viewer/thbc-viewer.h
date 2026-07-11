#if !defined(THBC_VIEWER_H)
#define THBC_VIEWER_H

#include "thbc-base.h"
#include "thbc-server.h"
#include "thbc-config.h"

#define THBC_MAX_SLIDES 10
typedef struct thbc_server_t thbc_server_t;

typedef struct thbc_viewer_state_t thbc_viewer_state_t;
typedef Texture2D (*thbc_slide_callback_t)(thbc_viewer_state_t* viewer_state, thbc_server_t* server, Rectangle bounds);

typedef struct thbc_slide_t thbc_slide_t;
struct thbc_slide_t {

    char *name, *image_path;
    Texture2D render_result;
    thbc_slide_callback_t render; // NOTE(Joel Zbinden): If render is NULL and image_path is REAL, render image at path as slide 

};
#define empty_thbc_slide_t                              (thbc_slide_t){0}

typedef struct thbc_quadrant_t thbc_quadrant_t;
struct thbc_quadrant_t {

    size_t slide_count;
    thbc_slide_t slides[THBC_MAX_SLIDES];

};
#define empty_thbc_quadrant_t                           (thbc_quadrant_t){0}

typedef struct thbc_viewer_state_t thbc_viewer_state_t;
struct thbc_viewer_state_t {

    Font font;
    size_t font_width;

    Image logo;
    Texture2D logo_texture;
    int logo_width, logo_height;

    bool running;
    thbc_quadrant_t quadrants[4];

};
#define empty_thbc_viewer_state_t                       (thbc_viewer_state_t){0}

int                     thbc_viewer_init                (thbc_viewer_state_t* viewer_state, thbc_server_t* server);
void                    thbc_viewer_update              (thbc_viewer_state_t* viewer_state, thbc_server_t* server);
void                    thbc_viewer_close               (thbc_viewer_state_t* viewer_state, thbc_server_t* server);

#endif // THBC_VIEWER_H