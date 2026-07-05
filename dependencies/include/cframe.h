/*

    MIT License
    Copyright (c) 2026 Joel Zbinden

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

    NOTE: cframe.h only handles HTTP 1.0 and 1.1

*/

#if !defined(CFRAME_H)
#define CFRAME_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <pthread.h> 

////////////////////////////////////////////////////////
// UTILITIES                                          //
////////////////////////////////////////////////////////

#define KB 1024

#define ERROR_MESSAGE(code, message)                                                    \
    "HTTP/1.1 "#code" "#message"\r\n"                                                   \
    "Connection: close\r\n"                                                             \
    "\r\n"                                                                              \

#define ERROR_100 ERROR_MESSAGE(100, Continue)
#define ERROR_101 ERROR_MESSAGE(101, Switching Protocols)
#define ERROR_102 ERROR_MESSAGE(102, Processing)
#define ERROR_103 ERROR_MESSAGE(103, Early Hints)

#define ERROR_200 ERROR_MESSAGE(200, OK)
#define ERROR_201 ERROR_MESSAGE(201, Created)
#define ERROR_202 ERROR_MESSAGE(202, Accepted)
#define ERROR_203 ERROR_MESSAGE(203, Non-Authoritative Information)
#define ERROR_204 ERROR_MESSAGE(204, No Content)
#define ERROR_205 ERROR_MESSAGE(205, Reset Content)
#define ERROR_206 ERROR_MESSAGE(206, Partial Content)
#define ERROR_207 ERROR_MESSAGE(207, Multi-Status)
#define ERROR_208 ERROR_MESSAGE(208, Already Reported)
#define ERROR_226 ERROR_MESSAGE(226, IM Used)

#define ERROR_300 ERROR_MESSAGE(300, Multiple Choices)
#define ERROR_301 ERROR_MESSAGE(301, Moved Permanently)
#define ERROR_302 ERROR_MESSAGE(302, Found)
#define ERROR_303 ERROR_MESSAGE(303, See Other)
#define ERROR_304 ERROR_MESSAGE(304, Not Modified)
#define ERROR_305 ERROR_MESSAGE(305, Use Proxy)
#define ERROR_307 ERROR_MESSAGE(307, Temporary Redirect)
#define ERROR_308 ERROR_MESSAGE(308, Permanent Redirect)

#define ERROR_400 ERROR_MESSAGE(400, Bad Request)
#define ERROR_401 ERROR_MESSAGE(401, Unauthorized)
#define ERROR_402 ERROR_MESSAGE(402, Payment Required)
#define ERROR_403 ERROR_MESSAGE(403, Forbidden)
#define ERROR_404 ERROR_MESSAGE(404, Not Found)
#define ERROR_405 ERROR_MESSAGE(405, Method Not Allowed)
#define ERROR_406 ERROR_MESSAGE(406, Not Acceptable)
#define ERROR_407 ERROR_MESSAGE(407, Proxy Authentication Required)
#define ERROR_408 ERROR_MESSAGE(408, Request Timeout)
#define ERROR_409 ERROR_MESSAGE(409, Conflict)
#define ERROR_410 ERROR_MESSAGE(410, Gone)
#define ERROR_411 ERROR_MESSAGE(411, Length Required)
#define ERROR_412 ERROR_MESSAGE(412, Precondition Failed)
#define ERROR_413 ERROR_MESSAGE(413, Content Too Large)
#define ERROR_414 ERROR_MESSAGE(414, URI Too Long)
#define ERROR_415 ERROR_MESSAGE(415, Unsupported Media Type)
#define ERROR_416 ERROR_MESSAGE(416, Range Not Satisfiable)
#define ERROR_417 ERROR_MESSAGE(417, Expectation Failed)
#define ERROR_418 ERROR_MESSAGE(418, Im a Teapot)
#define ERROR_421 ERROR_MESSAGE(421, Misdirected Request)
#define ERROR_422 ERROR_MESSAGE(422, Unprocessable Content)
#define ERROR_423 ERROR_MESSAGE(423, Locked)
#define ERROR_424 ERROR_MESSAGE(424, Failed Dependency)
#define ERROR_425 ERROR_MESSAGE(425, Too Early)
#define ERROR_426 ERROR_MESSAGE(426, Upgrade Required)
#define ERROR_428 ERROR_MESSAGE(428, Precondition Required)
#define ERROR_429 ERROR_MESSAGE(429, Too Many Requests)
#define ERROR_431 ERROR_MESSAGE(431, Request Header Fields Too Large)
#define ERROR_451 ERROR_MESSAGE(451, Unavailable For Legal Reasons)

