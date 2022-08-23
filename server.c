#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define PORT 49153
#define MAX_PLAYERS 20

pthread_mutex_t lock;
int player_count = 0;
int glob_pid = 1;
int glob_gid = 1;



int sendToCLient(int client_sockfd, void* msg, int len, int flags){
    if(send(client_sockfd, msg, len, 0) < 0){
        printf("Error: sendToClient Failed\n");
        return 0;
    }
    return 1;
}

int checkWinner(int** board){
    for(int i=0;i<3;i++){
        int sum = 0;
        for(int j=0;j<3;j++){
            sum += board[i][j];
        }
        if(sum == 3){
            return 1;
        }
        if(sum == 0 && board[i][0] == board[i][1]){
            return 1;
        }
    }
    for(int i=0;i<3;i++){
        int sum = 0;
        for(int j=0;j<3;j++){
            sum += board[j][i];
        }
        if(sum == 3){
            return 1;
        }
        if(sum == 0 && board[0][i] == board[1][i]){
            return 1;
        }
    }
    int s1 = board[0][0] + board[1][1] + board[2][2]; 
    int s2 = board[0][2] + board[1][1] + board[2][0]; 
    if(s1 == 3){
        return 1;
    }
    if(s1 == 0 && board[0][0] == board[1][1]){
        return 1;
    }
    if(s2 == 3){
        return 1;
    }
    if(s2 == 0 && board[0][2] == board[1][1]){
        return 1;
    }
    return 0;
}

void showBoard(int** board){
    for(int i=0;i<3;i++){
        for(int j=0;j<3;j++){
            if(board[i][j] == 0){
                printf(" O ");
            }
            else if(board[i][j] == 1){
                printf(" X ");
            }
            else {
                printf(" _ ");
            }
            if(j != 2){
                printf("|");
            }
        }
        printf("\n");
    }
}

void* playGame(void* info){
    int currP = 0;
    int nextP = 1;
    int game_over = 0;
    int moves = 0;
    int moveslist1[50];
    int moveslist2[50];
    char outstr[100];
    int totmoves =0;
    int out = 0;
    int* pair_fds = (int*) info;
    int pid1 = pair_fds[2];
    int pid2 = pair_fds[3];
    int gameid = pair_fds[4];
            int w1 =0;
        int w2 =0;
    memset(moveslist1,-1,50);
    memset(moveslist2,-1,50);
    int** board = (int**)malloc(3*sizeof(int*));
    for(int i = 0; i < 3; i++){
        board[i] = (int*)malloc(3*sizeof(int));
        for(int j = 0; j < 3; j++){
            board[i][j] = -1; 
        }
    }
    
    while(!game_over){
        int readSts ;
        int turn_over = 0;
        int timeout = 0;
        int win_stat = 0;
        int unique_move = 0;
        char temp[5];

        int s1 = send(pair_fds[currP], "TURN", 5*sizeof(char), 0);
        int s2 = send(pair_fds[nextP], "WAIT", 5*sizeof(char), 0);
        while(!turn_over){
            char response[5];
            readSts = read(pair_fds[currP], response, 4*sizeof(char));
            response[4] = '\0';
            //printf("%s\n",response);
            if(readSts <= 0){
                sendToCLient(pair_fds[nextP], "QUIT", 5*sizeof(char),0);
                out = 1;
                break;
            }
            if(strcmp(response, "TIMO") == 0){
                timeout = 1;
                game_over = 1;
                win_stat = 0;
                break;
            }
            else if(strcmp(response, "INVI") == 0){
                printf("Game %d between %d and %d ending due to invalid inputs.\n",gameid,pid1,pid2);
                sendToCLient(pair_fds[nextP], "DISC", 5*sizeof(char),0);
                sendToCLient(pair_fds[currP], "QUIT", 5*sizeof(char),0);
                out = 1;
                break;
            }
            if(s1 <= 0 || s2 <= 0 ){
                printf("Game %d between %d and %d ending due to unresponsive clients.\n",gameid,pid1,pid2);
                sendToCLient(pair_fds[nextP], "QUIT", 5*sizeof(char),0);
                out =1;
                break;
            }
            int r = (int)response[0] - '0';
            int c = (int)response[2] - '0';
            printf("%d  %d  \n" ,r,c);
            if(board[r-1][c-1] != -1){
                sendToCLient(pair_fds[currP], "INVM", 5*sizeof(char),0);
            }
            else{
                moveslist1[totmoves] = r;
                moveslist2[totmoves] = c;
                totmoves++;
                board[r-1][c-1] = currP;
                turn_over = 1;
                moves++;
                for(int o = 0; o<2;o++){
                    temp[o] = response[o];
                }
                temp[0] = response[0];
                temp[1] = response[2];
                temp[2] = '0'+currP;
                temp[3] = '\0';
            }
        }
        if(out == 1){
            break;
        }
        if(!timeout){
            for(int i=0;i<2;i++){
                sendToCLient(pair_fds[i], "UPDA", 5*sizeof(char),0);
                sendToCLient(pair_fds[i], &temp, 5*sizeof(char),0);
            }
        }
        if(!game_over){
            win_stat = checkWinner(board);
            if(win_stat == 1){
                game_over = 1;
            }
            else{
                game_over = 0;
            }
        }
        if(game_over){
            char replay_resp1[5];
            char replay_resp2[5];
            if(timeout){
                sendToCLient(pair_fds[currP], "TIMM", 5*sizeof(char),0);
                sendToCLient(pair_fds[nextP], "TIMO", 5*sizeof(char),0);
            }
            else if(win_stat){
                if(currP == 1){
                    w1++;
                }
                else{
                    w2++;
                }
                sendToCLient(pair_fds[currP], "WINN", 5*sizeof(char),0);
                sendToCLient(pair_fds[nextP], "LOSE", 5*sizeof(char),0);
            }
            moveslist1[totmoves] = 4;
            moveslist2[totmoves] = 4;
            totmoves++;
            read(pair_fds[currP], replay_resp1, 5*sizeof(char));
            read(pair_fds[nextP], replay_resp2, 5*sizeof(char));

            if(!strcmp(replay_resp1,"CONT") && !strcmp(replay_resp2,"CONT")){
                for(int i = 0; i < 3; i++){
                    for(int j = 0; j < 3; j++){
                        board[i][j] = -1; 
                    }
                }
                game_over = 0;
                moves = 0;
                sendToCLient(pair_fds[currP], "CONT", 5*sizeof(char),0);
                sendToCLient(pair_fds[nextP], "CONT", 5*sizeof(char),0);
            }
        }else if(moves == 8){
            sendToCLient(pair_fds[currP], "DRAW", 5*sizeof(char),0);
            sendToCLient(pair_fds[nextP], "DRAW", 5*sizeof(char),0);
            game_over = 1;
        }
        w1 = w1^w2;
        w2 = w1^w2;
        w1 = w1^w2;
        currP = currP^nextP;
        nextP = currP^nextP;
        currP = currP^nextP;
    }
    int coun = 1;
    printf("Game id %d over\n", gameid);
    close(pair_fds[0]);
    close(pair_fds[1]);
    FILE *fp;
    pthread_mutex_lock(&lock);
    fp = fopen("log.txt","a");
    fprintf(fp,"Game log %d between %d and %d .\n",gameid,pid1,pid2);
    fprintf(fp,"Player %d: %d wins   Player %d: %d wins .\n",pid1,w1,pid2,w2);
    for(int i = 0; i < 50;i++){
        if(moveslist1[i] != -1){
            if(moveslist1[i] == 4){
                coun++;
                fprintf(fp,"One game ended\n");
            }
            else{
                fprintf(fp,"(%d,%d) ",moveslist1[i],moveslist2[i]);
            }
        }
        else{
            fprintf(fp,"ALL games in id %d ended\n",gameid);
            break;
        }
        
    }
    fclose(fp);
    player_count -= 2;
    pthread_mutex_unlock(&lock);

    pthread_exit(NULL);
}

