#ifndef HARISH_HTTP_SERVER
#define HARISH_HTTP_SERVER

typedef __u_short ushort;
#define BACKLOGS 20
#define MAX_HEADERS 32
#define MAX_PARAMS 10
#define SERVER "Server: HarishServer/1.0\r\n"
struct request{

    char *method;
    char *access_way;
    char *httptype;
    char *headers[MAX_HEADERS][2];
    char *body;
    char *params[MAX_PARAMS][2];
};

struct response{
    char *httptype;
    ushort stataus;
    char *status_message;
    char *content_type;
    u_int64_t content_length;

};

int server(ushort port,struct sockaddr* addrInfo, socklen_t *len );

void set_request(char* bytes,struct request *req);
void html_template_with_body_parameters(struct response *resp,struct request *req,char *html_content); 
void send_response_header(struct response *resp, int client_fd);
void send_response_file(char *file,int client_fd);

void die(char *string);

#endif