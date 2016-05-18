#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
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
#include "client.h"

void exitClient(int fd, fd_set *readfds, ClientInfo client_array[], int *num_clients){
  int i;
  
  close(fd);
  FD_CLR(fd, readfds); //clear the leaving client from the set
  
  // remove client info from client_array
  for (i = 0; i < (*num_clients) - 1; i++)
  if (client_array[i].sockfd == fd)
  break;
  for (; i < (*num_clients) - 1; i++){
    client_array[i] = client_array[i + 1];
  }
  (*num_clients)--;
}

void waitMsg(long type);

void sendMsgNotify(struct message msg);
void sendMsgRequest(struct message msg);
void *liftCtrlCallback();