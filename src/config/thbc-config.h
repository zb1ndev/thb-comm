#if !defined(THBC_CONFIG_H)
#define THBC_CONFIG_H

#include "thbc-base.h"
#include "thbc-server.h"
#include "thbc-viewer.h"

#define THBC_MAXIMUM_CONFIG_KVPS                        47

typedef struct thbc_server_t thbc_server_t;
typedef struct thbc_viewer_state_t thbc_viewer_state_t;

typedef struct thbc_config_t thbc_config_t;
struct thbc_config_t {

    // NOTE(Joel Zbinden): VIEWER FACING: If read lock is on, the server has control
    // once the server releases control, the viewer can update its
    // slides with the new data and give control back to the server
    // once finished.
    
    atomic_bool read_lock;

    thbc_kvp data[THBC_MAXIMUM_CONFIG_KVPS];
    size_t data_size;

};
#define empty_thbc_config_t                             (thbc_config_t){0}


void                    thbc_config_set_read_lock       (bool value);
bool                    thbc_config_get_read_lock       (void);
thbc_config_t*          thbc_config_get                 (void);


int                     thbc_config_load                (thbc_viewer_state_t* viewer_state, thbc_server_t* server);
void                    thbc_config_unload              (void);
int                     thbc_config_save                (void);

char*                   thbc_config_get_value           (char* key);

#endif // THBC_CONFIG_H