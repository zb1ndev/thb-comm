#if !defined(THBC_CONFIG_H)
#define THBC_CONFIG_H

#include "thbc-base.h"
#include "thbc-viewer.h"

typedef struct thbc_viewer_state_t thbc_viewer_state_t;
typedef Image (*thbc_slide_callback_t)(thbc_viewer_state_t* viewer_state);

typedef struct thbc_slide_t thbc_slide_t;
struct thbc_slide_t {
    char* name, image_path;
    thbc_slide_callback_t render;   // If render is NULL and image_path is REAL, render image at path as slide 
};

typedef struct thbc_quadrant_t thbc_quadrant_t;
struct thbc_quadrant_t {
    size_t slide_count;
    thbc_slide_t slides[10];        // Max Number of Slides
};

typedef struct thbc_config_t thbc_config_t;
struct thbc_config_t {

    // VIEWER FACING: If lock is on, it has updated and we can read metrics 
    // without the server touching it, and if its off the server has control.
    atomic_bool config_lock;    

    char* raw_config;
    size_t raw_config_size;

    // Metrics
    float pcya,
          order_cont_pcya,
          food_variance, 
          labor_variance,
          adt, 
          extremes, 
          cash;

    Color bg_color, text_color, accent_color;
    thbc_quadrant_t quadrants[4];

};
#define empty_thbc_config_t                             (thbc_config_t){0}

void                    thbc_set_config_lock            (bool value);
bool                    thbc_get_config_lock            (void);

thbc_config_t*          thbc_get_configuration          (void);
int                     thbc_load_configuration         (void);
void                    thbc_unload_configuration       (void);
int                     thbc_save_configuration         (void);

#endif // THBC_CONFIG_H