#include "server.h"

/* global variable */
struct msgbuf buf;
int msqid;
key_t key;

pthread_t liftCtrl_thr;

int num_clients = 0;
ClientInfo client_array[MAX_CLIENTS];
fd_set readfds, testfds, clientfds;
int server_sockfd, client_sockfd, elevator_sockfd = -1;

int main(int argc, char *argv[]) {
  int i=0;
  int result;
  struct message msg;

  struct sockaddr_in server_address;
  int addresslen = sizeof(struct sockaddr_in);
  int fd;

  FILE *f;

  /*Server==================================================*/
  if(argc != 1){
    printf("Sai tham số.\nUsage: server\n");
    exit(0);
  }

  printf("\n*** Server đang khởi động (enter \"quit\" to stop): \n");
  fflush(stdout);

  /* Create and name a socket for the server */
  server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = htonl(INADDR_ANY);
  server_address.sin_port = htons(MYPORT);
  bind(server_sockfd, (struct sockaddr *)&server_address, addresslen);

  /* Create a connection queue and initialize a file descriptor set */
  listen(server_sockfd, 1);
  FD_ZERO(&readfds);
  FD_SET(server_sockfd, &readfds);
  FD_SET(0, &readfds);  /* Add keyboard to file descriptor set */

  if ((f=fopen("request","w")) != NULL){
    fclose(f);
  }

  /* init shared memory segment */
  if ((key = ftok("request", 'B')) == -1) {
    perror("ftok");
    exit(1);
  }

  if ((msqid = msgget(key, 0644 | IPC_CREAT)) == -1) {
    perror("msgget");
    exit(1);
  }

  pthread_create(&liftCtrl_thr, NULL, liftCtrlCallback, NULL);


  /*  Now wait for clients and requests */
  while (1) {
    testfds = readfds;
    select(FD_SETSIZE, &testfds, NULL, NULL, NULL);

    /* If there is activity, find which descriptor it's on using FD_ISSET */
    for (fd = 0; fd < FD_SETSIZE; fd++) {
      if (FD_ISSET(fd, &testfds)) {
        if (fd == server_sockfd) { /* Accept a new connection request */
          client_sockfd = accept(server_sockfd, NULL, NULL);

          if (num_clients < MAX_CLIENTS) {
            // Add new clientfd to file descriptor set
            FD_SET(client_sockfd, &readfds);
            client_array[num_clients++].sockfd = client_sockfd;

            setupMessage( &msg, CONNECT_ACCEPTED, 0);
            write(client_sockfd, &msg, sizeof(struct message));
          } else {
            printf("Quá nhiều kết nối\n");
            setupMessage(&msg, CONNECT_REJECTED, 0);
            write(client_sockfd, &msg, sizeof(struct message));
            close(client_sockfd);
            }
          } else if (fd == 0)  {  /* Process keyboard activity */
          char mess[30];
          fgets(mess, 100, stdin);
          mess[strlen(mess) - 1] = '\0'; // remove \newline char
          if (strcmp(mess, "quit")==0) {
            setupMessage(&msg, SHUTDOWN_SERVER, 0);
            for (i = 0; i < num_clients ; i++) {
              write(client_array[i].sockfd, &msg, sizeof(struct message));
            }

            for (i = 0; i < num_clients ; i++) {
              close(client_array[i].sockfd);
              FD_CLR(client_array[i].sockfd, &readfds);
            }
            close(server_sockfd);
            FD_CLR(server_sockfd, &readfds);

            if (msgctl(msqid, IPC_RMID, NULL) == -1) {
              perror("msgctl");
            }

            exit(0);
          }
        } else if(fd) {  /*Process Client specific activity*/
          //read data from open socket
          result = read(fd, &msg, sizeof(struct message));

          if(result == -1) perror("read()");
          else {
            int code = msg.type;

            switch(code){
              case REQUEST_SHUTDOWN:
                printf("Client đang đóng \n");
                exitClient(fd, &readfds, client_array, &num_clients);
                break;
              /**
               * Request from floor controler
               */
              case REQUEST_JOIN:
                for (i = 0; i < num_clients; i++){
                  if (client_array[i].sockfd == fd){
                    client_array[i].clientType = CLIENT_FLOOR;
                    client_array[i].clientInfo = (int)msg.floor;                    
                    break;
                  }
                }
                setupMessage(&msg, JOIN_ACCEPTED, 0);
                write(fd, &msg, sizeof (struct message));
                break;
              case REQUEST_FLOOR:
                printf("Tầng %d được gọi\n", (int)msg.floor);
                sendMsgRequest(msg);
                break;
              /**
               * Request from elevator controler
               */
              case ELEVATOR_REQUEST_JOIN:
                for (i = 0; i < num_clients; i++){
                  if (client_array[i].sockfd == fd){
                    client_array[i].clientType = CLIENT_ELEVATOR;
                    elevator_sockfd = fd;
                    break;
                  }
                }
                setupMessage(&msg, JOIN_ACCEPTED, 0);
                write(fd, &msg, sizeof(struct message));
                break;
              case ELEVATOR_REQUEST_SUCCESS:
              case ELEVATOR_REQUEST_UPDATE:
                sendMsgNotify(msg);
                break;
              default:
                break;
            }
          }
        }
      }//if
    }//for
  }//while

  return 1;
}//main

