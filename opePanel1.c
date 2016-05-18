#include <stdio.h>
#include <unistd.h>
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

#include "message.h"
#include "floor.h"


int main(void) {
  int i=0;

  int server_sockfd, client_sockfd;
  struct sockaddr_in server_address;
  int addresslen = sizeof(struct sockaddr_in);
  int fd;
  fd_set readfds, testfds, clientfds;
  struct message msg;
  int fs_block_sz;
  FILE *f, *fr;

  /*Client variables=======================*/
  int sockfd;
  int result;
  char hostname[]="localhost";
  struct hostent *hostinfo;
  struct sockaddr_in address;
  int clientid;

  FloorInfo floor;

  /*Client==================================================*/
  floor.floorNo = 1;

  printf("*** Floor %d starts connecting...\n", floor.floorNo);
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

  /* init client variable */
  clientid = -1;

  fflush(stdout);

  FD_ZERO(&clientfds);// init client description files
  FD_SET(sockfd, &clientfds);// add socket client description file
  FD_SET(0, &clientfds);// add socket client description file

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
              printf("\nConnect request is accepted\n");
              setupMessage(&msg, REQUEST_JOIN, floor.floorNo);
              write(sockfd, &msg, sizeof(struct message));
            } else if (msg.type == JOIN_ACCEPTED){
              clientid = 1;
              printf("Join request is accepted (press \"0\" to exit)\n");
            } else if (msg.type == JOIN_REJECTED){
              perror("[ERROR] Server rejected connect for some reason\n");
              close(sockfd);
              exit(0);
            } else {
              printf("\n*** Unhandled message\n");
            }
          } else { /* start when clientid is set */
            int id;

            switch(msg.type){
              case ELEVATOR_MOVING:
                // printf("Door closes! Start moving...\n");
                break;
              case ELEVATOR_STOP:
                // printf("Door opens!\n");
                break;
              case ELEVATOR_APPEAR:
                printf("Elevator appears in this floor (%d)\n", (int)msg.floor);
                break;
              case SHUTDOWN_SERVER:
                close(sockfd); //close the current client
                exit(0);
                break;
            }
          }
        } else if(fd == 0){ /*process keyboard activiy*/
          int floorRequest = -1;
          fscanf(stdin, "%d", &floorRequest);
          if (floorRequest == -1){
          } else if (floorRequest == 0) {
            printf("Floor controller is shutting down\n");
            setupMessage(&msg, REQUEST_SHUTDOWN, 0);
            write(sockfd, &msg, sizeof(struct message));
            close(sockfd); //close the current client
            exit(0); //end program
          } else if (floorRequest > 5){
            printf("There are only 5 floors!\n");
          } else{
            setupMessage(&msg, REQUEST_FLOOR, floorRequest);
            write(sockfd, &msg, sizeof(struct message));
          }
        }// end if
      }// end if
    }// end for
  }// end while
}//main