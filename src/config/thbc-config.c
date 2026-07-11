#include "thbc-config.h"
internal thbc_config_t* global_configuration = NULL;

void thbc_config_set_read_lock(bool value) {

    atomic_store(&global_configuration->read_lock, value);

}

bool thbc_config_get_read_lock(void) { 

    return atomic_load(&global_configuration->read_lock);

}

thbc_config_t* thbc_config_get(void) {

    return global_configuration;

}

char* thbc_config_get_value(char* key) {

    for (size_t i = 0; i < global_configuration->data_size; i++)
        if (strcmp(key, global_configuration->data[i].key) == 0)
            return global_configuration->data[i].value;
    return NULL;

}

internal int thbc_config_set_value(char* key, char* value) {

    // TODO: Find or Create kvp based on key and allocate space for value then copy

    return 0;

}

internal void thbc_config_update(thbc_config_t* config, char* data, size_t data_length) {

    char* config_file = LoadFileText("./resources/thbc.config");
    size_t config_file_size = strlen(config_file);
        
    // TODO: Parse kvps and load them into the config

    if (data == NULL || data_length == 0)
        return; // Exit early, there is no more work to do

    // TODO: Parse config format into kvps

}

int thbc_config_load(thbc_viewer_state_t* viewer_state, thbc_server_t* server) {

    bool first_load = false;
    thbc_config_t* temp = global_configuration;

    // NOTE: Only gets ran once before server startup
    if (global_configuration == NULL) {
        
        temp = calloc(1, sizeof(thbc_config_t));
        if (temp == NULL) {
            perror("thbc_load_configuration: calloc");
            goto fail;
        }

        first_load = true;
        atomic_init(&temp->read_lock, true); // Give Server Control
    
    }

    thbc_config_update(temp, NULL, 0);
    
    if (thbc_config_save() < 0) {
        perror("thbc_load_configuration: thbc_save_configuration");
        goto fail;
    } 

    // TODO: Render Slides Here

    /* For the time being, and for testing I am only going to render one quadrant and put some value there + the QR Code */ {

        int width = GetRenderWidth();
        int height = GetRenderHeight();

        char lan_server_url[INET_ADDRSTRLEN + sizeof("http://:XXXX")] = {0};
        if (sprintf(lan_server_url, "http://%s:%d", server->ip_addr, server->http_server.port) < 0) {
            perror("thbc_init_viewer: sprintf");
            return -1;
        }
        Vector2 text_size = MeasureTextEx(viewer_state->font, lan_server_url, 32, 2);

        char labor_variance [sizeof("4294967295") + sizeof("Labor Variance: %")] = {0};
        // if (sprintf(labor_variance, "Labor Variance: %s%%", thbc_config_get_value("labor_variance")) < 0) {
        if (sprintf(labor_variance, "Labor Variance: %s%%", "0.1") < 0) {
            perror("thbc_init_viewer: sprintf");
            return -1;
        }

        char food_variance [sizeof("4294967295") + sizeof("Food Variance: %")] = {0};
        // if (sprintf(food_variance, "Food Variance: %s%%", thbc_config_get_value("food_variance")) < 0) {
        if (sprintf(food_variance, "Food Variance: %s%%", "0.1") < 0) {
            perror("thbc_init_viewer: sprintf");
            return -1;
        }

        RenderTexture2D slide = LoadRenderTexture(width, height);
        BeginTextureMode(slide);
            
            ClearBackground(BLACK);

            DrawTextEx(
                viewer_state->font, 
                labor_variance,
                (Vector2){
                    25, 
                    25
                }, 
                32, 2,
                WHITE
            );

            DrawTextEx(
                viewer_state->font, 
                food_variance,
                (Vector2){
                    25,
                    25+32+25
                }, 
                32, 2,
                WHITE
            );

            DrawTextEx(
                viewer_state->font, 
                lan_server_url,
                (Vector2){
                    ((float)width-(width/4))-(text_size.x/2), 
                    height/2 + (server->ip_qr.height/2) - text_size.y
                }, 
                32, 2,
                WHITE
            );

            DrawTextureEx(
                server->ip_qr, 
                (Vector2){
                    ((float)width-(width/4))-(server->ip_qr.height/2), 
                    height/2-(server->ip_qr.height/2)-text_size.y
                }, 
                0, 1.0f, 
                WHITE
            );

            DrawLine(
                width/2, 25, 
                width/2, height - 25, 
                (Color){25,25,25,255}    
            );            

        EndTextureMode();

        for (size_t i = 0; i < 1; i++) {
            viewer_state->quadrants[i].slide_count = 2;
            viewer_state->quadrants[i].slides[0] = (thbc_slide_t) {
                .name = "Test Slide",
                .render_result = slide.texture,
                .render = NULL,
                .image_path = NULL
            };
            viewer_state->quadrants[i].slides[1] = (thbc_slide_t) {
                .name = "Test Slide 2",
                .render_result = slide.texture,
                .render = NULL,
                .image_path = NULL
            };
        }

    }

    global_configuration = temp;
    return thbc_config_save();

fail:

    if (first_load == true)
        thbc_config_unload();
    return -1;

}

void thbc_config_unload(void) {
    
    if (global_configuration != NULL)
        free(global_configuration);
    else return;

}

int thbc_config_save(void) {

    // TODO: Take kvps and save changes to the config file

    return 0;

}