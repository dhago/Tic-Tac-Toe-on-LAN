#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <fcntl.h>

#define PORT_NO 49153
#define TIMEOUT 15

void receiveFromServer(int server_sockfd, void* msg, int len){
    if(read(server_sockfd, msg, len) < 0){
        printf("Error reading message\n");
        exit(0);
    }
}
void sendToServer(int server_sockfd, char* msg, int len, int flags){
    if(send(server_sockfd, msg, len,flags) < 0){
        printf("Error: Sending message %c failed\n", msg[0]);
        exit(0);
    }
}

void showBoard(int** board){
    for(int i=0;i<3;i++){
        for(int j=0;j<3;j++){
            if(board[i][j] == 0){printf(" O ");}
            else if(board[i][j] == 1){printf(" X ");}
            else {printf(" _ ");}
            if(j != 2){printf("|");}
        }
        printf("\n");
    }
}

// Get response from user, and send it to the client
void getAndSendPos(int server_sockfd){
    char response[5];
    printf("Enter (ROW,COL) for placing your mark: ");
    time_t start = time(0);
    fgets(response,5,stdin);
    time_t end = time(0);
    printf("\n");
    if( end - start > TIMEOUT){
        sendToServer(server_sockfd, "TIMO", 4*sizeof(char), 0);
        return;
    }
    //printf("%c",response[2]);
    if((int)response[0] < '1' || (int)response[0] > '3' || (int)response[1] != ',' || (int)response[2] < '1' || (int)response[2] > '3'){
        printf("Invalid Input\n");
        sendToServer(server_sockfd, "INVI", 4*sizeof(char),0);
        return;
    }
    response[3] = '\0';
    sendToServer(server_sockfd, response, 4*sizeof(char), 0);
    return;
}
int getAndSendReply(int server_sockfd){
    char response[3];
    time_t start = time(0);
    fgets(response,3,stdin);
    char c = response[0];
    time_t end = time(0);
    printf("\n");
    if( end - start > TIMEOUT){
        sendToServer(server_sockfd, "TIMO", 4*sizeof(char), 0);
        return 0;
    }
    if(c == 'y' || c == 'Y'){
        sendToServer(server_sockfd, "CONT", 4*sizeof(char), 0);
        printf("Waiting for opponent to respond...\n");
        return 1;
    }
    else if(c == 'n' || c == 'N'){
        sendToServer(server_sockfd, "QUIT", 4*sizeof(char), 0);
        return 1;
    }
    return 0;
}
int main(){
    int pid = 0;
    int gid = 0;
    int sock;
    struct sockaddr_in sock_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0){
        printf("Error: Socket creation failed\n");
        exit(1);
    }
    int **board = (int**)malloc(3*sizeof(int*));
    for(int i=0;i<3;i++){
        board[i] = (int*)malloc(3*sizeof(int));
        for(int j=0;j<3;j++){
            board[i][j] = -1;
        }
    }
    //Connect to server socket
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    sock_addr.sin_port = htons(PORT_NO);

    
    if(connect(sock, (struct sockaddr*) &sock_addr, sizeof(sock_addr)) < 0){
        printf("Error: Connection failed\n");
        exit(1);
    }

    receiveFromServer(sock, &pid, sizeof(int));
    if(pid%2 == 1){
        printf("Connected to the game server. Your player ID is %d. Waiting for a partner to join . . .\n",pid);
    }else{
        printf("Connected to the game server. Your player ID is %d. Your partner's ID is %d. Your symbol is 'X'\n",pid, pid-1);
    }
    //printf("REACHED HERE\n");
    receiveFromServer(sock, &gid, sizeof(int));
    //printf("REACHED HERE2\n");
    
    if(pid % 2 == 1){
        printf("Your partner's ID is %d. Your symbol is 'O'.\n",pid+1);
    }
    printf("Your game ID is %d. Starting the game . . .\n",gid);

    showBoard(board);

    while(1){
        char msg[5];
        receiveFromServer(sock, msg, 5*sizeof(char));
        msg[4]='\0';
        //printf("%c",msg[0]);
        if(strcmp( msg, "SHOW")==0){
            showBoard(board);
        }
        else if(strcmp( msg, "TURN")==0){
            getAndSendPos(sock);
        }
        else if(strcmp( msg, "WAIT")==0){
            printf("Waiting for Opponent to make a move . . .\n");
        }
        else if(strcmp( msg, "DISC")==0){
            printf("Sorry, your partner disconnected.\n");
            break;
        }
        else if(strcmp( msg, "QUIT")==0){
            printf("Disconnecting . . . \n");
            break;
        }
        else if(strcmp( msg, "TIMM")==0){
            printf("You took too long to respond. Stopping . . .\n");
            printf("Would you like to restart? (y or n)\n");
            int res = getAndSendReply(sock);
            receiveFromServer(sock,msg,5*sizeof(char));
            if(strcmp( msg, "CONT")==0){
                for(int i=0;i<3;i++){
                    
                    for(int j=0;j<3;j++){
                        board[i][j] = -1;
                    }
                }
            }
            else{
                printf("Game over, quitting.\n");
                break;
            }
            
        }
        else if(strcmp( msg, "TIMO")==0){
            printf("Your opponent took too long to respond. Stopping . . .\n");
            printf("Would you like to restart? (y or n)\n");
            int res = getAndSendReply(sock);
            receiveFromServer(sock,msg,5*sizeof(char));
            if(strcmp( msg, "CONT")==0){
                for(int i=0;i<3;i++){
                    
                    for(int j=0;j<3;j++){
                        board[i][j] = -1;
                    }
                }
            }
            else{
                printf("Game over, quitting.\n");
                break;
            }
            
        }
        else if(strcmp( msg, "INVM")==0){
            printf("Invalid move, position is occupied, Try again\n");
            getAndSendPos(sock);
        }
        else if(strcmp( msg, "UPDA")==0){
            char pos[5];
            receiveFromServer(sock, pos, 5*sizeof(char));
            int i = (int)pos[0]-'1';
            int j = (int)pos[1]-'1';
            board[i][j] = (int)pos[2]-'0';
            showBoard(board);
        }
        else if(strcmp( msg, "WINN")==0){
            printf("Match ended: You WON !!!\n");
            printf("Would you like a rematch? (y or n)\n");
            int res = getAndSendReply(sock);
            receiveFromServer(sock,msg,5*sizeof(char));
            if(strcmp( msg, "CONT")==0){
                for(int i=0;i<3;i++){
                    for(int j=0;j<3;j++){
                        board[i][j] = -1;
                    }
                }
            }
            else{
                printf("Game over, quitting.\n");
                break;
            }

        }
        else if(strcmp( msg, "LOSE")==0){
            printf("Match ended: You LOST :(\n");
            printf("Would you like a rematch? (y or n)\n");
            int res = getAndSendReply(sock);
            receiveFromServer(sock,msg,5*sizeof(char));
            if(strcmp( msg, "CONT")==0){
                for(int i=0;i<3;i++){
             
                    for(int j=0;j<3;j++){
                        board[i][j] = -1;
                    }
                }
            }
            else{
                printf("Game over, quitting.\n");
                break;
            }
        }
        else if(strcmp( msg, "DRAW")==0){
            printf("Match ended: DRAW:(\n");
            printf("Would you like a rematch? (y or n)\n");
            int res = getAndSendReply(sock);
            receiveFromServer(sock,msg,5*sizeof(char));
            if(strcmp( msg, "CONT")==0){
                for(int i=0;i<3;i++){
                   
                    for(int j=0;j<3;j++){
                        board[i][j] = -1;
                    }
                }
            }
            else{
                printf("Game over, quitting.\n");
                break;
            }
        }
        else{
            printf("ERROR %c %c %c \n",msg[0],msg[1],msg[2]);
            break;
        }
    }
    close(sock);
    for(int i=0;i<3;i++){
        free(board[i]);
    }
    free(board);
    return 0;
}