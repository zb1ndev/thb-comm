#include "thbc-viewer-api.c"

int thbc_viewer_init(thbc_viewer_state_t* viewer_state, thbc_server_t* server) {

    InitWindow(1280, 720, "THB Store Communications Viewer");
    if (!IsWindowFullscreen())
        ToggleFullscreen();
    SetTargetFPS(60);
    HideCursor();

    // NOTE(Joel Zbinden): Loading Last Server Resource, has to be done after GL initialization
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

    if (thbc_load_viewer_resources(viewer_state, server) < 0)
        return -1;

    viewer_state->running = true;
    return 0;

}

void thbc_viewer_update(thbc_viewer_state_t* viewer_state, thbc_server_t* server) {

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
    
    if (!thbc_config_get_read_lock()) {
        if (thbc_config_load(viewer_state, server) < 0)
            fprintf(stderr, "error: failed to load congfig");
        thbc_config_set_read_lock(true);
    }

    BeginDrawing();
       
        ClearBackground(BLACK);
        thbc_draw_crest(viewer_state);
        for (size_t i = 0; i < 4; i++)
            thbc_draw_quadrant(viewer_state, i+1, 10);

    EndDrawing();

}

void thbc_viewer_close(thbc_viewer_state_t* viewer_state, thbc_server_t* server) {

    thbc_config_unload();

    if (IsImageValid(viewer_state->logo))
        UnloadImage(viewer_state->logo);
    
    if (IsTextureValid(viewer_state->logo_texture))
        UnloadTexture(viewer_state->logo_texture);

    if (IsFontValid(viewer_state->font))
        UnloadFont(viewer_state->font);

    // NOTE(Joel Zbinden): Have to unload textures before GL deinitialization,
    // so we unload the QR Code texture here instead of in server clean-up
    if (IsTextureValid(server->ip_qr))
        UnloadTexture(server->ip_qr); 

    CloseWindow();

}