#define ERROR_500 ERROR_MESSAGE(500, Internal Server Error)
#define ERROR_501 ERROR_MESSAGE(501, Not Implemented)
#define ERROR_502 ERROR_MESSAGE(502, Bad Gateway)
#define ERROR_503 ERROR_MESSAGE(503, Service Unavailable)
#define ERROR_504 ERROR_MESSAGE(504, Gateway Timeout)
#define ERROR_505 ERROR_MESSAGE(505, HTTP Version Not Supported)
#define ERROR_506 ERROR_MESSAGE(506, Variant Also Negotiates)
#define ERROR_507 ERROR_MESSAGE(507, Insufficient Storage)
#define ERROR_508 ERROR_MESSAGE(508, Loop Detected)
#define ERROR_510 ERROR_MESSAGE(510, Not Extended)
#define ERROR_511 ERROR_MESSAGE(511, Network Authentication Required)

#define err(e, m) {errno=e;}perror(m)

typedef char* string_t;

////////////////////////////////////////////////////////
// FORWARD DECLARATIONS                               //
////////////////////////////////////////////////////////

typedef struct http_route_t http_route_t;
typedef struct http_client_t http_client_t;
typedef struct dom_element_t dom_element_t;
typedef struct http_server_t http_server_t;
typedef struct http_response_t http_response_t;
typedef struct http_request_t http_request_t;

////////////////////////////////////////////////////////
// TYPE / EMPTY DEFINITIONS                           //
////////////////////////////////////////////////////////

/*
    A function pointer type for route handlers.
    @param server           The server calling the handler.
    @param data             The incoming request data.
    @param data_length      The size of the request data.
    @param uri_data         If the route uri is dynamic, this parameter will contain the encoded data.
    @param uri_data_count   The number of items in the `uri_data` list.
    @returns                A complete `http_response_t` that will be sent back to the client.
*/
typedef http_response_t (*route_handler_t)(http_server_t* server, http_request_t* request, string_t* uri_data, size_t uri_data_count);

/*
    Contains data associated with a particular route.
    
    Members:
     `uri` -        The uri associated with the route.
     `dynamic` -    Whether the `uri` contains encoded data.
     `handler` -    The `route_handler_t` function that will handle requests to this route. 
*/
struct http_route_t {
    string_t uri;
    bool dynamic;
    route_handler_t handler;
};
#define empty_http_route_t (http_route_t){0}

/*
    Contains server startup/runtime information.
    
    Members:
     `running` -        A switch for turning the server off.
     `port` -           The port the server will be hosted on.
     `fd` -             The file descriptor associated with this server instance.
     `workers` -        The number of threads/workers the server will instance.
     `routes` -         A list of user-defined routes that the server will serve.
     `route_404` -      A default route for handling 404 errors.
     `address` -        The address the server is hosted on.
     `max_events` -     The maximum number of epoll events.
     `route_count`  -   The number of items in the `routes` list.
*/
struct http_server_t {
    bool running;
    int port, fd, workers;
    http_route_t* routes;
    http_route_t route_404;
    struct sockaddr_in address;
    size_t max_events, route_count;
};
#define empty_http_server_t (http_server_t){0}

/*
    Contains data used to construct a response.
    
    Members:
     `data` -           The data being sent to the client.
     `data_length` -    The size of the data being sent to the client. 
*/
struct http_response_t {
    int status;
    string_t data;
    size_t data_length;
    const string_t content_type;
};
#define empty_http_response_t (http_response_t){0}

