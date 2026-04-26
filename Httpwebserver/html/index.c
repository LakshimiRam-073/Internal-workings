#include <stdio.h>


extern char **environ;
int main(void ){

    int i =0;
    printf("Content-type: text/html\n\n");

    printf("<html>");
        printf("<body>");
            printf("<h1>Hello, World</h1>");
            while(environ[i]!=NULL){
                printf("<h2>%s\n</h2>",environ[i]);
                i++;
            }
        printf("</body>");
    printf("</html>");

    
    
    
}