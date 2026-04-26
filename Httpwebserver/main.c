#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "server.h"
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#define BUFFER_LEN 1024
#define LINE_BUFFER_LEN 128


void print_char(char *array){

    char *c=array;

    while(*c != '\0'){
        printf("%c",*c);
        c++;
    }

    printf("\n");
}

int get_line(int child_fd, char *buf) {
    int n, total_bytes = 0;
    char c;
    int i = 0;

    while (i < LINE_BUFFER_LEN - 1) {
        n = recv(child_fd, &c, 1, 0);

        if (n == 0) {
            break;  
        } 
        else if (n < 0) {
            die("recv");
        }

        buf[i++] = c;
        total_bytes += n;

        if (c == '\n') {
            break;
        }
    }

    buf[i] = '\0';
    return total_bytes;
}

#include <string.h>
#include <bits/pthreadtypes.h>

void get_full_bytes(int client_fd, char *buf) {
    int i = 0;
    char line_buffer[LINE_BUFFER_LEN];
    size_t total_bytes = 0;
    int current_bytes;

    while (total_bytes < BUFFER_LEN - 1) {
        current_bytes = get_line(client_fd, line_buffer);

        if (current_bytes <= 0) {
            break;
        }

        if (total_bytes + current_bytes >= BUFFER_LEN) {
            die("overflow");
        }
        memcpy(buf + total_bytes, line_buffer, current_bytes);
        total_bytes += current_bytes;
        if (strcmp(line_buffer, "\r\n") == 0) {
            break;
        }

        i++;
    }

    buf[total_bytes] = '\0';
}
int isfilepresent(char *file){
    struct stat stats;
    if (stat(file, &stats) == 0)
    {
        return 1;
    }
    return 0;
    

}

void find_file_type(char *file_path, char* file_type){
    char *save_ptr;
    __strtok_r(file_path,".",&save_ptr);
    char *file_type_extracted=__strtok_r(NULL,".",&save_ptr);
    
    if(strcasecmp(file_type_extracted,"html") == 0){
        sprintf(file_type,"text/html");
    }else if(strcasecmp(file_type_extracted,"png") == 0){
        sprintf(file_type,"image/png");
    }else if(strcasecmp(file_type_extracted,"jpg") == 0 || strcasecmp(file_type_extracted,"jpeg") == 0){
        sprintf(file_type,"image/jpeg");
    }else if(strcasecmp(file_type_extracted,"gif") == 0){
        sprintf(file_type,"image/gif");
    }else{
        sprintf(file_type,"text/plain");
    }
}
void* handle_request(void *arg){
    int client_fd = (intptr_t)arg;
    char buf[BUFFER_LEN];
    int numbytes_read;
    char *file;
    get_full_bytes(client_fd,buf);
    print_char(buf);

    struct request req ;
    struct response resp;
    memset(&req,0,sizeof req);
    memset(&resp,0,sizeof resp);
    set_request(buf,&req);  
    
    if (strcasecmp(req.method,"GET") == 0)
    {
        char filepath[512];
        snprintf(filepath, sizeof(filepath), "html%s", req.access_way);
        char file_type[64];
        if (filepath[strlen(filepath)-1] == '/') 
        {
            strcat(filepath, "index.html");
            strcpy(file_type, "text/html");
        }else{
            find_file_type(req.access_way,file_type);
        }
        //check file
        printf("GET request on %s\n", filepath);
        if (isfilepresent(filepath))
        {
            
            
            resp.content_type=file_type;
            resp.httptype=req.httptype;
            resp.stataus=200;
            resp.status_message="OK";
            send_response_header(&resp,client_fd);
            send_response_file(filepath, client_fd);
            
        }else{
            resp.content_type="text/html";
            resp.httptype=req.httptype;
            resp.stataus=404;
            resp.status_message="File Missing";
            send_response_header(&resp,client_fd);
            
        }
        
        
    }else if(strcasecmp(req.method,"POST") == 0){
        char *content_length_str = NULL;
        int n=0;
        for (int i = 0; req.headers[i][0] != NULL; i++) {
            if (strcasecmp(req.headers[i][0], "Content-Length") == 0) {
                content_length_str = req.headers[i][1];
                break;
            }
        }
        
        if (content_length_str == NULL) {
            resp.content_type="text/html";
            resp.httptype=req.httptype;
            resp.stataus=411;
            resp.status_message="Length Required";
            send_response_header(&resp,client_fd);
        } else {
            size_t bodylen = atoi(content_length_str);
            if (bodylen > BUFFER_LEN - 1) 
                bodylen = BUFFER_LEN - 1;
            
            char *body = malloc(bodylen + 1);
            if (body == NULL) {
                die("malloc");
            }
            
            int total = 0;
            while(total < bodylen){
                n = recv(client_fd, body + total, bodylen - total, 0);
                if (n <= 0) {
                    break;
                }
                total += n;
            }
            body[total] = '\0';
            req.body = body;
            
            resp.content_type="text/html";
            resp.httptype=req.httptype;
            resp.stataus=200;
            resp.status_message="OK";
            send_response_header(&resp,client_fd);
            
            char html_body_response[1024];
            html_template_with_body_parameters(&resp,&req,html_body_response);
            if(send(client_fd,html_body_response,strlen(html_body_response),0) == -1){
                resp.content_type="text/html";
                resp.httptype=req.httptype;
                resp.stataus=505;
                resp.status_message="Internal error";
                send_response_header(&resp,client_fd);
            }
            
            free(body);
        }
    }else{
            resp.content_type="text/html";
            resp.httptype=req.httptype;
            resp.stataus=404;
            resp.status_message="Not found";
            send_response_header(&resp,client_fd);
    }
    
    close(client_fd);
    return NULL;
}

int main(void ){

    ushort port=3333;
    int server_fd,client_fd;
    struct sockaddr addr_info;
    socklen_t sock=0; 
    pthread_t newthread;

    server_fd = server(port,&addr_info,&sock);
  
    while(1){

        client_fd = accept(server_fd,&addr_info,&sock);
        if (client_fd==-1)
        {
            die("accept");
        }
        
        if (pthread_create(&newthread,NULL,handle_request,(void*)(intptr_t)client_fd)!=0)
        {
            die("threadcreation");
        }
        
    }

    close(server_fd);
    
}