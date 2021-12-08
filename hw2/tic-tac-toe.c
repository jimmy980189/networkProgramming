#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#define MAX_LINE 2048
#define USER_CNT 4

struct User {
    char username[128];
    char password[128];
    int isLogin;
    int isInGame;
    int fd;
};

struct User users[] = {
    {
        .username = "user1",
        .password = "pwd1",
        .isLogin = 0,
        .isInGame = 0,
        .fd = -1
    },
    {
        .username = "user2",
        .password = "pwd2",
        .isLogin = 0,
        .isInGame = 0,
        .fd = -1
    },
    {
        .username = "user3",
        .password = "pwd3",
        .isLogin = 0,
        .isInGame = 0,
        .fd = -1
    },
    {
        .username = "user4",
        .password = "pwd4",
        .isLogin = 0,
        .isInGame = 0,
        .fd = -1
    },
};

char square[10] = { 'o', '1', '2', '3', '4', '5', '6', '7', '8', '9' };

int checkwin();
void board();
int login(int);
int logout(int);
void menu(int);
void listUsers(int);
void lanuchMatch(int);

int main() {

    int on = 1;
    int sockfd, listenfd, connfd;
    int clientfd[FD_SETSIZE];
    int maxfd;
    int maxi;
    int nready;
    int i;
    char server_ip[16];
    char buf[MAX_LINE];
    ssize_t n;
    /*ssize_t ret;*/

    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t clilen;
    fd_set rfds, allset;

    /*signal(SIGCHLD, SIG_IGN);*/

    if ((listenfd = socket(AF_INET, SOCK_STREAM,0)) < 0) {
        perror("socket() failed");
        exit(3);
    }

    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
        perror("setsockopt() failed");
        close(listenfd);
        exit(3);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(8000);
    
    inet_ntop(AF_INET, &(server_addr.sin_addr), server_ip, 16);
    printf("Serving on %s port 8000 (http://%s:8000/) ...\n", server_ip, server_ip);  

    if (bind(listenfd, (struct sockaddr *)&server_addr,sizeof(server_addr)) < 0) {
        perror("bind() failed");
        close(listenfd);
        exit(3);
    }

    if (listen(listenfd,64) < 0) {
        perror("listen() failed");
        close(listenfd);
        exit(3);
    }
    
    maxfd = listenfd;
	maxi = -1;
	for(int i=0; i < FD_SETSIZE; i++) {
		clientfd[i] = -1;
	}
	FD_ZERO(&allset);
	FD_SET(listenfd , &allset);
    /*
     *if (ioctl(listenfd, FIONBIO, &on) < 0) {
     *    perror("ioctl() failed");
     *    close(listenfd);
     *    exit(3);
     *}
     */

    while(1) {

        rfds = allset;
        if ((nready = select(maxfd + 1, &rfds, NULL, NULL, NULL))  == -1) {
            perror("select() failed\n");
        }

		if(FD_ISSET(listenfd, &rfds)) {
			/*接收客戶端的請求*/
            printf("%d\n",nready);
            clilen = sizeof(client_addr);

			printf("\naccpet connection\n");

			if((connfd = accept(listenfd, (struct sockaddr *)&client_addr , &clilen)) < 0) {
				perror("accept() failed\n");
				exit(3);
			}

			printf("accpet a new client: %s:%d\n", inet_ntoa(client_addr.sin_addr), client_addr.sin_port);

            if(!login(connfd))
                continue;

            menu(connfd);

			/*將客戶連結套接字描述符新增到陣列*/
			for(i = 0; i < FD_SETSIZE; ++i) {
				if(clientfd[i] < 0) {
					clientfd[i] = connfd;
					break;
				}
			}

			if(FD_SETSIZE == i) {
				perror("too many connection.\n");
				exit(1);
			}

            FD_SET(connfd, &allset);
			
            if(connfd > maxfd)
				maxfd = connfd;
			if(i > maxi)
				maxi = i;

			if(--nready < 0)
				continue;
		}

		for(i = 0; i <= maxi; i++) {
			if((sockfd = clientfd[i]) < 0) //clientfd == -1 => 404
				continue;
			if(FD_ISSET(sockfd, &rfds)) {
				/*處理客戶請求*/
				printf("\nreading the socket \n");

				bzero(buf, MAX_LINE);
				if((n = read(sockfd, buf, MAX_LINE)) <= 0) {
					close(sockfd);
					FD_CLR(sockfd, &allset);
					clientfd[i] = -1;
				}
				else{
					printf("clientfd[%d] connfd: %d send message: %s\n",i ,clientfd[i], buf);
                    
                    switch (buf[0]) {
                        case '1':
                            listUsers(sockfd);
                            break;
                        case '2':
                            lanuchMatch(sockfd);
                            break;
                        case '3':
                            logout(sockfd);
                            break;
                        default:
                            menu(sockfd);
                            dprintf(sockfd, "command not found\n");

                    }
                    // echo server
                    /*
					 *if((ret = write(sockfd, buf, n)) != n) {
					 *    printf("write() failed\n");
					 *    break;
					 *}
                     */
				}
				if(--nready <= 0)
					break;
			}
		}
	}

    // local two player
/*
 *    int player = 1, i = -1, choice;
 *    char mark;
 *    while (i ==  - 1) {
 *        board();
 *        player = (player % 2) ? 1 : 2;
 *
 *        printf("Player %d, enter a number:  ", player);
 *        scanf("%d", &choice);
 *
 *        mark = (player == 1) ? 'X' : 'O';
 *
 *        if (choice == 1 && square[1] == '1')
 *            square[1] = mark;
 *            
 *        else if (choice == 2 && square[2] == '2')
 *            square[2] = mark;
 *            
 *        else if (choice == 3 && square[3] == '3')
 *            square[3] = mark;
 *            
 *        else if (choice == 4 && square[4] == '4')
 *            square[4] = mark;
 *            
 *        else if (choice == 5 && square[5] == '5')
 *            square[5] = mark;
 *            
 *        else if (choice == 6 && square[6] == '6')
 *            square[6] = mark;
 *            
 *        else if (choice == 7 && square[7] == '7')
 *            square[7] = mark;
 *            
 *        else if (choice == 8 && square[8] == '8')
 *            square[8] = mark;
 *            
 *        else if (choice == 9 && square[9] == '9')
 *            square[9] = mark;
 *            
 *        else {
 *            printf("Invalid move ");
 *
 *            player--;
 *            getchar();
 *        }
 *
 *        i = checkwin();
 *        player++;
 *    }
 *    
 *    board();
 *    
 *    if (i == 1)
 *        printf("==>\aPlayer %d win ", --player);
 *    else
 *        printf("==>\aGame draw");
 *
 *    getchar();
 */

    return 0;
}

