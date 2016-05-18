#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "message.h"
#include "elevator.h"

void updateState(struct message msg);
void waitMsg(long type);
void notifyMovement();
void * liftSensorCallback();

struct msgbuf buf;
int msqid;
key_t key;

int sockfd;
ElevatorInfo elevator;

int main(int argc, char *argv[]) {
  int i=0;
  struct message msg;

  fd_set readfds, testfds, clientfds;
  struct sockaddr_in server_address;
  int addresslen = sizeof(struct sockaddr_in);
  int fd;

  FILE *f;

  /*Client variables=======================*/  
  int result;
  char hostname[]="localhost";
  struct hostent *hostinfo;
  struct sockaddr_in address;
  int clientid;

  pthread_t liftSensor_thr;

  /*Client==================================================*/

  if ((f=fopen("request_elevator","w")) != NULL){
    fclose(f);
  }

  printf("\n*** Đang đợi kết nối...\n");
  fflush(stdout);

  /* Create a socket for the client */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  /* Name the socket, as agreed with the server */
  hostinfo = gethostbyname(hostname);  /* look for host's name */
  address.sin_addr = *(struct in_addr *)*hostinfo -> h_addr_list;
  address.sin_family = AF_INET;
  address.sin_port = htons(MYPORT);

  /* Connect the socket to the server's socket */
  if(connect(sockfd, (struct sockaddr *)&address, sizeof(address)) < 0){
    perror("connecting");
    exit(1);
  }

  /* init shared memory segment */
  if ((key = ftok("request_elevator", 'B')) == -1) {
    perror("ftok");
    exit(1);
  }

  if ((msqid = msgget(key, 0644 | IPC_CREAT)) == -1) {
    perror("msgget");
    exit(1);
  }

  pthread_create(&liftSensor_thr, NULL, liftSensorCallback, NULL);

  /* init client variable */
  clientid = -1;
  elevator.height = 0;
  elevator.state = ELEVATOR_STATE_STOP;

  fflush(stdout);

  FD_ZERO(&clientfds);// init client description files
  FD_SET(sockfd, &clientfds);// add socket client description file

  /*  Now wait for messages from the server */
  while (1) {
    testfds=clientfds;
    select(FD_SETSIZE,&testfds,NULL,NULL,NULL);

    for(fd=0;fd<FD_SETSIZE;fd++){
      if(FD_ISSET(fd,&testfds)){
      	if(fd==sockfd){   /*Accept data from open socket */
      	  //read data from open socket
      	  result = read(sockfd, &msg, sizeof(struct message));

          if (result == 0) return 1;

      	  if (clientid < 0){ /* set client id before start */
      	    if (msg.type == CONNECT_ACCEPTED){
              setupMessage(&msg, ELEVATOR_REQUEST_JOIN, 0);
              write(sockfd, &msg, sizeof(struct message));
      	    } else if (msg.type == JOIN_ACCEPTED){
              clientid = 1;
              printf("Join request is accepted\n");
            } else if (msg.type == JOIN_REJECTED){
      	      perror("[ERROR] Server rejected connect for some reason\n");
              close(sockfd);
              if (msgctl(msqid, IPC_RMID, NULL) == -1) {
						    perror("msgctl");
						  }
      	      exit(0);
      	    } else {
              printf("*** Unhandled message\n");
            }
      	  } else { /* start when clientid is set */
      	    int code = msg.type;            

      	    switch(code){
      	    		case LIFT_UP:
                case LIFT_DOWN:
                case LIFT_STOP:
                  updateState(msg);
      	    			break;
        	    case SHUTDOWN_SERVER:
        	      close(sockfd); //close the current client
        	      if (msgctl(msqid, IPC_RMID, NULL) == -1) {
							    perror("msgctl");
							  }
        	      exit(0);
        	      break;
      	    }
      	  }
      	}
      }// end if
    }// end for
  }// end while
}//main

void updateState(struct message msg){  
  switch(msg.type){
    case LIFT_UP:      
      elevator.state = ELEVATOR_STATE_MOVING;
      elevator.height += ELEVATOR_SPEED;            
      printf("^ LÊN - %f\n", elevator.height);
      notifyMovement();
      break;
    case LIFT_DOWN:      
      elevator.state = ELEVATOR_STATE_MOVING;
      elevator.height -= ELEVATOR_SPEED;
      if (elevator.height < 0) elevator.height = 0;      
      notifyMovement();
      printf("v XUỐNG - %f\n", elevator.height);
      break;
    case LIFT_STOP:
      printf("- Dừng: ");
      elevator.state = ELEVATOR_STATE_STOP;
      if (msg.floor != 1){
        printf("Chờ 3s để chuyển đồ...\n");
        msleep(ELEVATOR_WAITING_TIME);
        printf("Xong! ĐÓng cửa. Thang máy đi xuống...\n");
        setupMessage(&msg, ELEVATOR_REQUEST_SUCCESS, 0);
        write(sockfd, &msg, sizeof(struct message));
      } else {
        printf("Trở về tầng 1.\n");
        msleep(ELEVATOR_WAITING_TIME);
        printf("Xong!\n");
        printf("Cửa mở.\n");
        setupMessage(&msg, ELEVATOR_REQUEST_SUCCESS, 0);
        write(sockfd, &msg, sizeof(struct message));
      }
      break;
  }
  // setupBufMessage(&buf, MSG_TYPE_BODY_REQUEST, direction, 0);
  // msgsnd(msqid, &buf, sizeof(struct msgbuf) - sizeof(long), 0);
}

void notifyMovement(){
  
  struct message msg;
  
  msleep(1000);
  setupMessage(&msg, ELEVATOR_REQUEST_UPDATE, elevator.height);
  write(sockfd, &msg, sizeof(struct message));
}

void waitMsg(long type){
  int size = msgrcv(msqid, &buf, sizeof(struct msgbuf) - sizeof(long), type, 0);
  if (size == -1){
    perror("msgrcv");
    exit(1);
  }
}

void sendMsgResponse(){
  setupBufMessage(&buf, MSG_TYPE_SENSOR_RESPONSE, 0, elevator.height);
  msgsnd(msqid, &buf, sizeof(struct msgbuf) - sizeof(long), 0);
}

void * liftSensorCallback(){
  struct message msg;
  int i;

  while (1) {
    waitMsg(MSG_TYPE_SENSOR_REQUEST);
    sendMsgResponse();
  }
}