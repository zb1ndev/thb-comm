#include "thbc-viewer-api.c"

int thbc_init_viewer(thbc_viewer_state_t* viewer_state, thbc_server_t* server) {

    InitWindow(1280, 720, "THB Store Communications Viewer");
    if (!IsWindowFullscreen())
        ToggleFullscreen();
    SetTargetFPS(60);
    HideCursor();

    // Loading Last Server Resource
    // Has to be done after GL initialization
    char lan_server_url[INET_ADDRSTRLEN + sizeof(":XXXX")] = {0};
    if (sprintf(lan_server_url, "http://%s:%d", server->ip_addr, server->http_server.port) < 0) {
        perror("thbc_init_viewer: sprintf");
        return -1;
    }

    uint8_t qr_code[qrcodegen_BUFFER_LEN_MAX];
	uint8_t temp_buffer[qrcodegen_BUFFER_LEN_MAX];
    enum qrcodegen_Ecc error_correction_level = qrcodegen_Ecc_LOW;
    
	if (!qrcodegen_encodeText(
        lan_server_url,
        temp_buffer,
        qr_code, 
        error_correction_level,
		qrcodegen_VERSION_MIN, 
        qrcodegen_VERSION_MAX, 
        qrcodegen_Mask_AUTO, 
        true
    )) {
        perror("thbc_init_viewer: qrcodegen_encodeText");
        return -1;
    }

    Image qr_image = thbc_qrcode_to_image(qr_code, 10, 4, WHITE, BLACK);
    server->ip_qr = LoadTextureFromImage(qr_image);

    if (thbc_load_viewer_resources(viewer_state) < 0)
        return -1;

    viewer_state->running = true;
    return 0;

}

void thbc_update_viewer(thbc_viewer_state_t* viewer_state, thbc_server_t* server) {

    thbc_config_t* configuration = thbc_get_configuration();
    render_height = GetRenderHeight();
    render_width = GetRenderWidth();

    if (WindowShouldClose()) {
        viewer_state->running = false;
        return;
    }

    if (IsKeyPressed(KEY_F))
        ToggleFullscreen();

    if (IsKeyPressed(KEY_C))
        if (IsCursorHidden()) ShowCursor(); else HideCursor();
    
    if (thbc_get_config_lock()) {
        if (thbc_load_configuration() < 0)
            fprintf(stderr, "error: failed to load congfig");
        thbc_set_config_lock(false);
    }

    BeginDrawing();
       
        ClearBackground(configuration->bg_color);
        thbc_draw_crest(viewer_state);

        for (size_t i = 0; i < 4; i++) {
            size_t num_slides = 0;
            thbc_slide_t* slides = thbc_get_quadrant_slides(viewer_state, i+1, &num_slides);
            thbc_draw_quadrant(viewer_state, i+1, 10000);
        }

        // TODO: Put this somewhere
        // DrawTextureEx(server->ip_qr, (Vector2){50, 25}, 0, 0.2f, configuration->text_color);

    EndDrawing();

}

void thbc_close_viewer(thbc_viewer_state_t* viewer_state, thbc_server_t* server) {

    thbc_unload_configuration();

    if (IsImageValid(viewer_state->logo))
        UnloadImage(viewer_state->logo);
    
    if (IsTextureValid(viewer_state->logo_texture))
        UnloadTexture(viewer_state->logo_texture);

    if (IsFontValid(viewer_state->font))
        UnloadFont(viewer_state->font);

    // Have to unload textures before GL deinitialization
    // So we unload the QR Code texture here instead of in server clean-up
    if (IsTextureValid(server->ip_qr))
        UnloadTexture(server->ip_qr); 

    CloseWindow();

}