void waitMsg(long type){
  int size = msgrcv(msqid, &buf, sizeof(struct msgbuf) - sizeof(long), type, 0);
  if (size == -1){
    perror("msgrcv");
    exit(1);
  }
}

void sendMsgRequest(struct message msg){
  // printf("sendMsgRequest\n");
  setupBufMessage2(&buf, MSG_TYPE_CTRL_REQUEST, msg);
  msgsnd(msqid, &buf, sizeof(struct msgbuf) - sizeof(long), 0);
}

void sendMsgNotify(struct message msg){
  // printf("sendMsgNotify\n");
  setupBufMessage2(&buf, MSG_TYPE_CTRL_NOTIFY, msg);
  msgsnd(msqid, &buf, sizeof(struct msgbuf) - sizeof(long), 0);
}


void * liftCtrlCallback(){
  struct message msg;
  int i;

  while (1) {
    waitMsg(MSG_TYPE_CTRL_CALLBACK);
    //printf("[LIFTCTRL_CALLBACK] msg: %d(%f)\n", buf.command.type, buf.command.floor);
    
    int code = buf.command.type;

    switch(code){
      case LIFT_UP:
      case LIFT_DOWN:
        for (i = 0; i < num_clients; i++){
          if (client_array[i].clientType == CLIENT_ELEVATOR){
            write(client_array[i].sockfd, &(buf.command), sizeof(struct message));
          } else if (client_array[i].clientType == CLIENT_FLOOR){
            msg = buf.command;
            if ((int)msg.floor == (int)client_array[i].clientInfo){
              msg.type = ELEVATOR_APPEAR;
             } else {
              msg.type = ELEVATOR_MOVING;
            }
            write(client_array[i].sockfd, &msg, sizeof(struct message));
          }
        }
        break;
      case LIFT_STOP:
        for (i = 0; i < num_clients; i++){
          if (client_array[i].clientType == CLIENT_ELEVATOR){
            write(client_array[i].sockfd, &(buf.command), sizeof(struct message));
          } else if (client_array[i].clientType == CLIENT_FLOOR){
            msg = buf.command;
           	if ((int)msg.floor == (int)client_array[i].clientInfo){
              msg.type = ELEVATOR_APPEAR;              
              write(client_array[i].sockfd, &msg, sizeof(struct message));
            }
            msg.type = ELEVATOR_STOP;            
            write(client_array[i].sockfd, &msg, sizeof(struct message));
          }
        }
        break;
      default:
        break;
    }
  }
  return NULL;
}