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
#define NUM_ROOM 2

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

char square[NUM_ROOM][10] = {
    { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' },
    { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' }
};

int checkwin(int);
void board(int, int);
void clearBoard(int);
int login(int);
int logout(int);
int game(int, int, int);
void menu(int);
void listUsers(int);
void lanuchMatch(int);

int room = 0;
int clientfd[FD_SETSIZE];
int clientid[FD_SETSIZE];
int rivalfd[FD_SETSIZE];

int main() {

    int on = 1;
    int sockfd, listenfd, connfd;
    int maxfd;
    int maxi;
    int nready;
    int i;
    char server_ip[16];
    char buf[MAX_LINE];
    ssize_t n;

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
        clientid[i] = -1;
        rivalfd[i] = -1;
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
            clilen = sizeof(client_addr);
			printf("\naccpet connection\n");

			if((connfd = accept(listenfd, (struct sockaddr *)&client_addr , &clilen)) < 0) {
				perror("accept() failed\n");
				exit(3);
			}
			printf("accpet a new client: %s:%d\n", inet_ntoa(client_addr.sin_addr), client_addr.sin_port);

            clientid[connfd] = login(connfd);
            clientfd[connfd] = connfd;
            i = connfd;
            menu(connfd);
            /*
			 *for(i = 0; i < FD_SETSIZE; ++i) {
			 *    if(clientfd[i] < 0) {
			 *        clientfd[i] = connfd;
			 *        break;
			 *    }
			 *}
             */

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

				printf("\nreading the socket \n");

				bzero(buf, MAX_LINE);
				if((n = read(sockfd, buf, MAX_LINE)) <= 0) {
					close(sockfd);
					FD_CLR(sockfd, &allset);
					clientfd[i] = -1;
				}
				else{
					printf("clientfd[%d] connfd: %d send message: %s\n",i ,clientfd[i], buf);
                    
                    if (strncmp(buf, "yes", 3) == 0 && (rivalfd[sockfd] != -1)) { 
                        printf("%d\n", sockfd);
                        dprintf(sockfd, "%s\n", users[clientid[rivalfd[sockfd]]].username);
                        dprintf(rivalfd[sockfd], "%s 接收你的邀請\n", users[clientid[rivalfd[sockfd]]].username);

                        users[clientid[sockfd]].isInGame = 1;
                        users[clientid[rivalfd[sockfd]]].isInGame = 1;
                    }
                    else if (strncmp(buf, "no", 2) == 0 && (rivalfd[sockfd] != -1)) {
                        dprintf(rivalfd[sockfd], "%s 拒絕你的邀請\n", users[clientid[rivalfd[sockfd]]].username);
                    }
                    else if (users[clientid[sockfd]].isInGame) {
                       dprintf(sockfd, "TESTTEST\n"); 
                       dprintf(rivalfd[sockfd], "TESTTEST\n"); 

                       if (room < NUM_ROOM) {
                           if (game(sockfd, rivalfd[sockfd], room++)) {
                               users[clientid[sockfd]].isInGame = 0;
                               users[clientid[rivalfd[sockfd]]].isInGame = 0;
                               rivalfd[sockfd] = -1;
                               rivalfd[rivalfd[sockfd]] = -1;
                               --room;
                           }
                       }
                    }
                    else {
                        switch (buf[0]) {
                            case '1':
                                listUsers(sockfd);
                                break;
                            case '2':
                                lanuchMatch(sockfd);
                                break;
                            case '3': //logout
                                logout(sockfd);
                                close(sockfd);
                                FD_CLR(sockfd, &allset);
                                clientfd[i] = -1;
                                break;
                            default:
                                menu(sockfd);
                                dprintf(sockfd, "command not found\n");
                        }
                    }
				}
				if(--nready <= 0)
					break;
			}
		}
	}

    return 0;
}