/*
    Contains data associated with a client.
    
    Members:
     `fd` -             The file descriptor where the client connection is located.
     `read_buffer` -    The data read from the client. 
     `write_buffer` -   The data being sent to the client. 
     `read_size` -      The size of the data inside `read_buffer`. 
     `read_offset` -    The amount of data already read from the client.
     `write_size` -     The size of the data inside `write_buffer`.
     `write_offset` -   The amount of data already sent to the client.
*/
struct http_client_t {
    int fd;
    bool keep_alive;
    char read_buffer[(4*KB)+1], *write_buffer;
    size_t read_size, read_offset, write_size, write_offset;
};
#define empty_http_client_t (http_client_t){0}

typedef enum {
    RS_ERROR    = -1,
    RS_DONE, 
    RS_AGAIN,
} http_req_state_t;

typedef enum {
    RT_INVAL    = -1,
    RT_GET,
    RT_POST,
    RT_PUT,
    RT_PATCH,
    RT_DELETE,
} http_req_type_t;

/*
    Contains data associated with a request.
    
    Members:
     `uri` -            The path specified in the request.
     `data` -           The data section of the request.
     `data_length` -    The size of the data section. 
     `type` -           The request method.
*/
struct http_request_t {
    size_t data_length;
    http_req_type_t type;
    string_t data, uri;
};
#define empty_http_request_t (http_request_t){0}

#define MIME_TYPE_HTML              "text/html"
#define MIME_TYPE_WASM              "application/wasm"
#define MIME_TYPE_JAVA_SCRIPT       "application/javascript"
#define MIME_TYPE_DATA              "application/octet-stream"
#define MIME_TYPE_DEFAULT           "application/octet-stream"

/*
    An enum containing many common html tags.
*/
typedef enum {
    TAG_A, TAG_ABBR, TAG_ADDRESS, TAG_AREA, TAG_ARTICLE, TAG_ASIDE, TAG_AUDIO, TAG_B,
    TAG_BASE, TAG_BDI, TAG_BDO, TAG_BLOCKQUOTE, TAG_BODY, TAG_BR, TAG_BUTTON, TAG_CANVAS,
    TAG_CAPTION, TAG_CITE, TAG_CODE, TAG_COL, TAG_COLGROUP, TAG_DATA, TAG_DATALIST, TAG_DD,
    TAG_DEL, TAG_DETAILS, TAG_DFN, TAG_DIALOG, TAG_DIV, TAG_DL, TAG_DT, TAG_EM, TAG_FIELDSET,
    TAG_FIGCAPTION, TAG_FIGURE, TAG_FOOTER, TAG_FORM, TAG_H1, TAG_H2, TAG_H3, TAG_H4, TAG_H5, 
    TAG_H6, TAG_HEAD, TAG_HEADER, TAG_HR, TAG_HTML, TAG_I, TAG_IFRAME, TAG_IMG, TAG_INPUT, 
    TAG_INS, TAG_KBD, TAG_LABEL, TAG_LEGEND, TAG_LI, TAG_LINK, TAG_MAIN, TAG_MAP, TAG_MARK,
    TAG_MENU, TAG_META, TAG_METER, TAG_NAV, TAG_NOSCRIPT, TAG_OBJECT, TAG_OL, TAG_OPTGROUP, 
    TAG_OPTION, TAG_OUTPUT, TAG_P, TAG_PARAM, TAG_PICTURE, TAG_PRE, TAG_PROGRESS, TAG_Q, TAG_RP,
    TAG_RT, TAG_RUBY, TAG_S, TAG_SAMP, TAG_SCRIPT, TAG_SECTION, TAG_SELECT, TAG_SMALL, TAG_SOURCE,
    TAG_SPAN, TAG_STRONG, TAG_STYLE, TAG_SUB, TAG_SUMMARY, TAG_SUP, TAG_TABLE, TAG_TBODY, TAG_TD,
    TAG_TEMPLATE, TAG_TEXTAREA, TAG_TFOOT, TAG_TH, TAG_THEAD, TAG_TIME, TAG_TITLE, TAG_TR, TAG_TRACK, 
    TAG_U, TAG_UL, TAG_VAR, TAG_VIDEO, TAG_WBR
} html_tag_t;

