
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include<string.h>
#include <fcntl.h>

#include "server.h"



void die(char *string){
    perror(string);
    exit(1);
}

extern void print_char(char *array);


void html_template(struct response *resp,char* html_content){
    
    sprintf(
        html_content,
        "<HTML><HEAD><TITLE>%s\r\n</TITLE></HEAD>\r\n<BODY><P>%s.</P>\r\n</BODY></HTML>\r\n",
        resp->status_message,resp->status_message);
}

void html_template_with_body_parameters(struct response *resp,struct request *req,char *html_content)
{
    int offset = 0;

    offset += sprintf(html_content + offset,
        "<HTML><HEAD><TITLE>%s</TITLE></HEAD>\r\n"
        "<BODY>\r\n"
        "<h1>%s</h1>\r\n",
        resp->status_message,
        resp->status_message
    );

    if (req->body) {
        offset += sprintf(html_content + offset,
            "<h2>Raw Body</h2>\r\n"
            "<pre>%s</pre>\r\n",
            req->body
        );
    }

    offset += sprintf(html_content + offset,
        "<h2>Parameters</h2>\r\n<ul>\r\n"
    );

    for (int i = 0; req->params[i][0] != NULL; i++) {
        offset += sprintf(html_content + offset,
            "<li>%s = %s</li>\r\n",
            req->params[i][0],
            req->params[i][1]
        );
    }

    offset += sprintf(html_content + offset,
        "</ul>\r\n</BODY></HTML>\r\n"
    );
}
void send_response_file(char *fileName, int client_fd){
    int file_fd = open(fileName, O_RDONLY);
    if (file_fd == -1) {
        die("open");
    }

    char buf[1024];
    int n;

    while ((n = read(file_fd, buf, sizeof(buf))) > 0) {
        send(client_fd, buf, n, 0);
    }

    close(file_fd);
}

void send_response_header(struct response *resp, int client_fd){
    
    char header_response[1024];
    
    sprintf(header_response,"%s %d %s\r\n%sContent-Type: %s\r\n",
    resp->httptype,resp->stataus,resp->status_message,SERVER,resp->content_type);
    if (resp->content_length!=0)
    {
        char  content_len[64];
        sprintf(content_len,"Content-Length: %ld\r\n",resp->content_length);
        strcat(header_response,content_len);
    }
    strcat(header_response,"\r\n");
    int bytes_sent = send(client_fd,header_response,strlen(header_response),0);
    if (bytes_sent==-1)
    {
        die("send_response_header");
    }
    printf("Response header\n");
    print_char(header_response);
    char html_temp[256];
     html_template(resp,html_temp);
    if (resp->stataus!=200)
    {
        send(client_fd,html_temp,strlen(html_temp),0);
    }

}
void parse_params(char *acces_way, struct request *request){
    char *save_ptr1 = NULL, *save_ptr2 = NULL;

    // /page?productid=1234&utm_source=google
    request->access_way =strtok_r(acces_way,"?",&save_ptr1);
    char *url_params = strtok_r(NULL,"?",&save_ptr1);
    
    char *line ,*save_ptr3=NULL;
    if (url_params != NULL) {
        line = strtok_r(url_params, "&", &save_ptr2);
    } else {
        line = NULL;
    }

    if (line == NULL) {
        request->params[0][0] = NULL;
        request->params[0][1] = NULL;
        return; 
    }
    
    int i =0;
    while(line != NULL){
        if (line[0]=='\0')
        {
            break;
        }
        request->params[i][0] = strtok_r(line,"=",&save_ptr3); //key 
        request->params[i][1] = strtok_r(NULL,"=",&save_ptr3);// value 
        //both should be decoded + -> converted to ' ' ,then %2F -> should be converted to character. 
        i++;
        if (i>=MAX_PARAMS)
        {
            break;
        }
        line = strtok_r(NULL, "&", &save_ptr2);
        
    }
    request->params[i][0]=NULL;
    request->params[i][1]=NULL;
}
void set_request(char *bytes , struct request *req){

    char *save_ptr1, *save_ptr2,*save_ptr3;
    char *line = strtok_r(bytes,"\r\n", &save_ptr1);
    
    req->method = strtok_r(line," ",&save_ptr2);
    char *url = strtok_r(NULL, " ",&save_ptr2);
    parse_params(url, req);
    req->httptype = strtok_r(NULL, " ",&save_ptr2);
    
    int i = 0;
    while((line = strtok_r(NULL,"\r\n",&save_ptr1)) != NULL){
        
        if (line[0] == '\0')
        {
            break;
        }
        
        // take only the first
        char *headervar = strtok_r(line, " ", &save_ptr3);
        // to remove the : 
        int header_len=strlen(headervar);
        headervar[header_len-1] = '\0';
        char *headerval = strtok_r(NULL," ",&save_ptr3 );
        

        req->headers[i][0]=headervar;
        req->headers[i][1]=headerval;

        i++;
        if (i>=MAX_HEADERS) 
        {
            break;
        }
        
    }

    req->headers[i][0]=NULL;
    req->headers[i][1]=NULL;
    
}

int server(ushort port, struct sockaddr* addrInfo, socklen_t *len){
    
    

    struct sockaddr_in in_addr;

    in_addr.sin_family=AF_INET;
    in_addr.sin_port=htons(port);
    in_addr.sin_addr.s_addr= htonl(INADDR_ANY);
    addrInfo=(struct sockaddr*)&in_addr;
    socklen_t len_in= sizeof  in_addr;
    len =&len_in;
    int fileD = socket(AF_INET,SOCK_STREAM,0);
    if (fileD < 0)
    {
        die("socket");
    }
    int yes =1;
    setsockopt(fileD,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof yes);
    if (bind(fileD,(struct sockaddr *)&in_addr,len_in)== -1){
        die("bind");
    }
    if (listen(fileD,BACKLOGS) == -1){
        die("listen");
    }
    
    return fileD;
    
}