int main(){
    int server_sockfd;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    pthread_mutex_init(&lock, NULL);

    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_sockfd < 0){
        printf("Error: Socket creation failed\n");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if(bind(server_sockfd, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0){
        printf("Error: Bind failed\n");
        exit(1);
    }
    FILE *fp;
    fp =fopen("log.txt","w");
    fprintf(fp,"Game server up and running.\n");
    fclose(fp);
    printf("Game server started. Waiting for players.\n");
    while(1){
        int players_waiting = 0;
        int *player_pair_sockfds = (int*)malloc(4*sizeof(int));
        while(players_waiting < 2){
            listen(server_sockfd, MAX_PLAYERS - player_count);
            memset(&client_addr, 0, sizeof(client_addr));
            int len = sizeof(client_addr);
            int client_sockfd = accept(server_sockfd, (struct sockaddr*) &client_addr, &len);
            if(client_sockfd < 0){
                printf("Error : Accepting client failed");
                exit(1);
            }
            player_pair_sockfds[players_waiting] = client_sockfd;
            players_waiting++;
            sendToCLient(client_sockfd, &glob_pid, sizeof(int),0);
            player_count++;
            printf("Player %d joined.\n",glob_pid);
            glob_pid++;
        }
        
        sendToCLient(player_pair_sockfds[0], &glob_gid, sizeof(int),0);
        sendToCLient(player_pair_sockfds[1], &glob_gid, sizeof(int),0);
        player_pair_sockfds[2] = glob_pid -1;
        player_pair_sockfds[3] = glob_pid;
        player_pair_sockfds[4] = glob_gid;
        glob_gid++;
        printf("Game %d starting with Player %d and Player %d.\n",glob_gid-1,glob_pid-2,glob_pid-1);
        
        pthread_t thread_per_game;
        int temp = pthread_create(&thread_per_game, NULL, playGame, (void*)player_pair_sockfds);
    }

    close(server_sockfd);
    pthread_mutex_destroy(&lock);
    pthread_exit(NULL);
}