/*
    A one-one mapping from enum to string of the `html_tag_t`.
*/
static const string_t html_tag_names[] = {
    "a","abbr","address","area","article","aside","audio", "b","base","bdi","bdo","blockquote",
    "body","br","button","canvas","caption","cite","code","col","colgroup","data","datalist",
    "dd","del","details","dfn","dialog","div","dl","dt","em","fieldset","figcaption","figure",
    "footer","form","h1","h2","h3","h4","h5","h6","head","header","hr","html","i","iframe",
    "img","input","ins","kbd","label","legend","li","link","main","map","mark","menu","meta",
    "meter","nav","noscript","object","ol","optgroup","option","output","p","param","picture",
    "pre","progress","q","rp","rt","ruby","s","samp","script","section","select","small",
    "source","span","strong","style","sub","summary","sup","table","tbody","td","template",
    "textarea","tfoot","th","thead","time","title","tr","track","u","ul","var","video","wbr"
};

/*
    Contains data used to construct a dom element.
    
    Members:
     `tag` -            The tag of the dom element.
     `child_count` -    The number  of immediate children.
     `children` -       A list of immediate children.
     `props` -          A string containing the properties formated as they are in html.
     `content` -        The content inside the dom element.
     `rendered` -       The rendered html representation of the dom element.
*/
typedef struct dom_element_t dom_element_t;
struct dom_element_t {
    html_tag_t tag;
    size_t child_count;
    dom_element_t* children;
    string_t props, content, rendered;
};
#define empty_dom_element_t (dom_element_t){0}

////////////////////////////////////////////////////////
// FUNCTION DECLARATIONS                              //
////////////////////////////////////////////////////////

void*               memmem                  (const void* haystack, size_t hlen, const void* needle, size_t nlen);

int                 http_server_start       (http_server_t* server);
int                 http_connection_accept  (http_server_t* server, int worker_fd, struct epoll_event* event);
void                http_try_write_error    (int fd, int code);
http_req_state_t    http_get_request_state  (http_client_t* client);
void*               http_server_worker      (void* data);
http_response_t     http_create_response    (int status, const string_t mime, string_t data, size_t data_length);
int                 http_handle_request     (http_server_t* server, http_client_t* client);

// dom_element_t       dom_create_element      (html_tag_t tag, string_t props, string_t content, size_t child_count, ...);
// void                dom_free_element        (dom_element_t* element);
// string_t            dom_render              (dom_element_t* root);

