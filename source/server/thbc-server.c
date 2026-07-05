#include "thbc-server.h"
internal thbc_server_t* instance;

internal http_response_t thbc_comm_manager(http_server_t* server, http_request_t* request, string_t* uri_data, size_t uri_data_count) {

    (void)server;
    (void)request;
    (void)uri_data;
    (void)uri_data_count;

    return http_create_response(200, MIME_TYPE_HTML, instance->manager_page_html, instance->manager_page_length);

}

internal http_response_t thbc_404_page(http_server_t* server, http_request_t* request, string_t* uri_data, size_t uri_data_count) {

    (void)server;
    (void)request;
    (void)uri_data;
    (void)uri_data_count;

    if (request->data_length > 0)
        printf("%s\n", request->data);
    return http_create_response(404, MIME_TYPE_HTML, "404 PAGE NOT FOUND", 18);

}

internal void* thbc_server_worker(void* data) {
    
    // Block untill config is loaded for the first time, 
    // then give control to render thread before server takes it
    while (thbc_get_configuration() == NULL);
    thbc_set_config_lock(true);

    http_server_start(&((thbc_server_t*)data)->http_server);
    return NULL;

}

internal int thbc_load_pages(thbc_server_t* server) {

    const char* wd = GetWorkingDirectory();
    size_t wd_length = strlen(wd);

    char* path = malloc(wd_length + THBC_LONGEST_RESOURCE_NAME);
    if (path == NULL) {
        perror("thbc_start_server: malloc");
        free(server->ip_addr);
        return -1;
    } 

    // TODO: Make an internal function for loading pages
    (void)memset(path, 0, wd_length + THBC_LONGEST_RESOURCE_NAME);
    if (sprintf(path, "%s/resources/thb-comm-app.html", wd) < 0) {
        perror("thbc_start_server: sprintf");
        free(server->ip_addr);
        free(path);
        return -1;
    }

    server->manager_page_html = LoadFileText(path);
    if (server->manager_page_html == NULL) {
        perror("thbc_start_server: LoadFileText");
        free(server->ip_addr);
        free(path);
        return -1;
    }

    server->manager_page_length = strlen(server->manager_page_html);
    free(path);
    return 0;
        
}

int thbc_start_server(thbc_server_t* server) {

    http_route_t* routes = calloc(1, sizeof(http_route_t)); 
    routes[0] = (http_route_t) {
        .uri = "/",
        .handler = &thbc_comm_manager
    };

    server->http_server = (http_server_t) {

        .running = true,
        .max_events = 10,
        .port = 7374,
        .workers = 0,

        .route_404 = (http_route_t) {
            .uri = NULL, 
            .handler = &thbc_404_page
        },
        .routes = routes,
        .route_count = 1

    };

    if (thbc_load_pages(server) < 0) {
        perror("thbc_start_server: thbc_load_pages");
        return -1;
    }
    
    struct ifaddrs* ifa;
    struct ifaddrs* ifa_addr;
    
    server->ip_addr = calloc(INET_ADDRSTRLEN + 1, sizeof(char));
    if (server->ip_addr == NULL) {
        perror("thbc_start_server: calloc");
        return -1;
    }

    if (getifaddrs(&ifa_addr) < 0) {
        perror("thbc_start_server: getifaddrs");
        return -1;
    }

    for (ifa = ifa_addr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET)
            continue;
        if (strcmp(ifa->ifa_name, "lo") == 0)
            continue;
        struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
        inet_ntop(AF_INET, &addr->sin_addr, server->ip_addr, INET_ADDRSTRLEN);
    }

    freeifaddrs(ifa_addr);
    instance = server;

    if (pthread_create(&server->server_thread, NULL, &thbc_server_worker, server) < 0) {
        perror("thbc_start_server: pthread_create");
        return -1;
    }

    return 0;

}

void thbc_stop_server(thbc_server_t* server) {

    void* return_value = NULL;
    server->http_server.running = false;

    if (pthread_join(server->server_thread, &return_value) < 0) {
        perror("thbc_stop_server: pthread_join");
        return;
    }

    if (server->http_server.routes != NULL)
        free(server->http_server.routes);

    if (server->manager_page_html != NULL)
        UnloadFileText(server->manager_page_html);

    if (server->ip_addr != NULL)
        free(server->ip_addr);

}