int login(int fd) {
    char tmp_account[128];
    char tmp_password[128];

    while (1) {
        dprintf(fd, "username: ");
        if (read(fd, tmp_account, sizeof(tmp_account)) > 0) {
            dprintf(fd, "password: ");
            if (read(fd, tmp_password, sizeof(tmp_password)) > 0) {
                for (int i = 0; i < USER_CNT; i++) {
                    if ((strncmp(tmp_account, users[i].username, strlen(users[i].username)) == 0) && 
                        (strncmp(tmp_password, users[i].password, strlen(users[i].password)) == 0)) {
                        if (!users[i].isLogin) {
                            users[i].isLogin = 1;
                            users[i].fd = fd;
                            printf("%s - Login Success\n", users[i].username);
                            dprintf(fd, "Login Success\n");
                            return i;
                        }
                        else
                            dprintf(fd, "Login Failed\n");
                    }
                }
            }
        }
        dprintf(fd, "Login Failed\n");
    }
}

int logout(int fd) {
    /*
     *for (int i = 0; i < USER_CNT; i++) {
     *    if (users[i].fd == fd) {
     *        users[i].isLogin = 0;
     *        users[i].fd = -1;
     *        break;
     *    }
     *}
     */
    int id = clientid[fd];
    users[id].fd = -1;
    users[id].isLogin = 0;
    printf("%s - Logout\n", users[id].username);
    return 0;
}

void menu(int fd) {
    dprintf(fd, "================================\n");
    dprintf(fd, "   1. 查看在線使用者    \n");
    dprintf(fd, "   2. 對玩家發起對戰    \n");
    dprintf(fd, "   3. 登出              \n");
    dprintf(fd, "================================\n");
}

void listUsers(int fd) {
    dprintf(fd, "================================\n");
    dprintf(fd, "   目前在線玩家列表     \n");
    dprintf(fd, "================================\n");
    int cnt = 0;
    for (int i = 0; i < 4; i++) {
        if (users[i].isLogin == 1) {
           dprintf(fd, "   [%d] %s fd:%d isInGame: %d\n", cnt++, users[i].username, users[i].fd, users[i].isInGame);
        }
    }
    dprintf(fd, "            共 %d 人在線\n", cnt);
    dprintf(fd, "================================\n");
}

void lanuchMatch(int fd) {

    int rivalFd;
    char buf[MAX_LINE];
    
    listUsers(fd);
    dprintf(fd, "   選擇你的對手 [fd]: ");
    if (read(fd, buf, MAX_LINE) > 0) {
        if (buf[0] >= '0' && buf[0] <= '9') {
            rivalFd= atoi(buf); 
        }
        else {
            dprintf(fd, "Invalid Input\n");
            return;
        }
        printf("rivalfd: %d\n", rivalfd[fd]);


        if (fd != rivalFd) {
            if (users[clientid[rivalFd]].isLogin) {
                if (!users[clientid[rivalFd]].isInGame) {
                    
                    rivalfd[rivalFd] = fd;   // A --> B
                    rivalfd[fd] = rivalFd;   // B --> A
                    
                    dprintf(fd, "%s\n", users[clientid[rivalFd]].username);
                    dprintf(rivalfd[fd], "%s 邀請你遊玩, 是否加入? [yes/no]: ", users[clientid[fd]].username);
                }
                else {
                    dprintf(fd, "   玩家正在遊玩\n");
                    menu(fd);
                    return;
                }
            }
            else {
                dprintf(fd, "   玩家不在線上\n");
                menu(fd);
                return;
            }
        }

        else {
            dprintf(fd, "   你不能與自己遊玩\n");
            menu(fd);
            return;
        }
    }
}

