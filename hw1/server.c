#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE 8096


char *get_content_type(char* path) {
    const char *last_dot = strrchr(path, '.');
    if (last_dot) {
        if (strcmp(last_dot, ".css") == 0) return "text/css";
        if (strcmp(last_dot, ".csv") == 0) return "text/csv";
        if (strcmp(last_dot, ".gif") == 0) return "image/gif";
        if (strcmp(last_dot, ".htm") == 0) return "text/html";
        if (strcmp(last_dot, ".html") == 0) return "text/html";
        if (strcmp(last_dot, ".ico") == 0) return "image/x-icon";
        if (strcmp(last_dot, ".jpeg") == 0) return "image/jpeg";
        if (strcmp(last_dot, ".jpg") == 0) return "image/jpeg";
        if (strcmp(last_dot, ".js") == 0) return "application/javascript";
        if (strcmp(last_dot, ".json") == 0) return "application/json";
        if (strcmp(last_dot, ".png") == 0) return "image/png";
        if (strcmp(last_dot, ".pdf") == 0) return "application/pdf";
        if (strcmp(last_dot, ".svg") == 0) return "image/svg+xml";
        if (strcmp(last_dot, ".txt") == 0) return "text/plain";
    }

    return "application/octet-stream";
}

void connetction(int fd)
{
    int j, file_fd, buflen, len;
    long i, ret;
    char *ptr, *qtr;
    char buffer[BUFSIZE+1];

    ret = read(fd,buffer,BUFSIZE);   
    if (ret == 0 || ret == -1) {
        perror("read() browser");
        exit(3);
    }

    if (!(strncmp(buffer,"GET ",4) && strncmp(buffer,"get ",4))) {
    
        ptr = buffer + 5;
        while(*ptr != ' ' && *ptr != '\0')
            ptr++;
        *ptr = '\0';

        /* 檔掉回上層目錄的路徑『..』 */
        /*
         *for (j=0;j<i-1;j++)
         *    if (buffer[j]=='.'&&buffer[j+1]=='.')
         *        exit(3);
         */

        if (!strncmp(buffer,"GET /\0",6) || !strncmp(buffer,"get /\0",6) )
            strcpy(buffer,"GET /index.html\0");

        buflen = strlen(buffer);
        ptr = get_content_type(buffer);

        if((file_fd=open(buffer + 5 ,O_RDONLY)) == -1) {
        const char *c404 = "HTTP/1.1 404 Not Found\r\n"
            "Connection: close\r\n"
            "Content-Length: 9\r\n\r\nNot Found";
            write(fd, c404, strlen(c404));
            printf("%s - 404\n", buffer);
            close(fd);
            close(file_fd);
            exit(3);
        }

        printf("%s - 200\n", buffer);

        sprintf(buffer,"HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", ptr);
        write(fd,buffer,strlen(buffer));

        while ((ret=read(file_fd, buffer, BUFSIZE)) > 0) {
            write(fd,buffer,ret);
        }
        exit(1);
    }
    else if (!(strncmp(buffer,"POST ",5)&&strncmp(buffer,"post ",5))) {
        ptr = strstr(buffer, "Content-Length: ");
        ptr += 16;

        int content_len = atoi(ptr);
        char filename[128];
        
        ptr = strstr(ptr, "\r\n\r\n");
        ptr += 4;
        ptr = strstr(ptr, "filename");
        ptr += 10;
        qtr = filename;
        while(*ptr != '"' && *ptr != '\0')
            *qtr++ = *ptr++;
        *qtr = '\0';
        printf("%s",filename);
        ptr = strstr(ptr, "\r\n\r\n");
        ptr += 4; // file_start

        int file_fd = open(filename, O_RDWR | O_CREAT | O_APPEND, 0644);
        write(file_fd, ptr, BUFSIZE);


        exit(3);
    }
}

int main(int argc, char **argv)
{
    int i, pid, listenfd, socketfd;
    size_t length;
    struct sockaddr_in client_addr;
    struct sockaddr_in server_addr;
    char server_ip[16];

    if ((listenfd=socket(AF_INET, SOCK_STREAM,0))<0)
        exit(3);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(8000);
    
    inet_ntop(AF_INET, &(server_addr.sin_addr), server_ip, 16);
    printf("Serving HTTP on %s port 8000 (http://%s:8000/) ...\n", server_ip, server_ip);  

    if (bind(listenfd, (struct sockaddr *)&server_addr,sizeof(server_addr)) < 0) {
        perror("bind()");
        exit(3);
    }

    if (listen(listenfd,64) < 0) {
        perror("listen()");
        exit(3);
    }

    while(1) {
        length = sizeof(client_addr);
        if ((socketfd = accept(listenfd, (struct sockaddr *)&client_addr, &length)) < 0) {
            perror("accept()");
            exit(3);
        }

        if ((pid = fork()) < 0) {
            perror("fork()");
            exit(3);
        } else {
            if (pid == 0) { // child
                close(listenfd);
                connetction(socketfd);
            } else {  //parent
                close(socketfd);
            }
        }
    }
}
