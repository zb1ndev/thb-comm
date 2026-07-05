#include "thbc-config.h"
global thbc_config_t* global_configuration = NULL;

void thbc_set_config_lock(bool value)       { atomic_store(&global_configuration->config_lock, value); }
bool thbc_get_config_lock(void)             { return atomic_load(&global_configuration->config_lock);  }
thbc_config_t* thbc_get_configuration(void) { return global_configuration; }

int thbc_load_configuration(void) {

    bool first_load = false;
    thbc_config_t* temp = global_configuration;

    if (global_configuration == NULL) {
        
        temp = calloc(1, sizeof(thbc_config_t));
        if (temp == NULL) {
            perror("load_resources: calloc");
            goto fail;
        }

        first_load = true;
        atomic_init(&temp->config_lock, false);

        // TODO: Load from file into raw_config
        // have a back-up with "n/a" everywhere
    
    }

    temp->bg_color = BLACK;         // TODO: Load this from config
    temp->text_color = WHITE;       // TODO: Load this from config
    temp->accent_color = GRAY;      // TODO: Load this from config

    // TODO: Render Slides Here

    global_configuration = temp;
    return thbc_save_configuration();

fail:

    if (first_load) {  
        thbc_unload_configuration();
    } return -1;

}

void thbc_unload_configuration(void) {
    
    if (global_configuration != NULL)
        free(global_configuration);
    else return;

}

int thbc_save_configuration(void) {



    return 0;

}