int game(int fd, int rivalfd, int idx) {
    int i = -1;
    int player = 1;
    int turnfd = -1;
    int choice;
    char mark;
    char buf[8];

    while (i ==  - 1) {
        board(fd, idx);
        board(rivalfd, idx);

        player = (player % 2) ? 1 : 2;

        if (player == 1)
            turnfd = fd;
        else 
            turnfd = rivalfd;

        dprintf(turnfd, "Player %d, enter a number:  ", player);
        if (read(turnfd, buf, 8) > 0) {
            choice = atoi(buf);
        }
        printf("choice: %d\n", choice);

        mark = (player == 1) ? 'X' : 'O';

        if (choice == 1 && square[idx][1] == '1')
            square[idx][1] = mark;
            
        else if (choice == 2 && square[idx][2] == '2')
            square[idx][2] = mark;
            
        else if (choice == 3 && square[idx][3] == '3')
            square[idx][3] = mark;
            
        else if (choice == 4 && square[idx][4] == '4')
            square[idx][4] = mark;
            
        else if (choice == 5 && square[idx][5] == '5')
            square[idx][5] = mark;
            
        else if (choice == 6 && square[idx][6] == '6')
            square[idx][6] = mark;
            
        else if (choice == 7 && square[idx][7] == '7')
            square[idx][7] = mark;
            
        else if (choice == 8 && square[idx][8] == '8')
            square[idx][8] = mark;
            
        else if (choice == 9 && square[idx][9] == '9')
            square[idx][9] = mark;
            
        else {
            dprintf(turnfd, "Invalid move ");

            /*player--;*/
            continue;
        }

        i = checkwin(0);
        player++;
    }
    
    board(fd, idx);
    board(rivalfd, idx);
    
    if (i == 1) {
        dprintf(fd, "==>\aPlayer %d win \n", --player);
        dprintf(rivalfd, "==>\aPlayer %d win \n", player);
    }
    else {
        dprintf(fd, "==>\aGame draw");
        dprintf(rivalfd, "==>\aGame draw");
    }

    menu(fd);
    menu(rivalfd);
    clearBoard(idx);

    return 1;
}

void clearBoard(int idx) {
    for (int i = 0; i < 10; i++)
        square[idx][i] = '0' + i; 
}

int checkwin(int i) {
    if (square[i][1] == square[i][2] && square[i][2] == square[i][3])
        return 1;
        
    else if (square[i][4] == square[i][5] && square[i][5] == square[i][6])
        return 1;
        
    else if (square[i][7] == square[i][8] && square[i][8] == square[i][9])
        return 1;
        
    else if (square[i][1] == square[i][4] && square[i][4] == square[i][7])
        return 1;
        
    else if (square[i][2] == square[i][5] && square[i][5] == square[i][8])
        return 1;
        
    else if (square[i][3] == square[i][6] && square[i][6] == square[i][9])
        return 1;
        
    else if (square[i][1] == square[i][5] && square[i][5] == square[i][9])
        return 1;
        
    else if (square[i][3] == square[i][5] && square[i][5] == square[i][7])
        return 1;
        
    else if (square[i][1] != '1' && square[i][2] != '2' && square[i][3] != '3' &&
        square[i][4] != '4' && square[i][5] != '5' && square[i][6] != '6' && square[i][7] 
        != '7' && square[i][8] != '8' && square[i][9] != '9')

        return 0;
    else
        return  - 1;
}

void board(int fd, int i) {
    dprintf(fd, "\e[1;1H\e[2J\n\n\tTic Tac Toe - Room_%d\n\n", i);

    dprintf(fd, "Player 1 (X)  -  Player 2 (O)\n\n\n");


    dprintf(fd, "     |     |     \n");
    dprintf(fd, "  %c  |  %c  |  %c \n", square[i][1], square[i][2], square[i][3]);

    dprintf(fd, "_____|_____|_____\n");
    dprintf(fd, "     |     |     \n");

    dprintf(fd, "  %c  |  %c  |  %c \n", square[i][4], square[i][5], square[i][6]);

    dprintf(fd, "_____|_____|_____\n");
    dprintf(fd, "     |     |     \n");

    dprintf(fd, "  %c  |  %c  |  %c \n", square[i][7], square[i][8], square[i][9]);

    dprintf(fd, "     |     |     \n\n");
}
