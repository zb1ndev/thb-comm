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

internal int thbc_config_set_value(thbc_config_t* config, char* key, char* value) {

    thbc_kvp_t* pair = NULL;
    size_t value_len = strlen(value);
    size_t size = THBC_DEFAULT_CONFIG_VALUE_SIZE;

    printf("%s: %s\n", key, value);

    for (size_t i = 0; i < config->data_size; i++) {
        if (config->data[i].key != NULL) {
            if (strcmp(key, config->data[i].key) == 0){
                pair = &config->data[i];
                break;
            }
        }
    }

    if (pair == NULL) {
        
        pair = &config->data[config->data_size++];
        pair->key = calloc(strlen(key) + 1, sizeof(char));
        if (pair->key == NULL) {
            perror("thbc_config_set_value: calloc");
            return -1;
        }
        memcpy(pair->key, key, strlen(key));

        if (size < value_len) 
            size = value_len;

        pair->value = calloc(size + 1, sizeof(char));
        if (pair->value == NULL) {
            perror("thbc_config_set_value: calloc");
            return -1;
        }

        memcpy(pair->value, value, value_len);
        return 0;

    }

    if (size < value_len) {
        size = value_len;
        free(pair->value);
    } 

    pair->value = calloc(size + 1, sizeof(char));
    if (pair->value == NULL) {
        perror("thbc_config_set_value: calloc");
        return -1;
    }

    memcpy(pair->value, value, value_len);
    return 0;

}

internal void thbc_config_parse_quadrant_slides(thbc_config_t* config, char* data, size_t data_length, int id) {

    char* walker = data;
    int slide_counter = 0;
    *(data + data_length + 1) = '\0';

    do {

        if (slide_counter + 1 > 10)
            break;

        char buf[64] = {0};
        sprintf(buf, "quadrant_%d::%d", id, ++slide_counter);

        char* value_start = strchr(walker, '"');
        if (value_start == NULL)
            break;
        value_start++;

        char* value_end = strchr(value_start, '"'); 
        if (value_end == NULL)
            break;
        *value_end = '\0';

        if (thbc_config_set_value(config, buf, value_start) < 0) {
            perror("thbc_config_parse_quadrant_slides: thbc_config_set_value");
            return;
        }

        walker = value_end + 1;

    } while (true);
    
}

// NOTE(Joel Zbinden): Ran by the server to update the config
void thbc_config_update(thbc_config_t* config, char* data, size_t data_length) {

    char* scratch = calloc(data_length + 1, sizeof(char));
    if (scratch == NULL) {
        perror("thbc_config_update: calloc");
        return;
    }
    memcpy(scratch, data, data_length);

    // TODO: Parse Default

    char* quadrant_keys[4] = {
        "quadrant_1", "quadrant_2",
        "quadrant_3", "quadrant_4"
    };

    for (size_t i = 0; i < 4; i++) {
        
        char* start = memmem(scratch, data_length, quadrant_keys[i], 10);
        char* end = strchr(start, '}');

        thbc_config_parse_quadrant_slides(config, start, end - start, i+1);
    
    }
    

    free(scratch);

}

int thbc_config_load(thbc_viewer_state_t* viewer_state, thbc_server_t* server) {

    bool first_load = false;
    thbc_config_t* temp = global_configuration;

    // NOTE(Joel Zbinden): Only gets ran once before server startup
    if (global_configuration == NULL) {
        
        temp = calloc(1, sizeof(thbc_config_t));
        if (temp == NULL) {
            perror("thbc_load_configuration: calloc");
            goto fail;
        }

        first_load = true;
        atomic_init(&temp->read_lock, true); // NOTE(Joel Zbinden): Give server control once finished initializing 

        char* config_file = LoadFileText("./resources/thbc.config");
        size_t config_file_size = strlen(config_file);
        thbc_config_update(temp, config_file, config_file_size);

    }
    
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