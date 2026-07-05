#if !defined(THBC_SERVER_H)
#define THBC_SERVER_H

#include "thbc-base.h"
#include "thbc-config.h"

typedef struct thbc_server_t thbc_server_t;
struct thbc_server_t {
    
    char* ip_addr;
    Texture2D ip_qr;

    pthread_t server_thread;
    http_server_t http_server;

    char* manager_page_html;
    size_t manager_page_length;

};
#define empty_thbc_server_t                             (thbc_server_t){0}

int                     thbc_start_server               (thbc_server_t* server);
void                    thbc_stop_server                (thbc_server_t* server);

#endif // THBC_SERVER_H