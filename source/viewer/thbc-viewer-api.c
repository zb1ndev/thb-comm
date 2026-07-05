#include "thbc-viewer.h"
internal int render_width, render_height;

internal void thbc_draw_crest(thbc_viewer_state_t* viewer_state) {

    thbc_config_t* configuration = thbc_get_configuration();
    Vector2 logo_position = (Vector2) {
        .x = (render_width / 2) - (viewer_state->logo_width / 2),
        .y = 25
    };

    DrawLine(
        50, 25 + (viewer_state->logo_height / 2), 
        logo_position.x, 25 + (viewer_state->logo_height / 2), 
        DARKGRAY    
    );

    DrawLine(
        render_width/2, 25 + viewer_state->logo_height, 
        render_width/2, render_height - 25, 
        DARKGRAY    
    );

    DrawLine(
        render_width - 50, 25 + (viewer_state->logo_height / 2), 
        logo_position.x + viewer_state->logo_width, 25 + (viewer_state->logo_height / 2), 
        DARKGRAY
    );

    DrawTextureEx(
        viewer_state->logo_texture, 
        logo_position,
        0, 0.5f, 
        configuration->text_color
    );

}

internal Image thbc_qrcode_to_image(const uint8_t qr_code[], int scale, int border, Color dark_color, Color light_color) {

    int qr_size = qrcodegen_getSize(qr_code);
    int image_size = (qr_size + (border * 2)) * scale;

    Image result = GenImageColor(image_size, image_size, light_color);

    for (int y = 0; y < qr_size; y++) {
        for (int x = 0; x < qr_size; x++) {
            if (qrcodegen_getModule(qr_code, x, y)) {
                int px = (x + border) * scale;
                int py = (y + border) * scale;
                for (int dy = 0; dy < scale; dy++) {
                    for (int dx = 0; dx < scale; dx++) {
                        ImageDrawPixel(&result, px + dx, py + dy, dark_color);
                    }
                }
            }
        }
    }

    return result;

}

// NOTE: Ran only once at start-up
internal int thbc_load_viewer_resources(thbc_viewer_state_t* viewer_state) {

    viewer_state->logo = LoadImage("./resources/thb-nb.png");
    if (!IsImageValid(viewer_state->logo)) {
        perror("thbc_load_viewer_resources: LoadImage");
        goto fail;
    }

    viewer_state->logo_texture = LoadTextureFromImage(viewer_state->logo);
    if (!IsTextureValid(viewer_state->logo_texture)) {
        perror("thbc_load_viewer_resources: LoadTextureFromImage");
        goto fail;
    }

    // We only use it with a 0.5f scale factor, so just pre-compute it
    viewer_state->logo_width  = (int)(viewer_state->logo.width  / 2);
    viewer_state->logo_height = (int)(viewer_state->logo.height / 2);

    // Load roboto font
    viewer_state->font = LoadFontEx("./resources/roboto.ttf", 32, NULL, 0);
    if (!IsFontValid(viewer_state->font)) {
        perror("thbc_load_viewer_resources: LoadFont");
        goto fail;
    }

    Vector2 font_size = MeasureTextEx(viewer_state->font, "X", 32, 2);
    viewer_state->font_width = font_size.x;

    if (thbc_load_configuration() < 0)
        goto fail;
    return 0;

fail:

    thbc_unload_configuration();

    if (IsImageValid(viewer_state->logo))
        UnloadImage(viewer_state->logo);
    
    if (IsTextureValid(viewer_state->logo_texture))
        UnloadTexture(viewer_state->logo_texture);

    if (IsFontValid(viewer_state->font))
        UnloadFont(viewer_state->font);

    return -1;

}

internal Rectangle thbc_get_quadrant_bounds(thbc_viewer_state_t* viewer_state, int quadrant) {

    const int MARGIN            = 50;
    const int TOP_MARGIN        = (viewer_state->logo_texture.height/2)-MARGIN;
    const int half_height       = (render_height / 2);
    const int half_width        = (render_width / 2);

    // Maximum of 4 Quadrants lol (aaron joke haha)
    if (quadrant >= 4) quadrant = 4;
    if (quadrant <= 0) quadrant = 0;

    return (Rectangle) {
        .height     = half_height-(MARGIN*2)-(quadrant<3?TOP_MARGIN:-MARGIN),
        .width      = half_width-(MARGIN*2),
        .x          = (quadrant==2||quadrant==4?MARGIN+half_width:MARGIN),
        .y          = (MARGIN)+(quadrant<3?TOP_MARGIN:half_height-MARGIN),
    }; // X, Y is from top left

}

internal void thbc_draw_quadrant(thbc_viewer_state_t* viewer_state, int id, int interval) {
    
    local_persist int current_slide = 0;
    thbc_config_t* configuration = thbc_get_configuration();
    thbc_quadrant_t* quadrant = &configuration->quadrants[id];

    const char* title = quadrant->slide_count > 0
        ? quadrant->slides[current_slide].name
        : "XXXX>XXXX";

    Rectangle quadrant_bounds = thbc_get_quadrant_bounds(viewer_state, id);
    DrawRectangleRoundedLines(quadrant_bounds, 0.1f, 10, configuration->accent_color);

    const int TITLE_SIZE = 20;
    const int TITLE_MARGIN = 20;
    const int TITLE_LENGTH = MeasureTextEx(viewer_state->font, title, TITLE_SIZE, 2).x + TITLE_MARGIN;

    DrawRectangle(
        quadrant_bounds.x+50, 
        quadrant_bounds.y-(TITLE_SIZE/2), 
        TITLE_LENGTH,
        TITLE_SIZE,
        BLACK
    );

    DrawTextEx(
        viewer_state->font, 
        title,
        (Vector2) {
            quadrant_bounds.x+50+(TITLE_MARGIN/2), 
            quadrant_bounds.y-(TITLE_SIZE/2)
        }, 
        TITLE_SIZE, 2,
        configuration->text_color
    );

    for (size_t i = 0; i < quadrant->slide_count; i++) {

        DrawCircle(
            quadrant_bounds.x + TITLE_LENGTH + 50 + (TITLE_MARGIN) + (i*TITLE_MARGIN), 
            quadrant_bounds.y, 
            (TITLE_MARGIN/2), 
            BLACK
        );

        DrawCircle(
            quadrant_bounds.x + TITLE_LENGTH + 50 + (TITLE_MARGIN) + (i*TITLE_MARGIN), 
            quadrant_bounds.y, 
            5, 
            i==0?WHITE:BLACK
        );

    }

    // TODO: Draw Slides

}

internal thbc_slide_t* thbc_get_quadrant_slides(thbc_viewer_state_t* viewer_state, int quadrant, size_t* size_out) {

    thbc_slide_t* slides = NULL;
    thbc_config_t* configuration = thbc_get_configuration();
    Rectangle bounds = thbc_get_quadrant_bounds(viewer_state, quadrant);

    #define TOP_LEFT_QUADRANT 1
    #define TOP_RIGHT_QUADRANT 2
    #define BOTTOM_LEFT_QUADRANT 3
    #define BOTTOM_RIGHT_QUADRANT 4

    switch (quadrant) {

        case TOP_LEFT_QUADRANT: {

        } break;

        case TOP_RIGHT_QUADRANT: {

        } break;

        case BOTTOM_LEFT_QUADRANT: {

        } break;

        case BOTTOM_RIGHT_QUADRANT: {

        } break;
            
        default: fprintf(stderr, "error: invalid quadrant"); break;

    }

    return NULL;

}