/*********************************************
FUNCTION TO RETURN GAME STATUS
1 FOR GAME IS OVER WITH RESULT
-1 FOR GAME IS IN PROGRESS
O GAME IS OVER AND NO RESULT
 **********************************************/

int login(int fd) {
    char tmp_account[128];
    char tmp_password[128];

    while (1) {
        if (read(fd, tmp_account, sizeof(tmp_account)) > 0) {
            if (read(fd, tmp_password, sizeof(tmp_password)) > 0) {
                for (int i = 0; i < USER_CNT; i++) {
                    if ((strncmp(tmp_account, users[i].username, strlen(users[i].username)) == 0) && 
                        (strncmp(tmp_password, users[i].password, strlen(users[i].password)) == 0)) {
                        users[i].isLogin = 1;
                        users[i].fd = fd;
                        dprintf(fd, "Login Success\n");
                        return 1;
                    }
                }
            }
        }
        dprintf(fd, "Login Failed\n");
    }
}

int logout(int fd) {

    return 0;
}

void menu(int fd) {
    dprintf(fd, "========================\n");
    dprintf(fd, "   1. 查看在線使用者    \n");
    dprintf(fd, "   2. 對玩家發起對戰    \n");
    dprintf(fd, "   3. 登出              \n");
    dprintf(fd, "========================\n");
}

void listUsers(int fd) {
    dprintf(fd, "========================\n");
    dprintf(fd, "   目前在線玩家列表     \n");
    dprintf(fd, "========================\n");
    int cnt = 0;
    for (int i = 0; i < 4; i++) {
        if (users[i].isLogin == 1) {
            dprintf(fd, "   [%d] %s fd:%d\n", i, users[i].username, users[i].fd);
            ++cnt;
        }
    }
    dprintf(fd, "            共 %d 人在線\n", cnt);
    dprintf(fd, "========================\n");
}

void lanuchMatch(int fd) {

}

int checkwin() {
    if (square[1] == square[2] && square[2] == square[3])
        return 1;
        
    else if (square[4] == square[5] && square[5] == square[6])
        return 1;
        
    else if (square[7] == square[8] && square[8] == square[9])
        return 1;
        
    else if (square[1] == square[4] && square[4] == square[7])
        return 1;
        
    else if (square[2] == square[5] && square[5] == square[8])
        return 1;
        
    else if (square[3] == square[6] && square[6] == square[9])
        return 1;
        
    else if (square[1] == square[5] && square[5] == square[9])
        return 1;
        
    else if (square[3] == square[5] && square[5] == square[7])
        return 1;
        
    else if (square[1] != '1' && square[2] != '2' && square[3] != '3' &&
        square[4] != '4' && square[5] != '5' && square[6] != '6' && square[7] 
        != '7' && square[8] != '8' && square[9] != '9')

        return 0;
    else
        return  - 1;
}

void board() {
    system("clear");
    printf("\n\n\tTic Tac Toe\n\n");

    printf("Player 1 (X)  -  Player 2 (O)\n\n\n");


    printf("     |     |     \n");
    printf("  %c  |  %c  |  %c \n", square[1], square[2], square[3]);

    printf("_____|_____|_____\n");
    printf("     |     |     \n");

    printf("  %c  |  %c  |  %c \n", square[4], square[5], square[6]);

    printf("_____|_____|_____\n");
    printf("     |     |     \n");

    printf("  %c  |  %c  |  %c \n", square[7], square[8], square[9]);

    printf("     |     |     \n\n");
}
