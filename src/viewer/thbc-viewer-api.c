#include "thbc-viewer.h"
internal int render_width, render_height;

internal void thbc_draw_crest(thbc_viewer_state_t* viewer_state) {

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
        WHITE
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

// NOTE(Joel Zbinden): Ran only once at start-up
internal int thbc_load_viewer_resources(thbc_viewer_state_t* viewer_state, thbc_server_t* server) {

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
    viewer_state->font = LoadFontEx("./resources/jetbrains.ttf", 32, NULL, 0);
    if (!IsFontValid(viewer_state->font)) {
        perror("thbc_load_viewer_resources: LoadFont");
        goto fail;
    }

    Vector2 font_size = MeasureTextEx(viewer_state->font, "X", 32, 2);
    viewer_state->font_width = font_size.x;

    if (thbc_config_load(viewer_state, server) < 0)
        goto fail;
    return 0;

fail:

    thbc_config_unload();

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
    const int DOUBLE_MARGIN     = (MARGIN * 2);

    const int CREST_HEIGHT      = (viewer_state->logo_texture.height / 2);
    const int REAL_HEIGHT       = (render_height - CREST_HEIGHT);
    const int QUAD_HEIGHT       = ((REAL_HEIGHT / 2) - MARGIN);
    const int QUAD_WIDTH        = ((render_width / 2) - DOUBLE_MARGIN);

    const int HALF_HEIGHT       = (render_height / 2);
    const int HALF_WIDTH        = (render_width / 2);

    // Maximum of 4 Quadrants lol (aaron joke haha)
    if (quadrant >= 4) quadrant = 4;
    if (quadrant <= 0) quadrant = 0;

    return (Rectangle) {
        .height = QUAD_HEIGHT,
        .width = QUAD_WIDTH,
        .x = (quadrant%2==0?(HALF_WIDTH+MARGIN):MARGIN),
        .y = (CREST_HEIGHT+(quadrant<3?0:(QUAD_HEIGHT+MARGIN)))
    };

}

internal void thbc_draw_quadrant(thbc_viewer_state_t* viewer_state, int id, int interval) {

    local_persist int slide_increment = 0;
    local_persist float elapsed = 0.0f;

    elapsed += GetFrameTime() / 3.5f;
    if (elapsed > (float)interval) {
        slide_increment = (slide_increment + 1) % 10;
        elapsed = 0.0f;
    }

    thbc_quadrant_t* quadrant       = &viewer_state->quadrants[id-1];
    Rectangle quadrant_bounds       = thbc_get_quadrant_bounds(viewer_state, id);
    
    thbc_slide_t* current_slide     = NULL;
    thbc_slide_t* slides            = quadrant->slides; 
    size_t slide_count              = quadrant->slide_count;

    const char* title               = "PLACE HOLDER";
    
    if (slide_count > 0) {

        const float PADDING = 20.0f;
        current_slide = &slides[slide_increment % slide_count];

        Texture2D texture = current_slide->render_result;
        Rectangle destination = (Rectangle) {
            .height = quadrant_bounds.height - PADDING,
            .width = quadrant_bounds.width - PADDING,
            .x = quadrant_bounds.x + (PADDING/2),
            .y = quadrant_bounds.y + (PADDING/2)
        };

        DrawTexturePro(
            texture, 
            (Rectangle){
                0, 0, 
                (float)texture.width, 
                -(float)texture.height
            }, 
            destination, 
            (Vector2){0,0}, 0.0f, 
            WHITE
        );

        title = current_slide->name;

    }

    DrawRectangleLinesEx(quadrant_bounds, 1.0f, DARKGRAY);

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
        WHITE
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
            i==(slide_increment % slide_count)?WHITE:BLACK
        );

    }

}