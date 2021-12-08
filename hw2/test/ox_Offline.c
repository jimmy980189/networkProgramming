#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>

#define row 3
#define col 3

int gameBoard[row][col] = {0};
int checkBoard(void*);
void displayBoard(void*);
void menu();

int main() {

    menu();

    return 0;
}

void menu() {

    int turn = 9;
    int opRow = 0;
    int opCol = 0;
    int check = 0;

    while (turn > 0) {
        printf("  ---------------\n");
        printf("  | Tic-Tac-Toe |\n");
        printf("  ---------------\n\n");
        
        displayBoard(gameBoard);

        printf("\n  remain: %d", turn);
        printf("\n  It's %c's turn \"row,col\" : ", turn % 2 ? 'O':'X');
        scanf("%d,%d", &opRow, &opCol);

        if ( (opRow > 0 && opRow < 10) && (opCol > 0 && opCol < 10)) {
            if (gameBoard[opRow -1][opCol - 1] == 0) {
                if (turn % 2 ==  1) 
                    gameBoard[opRow - 1][opCol - 1] = 1;
                else if (turn % 2 == 0)
                    gameBoard[opRow - 1][opCol - 1] = -1;
            }
            else {
                printf("  already\n");
                continue;
            }
        }

        --turn;
        if ((turn < 5) && ((check = checkBoard(gameBoard)) == 1)) {
            break;
        }

        printf("\n");
    }
    printf("\n#####################\n");
    printf("The game has finished\n");
    printf("The winner is %c\n", check ? 'O':'X');
    
    displayBoard(gameBoard);
}

void displayBoard(void *para) {
    int (*board)[col] = para;

    printf("  -------\n");

    for (int i = 0; i < row; i++) {
        printf("  ");
        for (int j = 0; j < col; j++) {
            if (board[i][j] == 0) 
                printf("|*");
            else if (board[i][j] == 1) 
                printf("|O");
            else if (board[i][j] == -1) 
                printf("|X");
        }

        printf("|\n  -------\n");
    }
}

int checkBoard(void *para) {
    int (*board)[col] = para;

    for (int i = 0; i < row; i++) {
        for (int j = 0; j < col; j++) {
            
        }
    }
    return 0;
}
