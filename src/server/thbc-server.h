#if !defined(THBC_SERVER_H)
#define THBC_SERVER_H

#include "thbc-base.h"
#include "thbc-config.h"

typedef enum {

    PG_LOGIN, PG_HOME, PG_MANAGER,
    PG_COUNT

} thbc_page_type_t;

typedef struct thbc_page_t thbc_page_t;
struct thbc_page_t {
    char* data;
    size_t size;
};
#define empty_thbc_page_t                               (thbc_page_t){0}

typedef struct thbc_server_t thbc_server_t;
struct thbc_server_t {
    
    char* ip_addr;
    Texture2D ip_qr;

    pthread_t server_thread;
    http_server_t http_server;

    thbc_page_t pages[PG_COUNT];

};
#define empty_thbc_server_t                             (thbc_server_t){0}

int                     thbc_server_start               (thbc_server_t* server);
void                    thbc_server_stop                (thbc_server_t* server);

#endif // THBC_SERVER_H