// #define CFRAME_IMPLEMENTATION
#if defined(CFRAME_IMPLEMENTATION)

    #undef EXIT_FAILURE
    #define EXIT_FAILURE -1

    void* memmem(const void* haystack, size_t hlen, const void* needle, size_t nlen) {
        
        if (hlen < nlen) 
            return NULL;
        
        if (nlen == 0) 
            return (void*)haystack;
        
        const char* h = (const char*)haystack;
        const char* n = (const char*)needle;
        for (size_t i = 0; i <= hlen - nlen; i++)
            if (memcmp(h + i, n, nlen) == 0)
                return (void*)(h + i);
        return NULL;

    }

    int http_server_start(http_server_t* server) {

        if (server == NULL) {
            err(EINVAL, "http_server_start: server parameter is invalid");
            return EXIT_FAILURE;
        }

        if (fcntl(0, F_GETFL) < 0 && errno == EBADF) {
            err(EBADFD, "http_server_start: stdin was closed, 0 available");
            return EXIT_FAILURE; 
        }

        server->fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server->fd < 0) {
            err(errno, "http_server_start: socket");
            return EXIT_FAILURE;
        }

        int opt = 1;
        if (setsockopt(server->fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            err(errno, "http_server_start: setsockopt");
            return EXIT_FAILURE;
        }

        memset(&server->address, 0, sizeof(server->address));
        server->address.sin_family          = AF_INET;
        server->address.sin_addr.s_addr     = INADDR_ANY;
        server->address.sin_port            = htons(server->port);

        if (bind(server->fd, (struct sockaddr*)&server->address, sizeof(server->address)) < 0) {
            err(errno, "http_server_start: bind");
            close(server->fd);
            return EXIT_FAILURE;
        }

        if (listen(server->fd, SOMAXCONN) < 0) {
            err(errno, "http_server_start: listen");
            close(server->fd);
            return EXIT_FAILURE;
        }

        int flags = fcntl(server->fd, F_GETFL, 0);
        if (fcntl(server->fd, F_SETFL, flags | O_NONBLOCK) < 0) {
            err(errno, "http_server_start: fcntl");
            close(server->fd);
            return EXIT_FAILURE;
        }

        if (server->workers == 0) {
            http_server_worker(server);
            return EXIT_SUCCESS;
        }

        pthread_t* workers = (pthread_t*)calloc(server->workers, sizeof(pthread_t));
        if (workers == NULL) {
            err(errno, "http_server_start: calloc");
            close(server->fd);
            return EXIT_FAILURE;
        }
        
        for (int w = 0; w < server->workers; w++)
            pthread_create(&workers[w], NULL, &http_server_worker, (void*)server);
       
        for (int w = 0; w < server->workers; w++)
            pthread_join(workers[w], NULL);

        return EXIT_SUCCESS;

    }

    int http_connection_accept(http_server_t* server, int worker_fd, struct epoll_event* event) {
        
        if (server == NULL || event == NULL) {
            err(EINVAL, "http_connection_accept: server or event parameters are invalid");
            return EXIT_FAILURE;
        }

        struct sockaddr addr;
        socklen_t addr_len = sizeof(addr);

        http_client_t* client = (http_client_t*)calloc(1, sizeof(http_client_t));
        if (client == NULL) {
            err(errno, "http_connection_accept: calloc");
            return EXIT_FAILURE;
        }

        client->read_size = (4 * KB) + 1;
        client->fd = accept(server->fd, &addr, &addr_len);
        if (client->fd < 0) {
            if (errno == EWOULDBLOCK)
                return EXIT_SUCCESS;
            err(errno, "http_connection_accept: accept");
            return EXIT_FAILURE;
        }

        int flags = fcntl(client->fd, F_GETFL, 0);
        if (fcntl(client->fd, F_SETFL, flags | O_NONBLOCK) < 0) {
            err(errno, "http_connection_accept: fcntl");
            return EXIT_FAILURE;
        }
                            
        struct epoll_event client_event = (struct epoll_event) {
            .events = EPOLLIN | EPOLLET | EPOLLONESHOT,
            .data.ptr = client,
        };

        epoll_ctl(worker_fd, EPOLL_CTL_ADD, client->fd, &client_event);
        return EXIT_SUCCESS;

    }

    void http_try_write_error(int fd, int code) {

        const char* message = NULL;
        switch (code) {
            case 400 : message = ERROR_400; break;
            case 413 : message = ERROR_413; break;
            case 500 : message = ERROR_500; break;
            default  : message = ERROR_404; break;
        }

        write(fd, message, strlen(message));

    }

    http_req_state_t http_get_request_state(http_client_t* client) {
        
        // Either you fucked sum shit up or your connection is ass
        if (client->read_offset < 14)
            return RS_ERROR;

        int major, minor;
        if (sscanf(client->read_buffer, "%*7s %*2047s HTTP/%d.%d %*8191[^\n]", &major, &minor) != 2)
            return RS_ERROR;

        if (major == 1 && minor == 1)
            client->keep_alive = true;
            
        char* header = (char*)memmem(client->read_buffer, client->read_offset, "\r\n\r\n", 4);
        if (header == NULL)
            return RS_AGAIN;

        size_t header_len = header - client->read_buffer;
        char* connection = (char*)memmem(client->read_buffer, header_len, "close", 5);
        if (connection != NULL)
            client->keep_alive = false;

        char* temp = (char*)memmem(client->read_buffer, header_len, "Content-Length:", 15);
        if (temp == NULL)
            return RS_DONE;
        temp += 15;

        char* temp_end = (char*)memmem(temp, header_len, "\r\n", 2);
        if (temp_end == NULL)
            return RS_ERROR; // Malformed Header

        *temp_end = '\0';
        int content_length = atoi(temp);
        *temp_end = '\r';
        
        if (strlen(header + 4) == (size_t)content_length)
            return RS_DONE;

        return RS_AGAIN;

    }

    void* http_server_worker(void* data) {

        if (data == NULL) {
            err(EINVAL, "http_server_worker: data parameter is invalid");
            return NULL;
        }

        http_server_t* server = (http_server_t*)data;

        int worker_fd = epoll_create1(0);
        if (worker_fd < 0) {
            err(errno, "http_server_worker: epoll_create1");
            return NULL;
        }

        struct epoll_event* events = (struct epoll_event*)calloc(server->max_events, sizeof(struct epoll_event));
        if (events == NULL) {
            err(errno, "http_server_worker: calloc");
            goto exit;
        }

        struct epoll_event worker_event = (struct epoll_event) {
            .events = EPOLLIN | EPOLLET | EPOLLEXCLUSIVE,
            .data.fd = server->fd,
        };

        if (epoll_ctl(worker_fd, EPOLL_CTL_ADD, server->fd, &worker_event) < 0) {
            err(errno, "http_server_worker: epoll_ctl");
            goto exit;
        }
        
        while (server->running) {

            int event_count = epoll_wait(worker_fd, events, server->max_events, 0);
            if (event_count < 0) {
                if (errno == EINTR)
                    continue;
                err(errno, "http_server_worker: epoll_wait");
                goto exit;
            }

            for (int e = 0; e < event_count; e++) {

                if (events[e].data.fd == server->fd) {
                    if (http_connection_accept(server, worker_fd, &events[e]) < 0)
                        goto exit;
                }

                else if (events[e].events & EPOLLIN) {

                    http_client_t* client = (http_client_t*)events[e].data.ptr;
                    size_t remaining = client->read_size - client->read_offset;
                    if (remaining == 0) {
                        err(EFBIG, "http_server_worker: read");
                        close(client->fd);
                        free(client);
                        continue;
                    }

                    ssize_t recieved = read(client->fd, client->read_buffer + client->read_offset, remaining);
                    if (recieved < 0) {
                        if (errno == EWOULDBLOCK)
                            continue;
                        err(errno, "http_server_worker: read");
                        close(client->fd);
                        free(client);
                        continue;
                    }

                    printf("%s\n", client->read_buffer);

                    if (recieved == 0) {
                        close(client->fd);
                        free(client);
                        continue;
                    }

                    client->read_offset += recieved;
                    http_req_state_t state = http_get_request_state(client);

                    if (state == RS_AGAIN) {
                        
                        struct epoll_event ev = {
                            .events   = EPOLLIN | EPOLLET | EPOLLONESHOT,
                            .data.ptr = client,
                        };

                        if (epoll_ctl(worker_fd, EPOLL_CTL_MOD, client->fd, &ev) < 0) {
                            err(errno, "http_server_worker: epoll_ctl");
                            close(client->fd);
                            free(client);
                            continue;
                        }

                    }

                    if (state == RS_ERROR) {
                        err(EBADR, "http_server_worker: http_get_request_state");
                        close(client->fd);
                        free(client);
                        continue;
                    }

                    if (state == RS_DONE) {

                        if (http_handle_request(server, client) < 0) {
                            close(client->fd);
                            free(client);
                            continue;
                        }

                        bool client_closed = false;
                        while (client->write_offset < client->write_size) {

                            size_t remaining = client->write_size - client->write_offset;
                            ssize_t sent = write(client->fd, client->write_buffer + client->write_offset, remaining);

                            if (sent < 0) {

                                if (errno == EWOULDBLOCK) {

                                    struct epoll_event ev = {
                                        .events   = EPOLLIN | EPOLLOUT | EPOLLET,
                                        .data.ptr = client,
                                    };

                                    if (epoll_ctl(worker_fd, EPOLL_CTL_MOD, client->fd, &ev) < 0){
                                        err(errno, "http_server_worker: epoll_ctl");
                                        goto exit;
                                    }

                                    break;
                                }

                                client_closed = true;
                                close(client->fd);
                                free(client);
                                break;
                            
                            }

                            client->write_offset += sent;
                        
                        }
                
                        if (!client_closed && client->write_offset == client->write_size) {

                            memset(client->read_buffer, 0, client->read_size);
                            free(client->write_buffer);
                            
                            client->read_offset  = 0;
                            client->write_offset = 0;
                            client->write_size   = 0;
                            client->write_buffer = NULL;

                            if (!client->keep_alive) {
                                close(client->fd);
                                free(client);
                                break;
                            }

                            struct epoll_event ev = {
                                .events   = EPOLLIN | EPOLLET | EPOLLONESHOT,
                                .data.ptr = client,
                            };

                            if (epoll_ctl(worker_fd, EPOLL_CTL_MOD, client->fd, &ev) < 0) {
                                err(errno, "http_server_worker: epoll_ctl");
                                close(client->fd);
                                free(client);
                                continue;
                            }

                        }

                    }
                    
                } 
                
                else if (events[e].events & EPOLLOUT) {

                    bool client_closed = false;
                    http_client_t* client = events[e].data.ptr;

                    while (client->write_offset < client->write_size) {

                        size_t remaining = client->write_size - client->write_offset;
                        ssize_t sent = write(client->fd, client->write_buffer + client->write_offset, remaining);
                        
                        if (sent < 0) {
                            
                            if (errno == EWOULDBLOCK) 
                                break;
                                
                            close(client->fd);
                            free(client);

                            client_closed = true;
                            break;
                        }

                        client->write_offset += sent;
                    
                    }

                    if (!client_closed && client->write_offset == client->write_size) {
                         
                        memset(client->read_buffer, 0, client->read_size);
                        free(client->write_buffer);
                        
                        client->read_offset  = 0;
                        client->write_offset = 0;
                        client->write_size   = 0;
                        client->write_buffer = NULL;

                        if (!client->keep_alive) {
                            close(client->fd);
                            free(client);
                            break;
                        }

                        struct epoll_event ev = {
                            .events   = EPOLLIN | EPOLLET | EPOLLONESHOT,
                            .data.ptr = client,
                        };

                        if (epoll_ctl(worker_fd, EPOLL_CTL_MOD, client->fd, &ev) < 0) {
                            err(errno, "http_server_worker: epoll_ctl");
                            goto exit;
                        }

                    }

                } 

                else if (events[e].events & (EPOLLHUP | EPOLLERR)) {

                    if (events[e].data.fd == server->fd && events[e].data.ptr != NULL) {
                        err(EHOSTDOWN, "http_server_worker: server failure ");
                        goto exit;
                    }

                    http_client_t* client = (http_client_t*)events[e].data.ptr;
                    close(client->fd);
                    free(client);

                }
                
            }
            
        }

    exit:
        if (events != NULL) 
            free(events);
        close(worker_fd);
        return NULL;

    }
    
    http_response_t http_create_response(int status, const string_t mime, string_t data, size_t data_length) {

        return (http_response_t) {
            .content_type = mime,
            .data_length = data_length,
            .data = data,
            .status = status            
        };

    }

    int http_request_from_client(http_request_t* request, http_client_t* client) {
        
        if (client->read_offset < 14)
            return EXIT_FAILURE;

        switch (client->read_buffer[0]) {
            
            case 'D' : request->type = RT_DELETE; break;
            case 'G' : request->type = RT_GET; break;
            case 'P' : {
                request->type = (client->read_buffer[1] == 'A')
                    ? RT_PATCH
                    : (client->read_buffer[1] == 'O')
                    ? RT_POST
                    : (client->read_buffer[1] == 'U')
                    ? RT_PUT
                    : RT_INVAL;
            } break;
            
            default : request->type = RT_INVAL;
       
        }

        if (request->type == RT_INVAL)
            return EXIT_FAILURE;

        char* uri = memmem(client->read_buffer, client->read_offset, " ", 1) + 1;
        if (uri == NULL)
            return EXIT_FAILURE;

        char* uri_end = memmem(uri, client->read_offset - (uri - client->read_buffer), " ", 1);
        if (uri_end == NULL)
            return EXIT_FAILURE;

        size_t uri_length = uri_end - uri;
        request->uri = (string_t)calloc(uri_length + 1, sizeof(char));
        memcpy(request->uri, uri, uri_length);
        
        char* header_end = memmem(client->read_buffer, client->read_offset, "\r\n\r\n", 4);
        if (header_end == NULL)
            return EXIT_FAILURE;
        size_t header_len = header_end - client->read_buffer;
        
        char* temp = memmem(client->read_buffer, header_len, "Content-Length:", 15);
        if (temp == NULL)
            return EXIT_SUCCESS;
        temp += 15;

        char* temp_end = memmem(temp, header_len, "\r\n", 2);
        if (temp_end == NULL)
            return EXIT_FAILURE;

        *temp_end = '\0';
        int content_length = atoi(temp);
        *temp_end = '\r';
        
        request->data_length = content_length;
        request->data = header_end + 4;

        return EXIT_SUCCESS;

    }

    int http_handle_request(http_server_t* server, http_client_t* client) {

        #define HTTP_HEADER_MAX 256

        http_request_t request = empty_http_request_t;
        if (http_request_from_client(&request, client) < 0)
            return EXIT_FAILURE;

        http_route_t* route = &server->route_404;
        for (size_t r = 0; r < server->route_count; r++) {
            if (strcmp(request.uri, server->routes[r].uri) == 0) {
                route = &server->routes[r];
                break;
            }
        }

        if (route->handler == NULL) {
            http_try_write_error(client->fd, 404);
            return EXIT_FAILURE;
        }

        // size_t segment_count = 0;
        // string_t* data_segments = route->dynamic 
        //     ? data_from_dynamic_uri(route->uri, request.uri, &segment_count)
        //     : NULL;

        http_response_t result = route->handler(server, &request, NULL, 0);
        const string_t connection = (client->keep_alive ? "keep-alive" : "close");
        size_t write_size = HTTP_HEADER_MAX + result.data_length;

        client->write_buffer = calloc(write_size, sizeof(char));
        if (client->write_buffer == NULL) {
            err(errno, "http_handle_request: calloc");
            return EXIT_FAILURE;
        }

        if (result.status != 200) {
            http_try_write_error(client->fd, result.status);
            return EXIT_FAILURE;
        }
        
        int header_len = snprintf (
            client->write_buffer, 
            HTTP_HEADER_MAX,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %zu\r\n"
            "Connection: %s\r\n"
            "\r\n",
            (result.content_type ? result.content_type : "text/plain"),
            result.data_length,
            connection
        );

        if (header_len < 0 || (size_t)header_len >= HTTP_HEADER_MAX) {
            err(errno, "http_handle_request: snprintf");
            return EXIT_FAILURE;
        }
    
        if (result.data && result.data_length > 0)
            memcpy(client->write_buffer + header_len, result.data, result.data_length);
    
        client->write_size = header_len + result.data_length;
        client->write_offset = 0;

        return EXIT_SUCCESS;

    }

    #undef EXIT_FAILURE
    #define EXIT_FAILURE 1

#endif // CFRAME_IMPLEMENTATION
#endif // CFRAME_H