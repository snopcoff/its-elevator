#include <stdio.h>
#include <stdlib.h>
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
#include "elevator.h"
#include "floor.h"

void sendMsgRequest();
void waitMsg(long type);

struct msgbuf buf;
int msqid;
key_t key;
FloorInfo floors[MAX_FLOORS];


int main(int argc, char *argv[]) {
  
  /*LiftSensor==================================================*/
  if(argc != 1){
    printf("Invalid parameter.\nUsage: liftsensor\n");
    exit(0);
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

  // process message
  printf("[LIFTSENSOR]\n");
  printf("Height Tầng 1\tTầng 2\tTầng 3\tTầng 4\tTầng 5\n");
  while(1){ 
    sendMsgRequest();
    waitMsg(MSG_TYPE_SENSOR_RESPONSE);
    double height = buf.command.floor;
    int current_floor = (int) ( height / FLOOR_HEIGHT) + 1;
    int i=0;
    printf("%.2f\t",height);
    for(i=0;i<MAX_FLOORS;i++){
      floors[i].floorNo = i+1;
      if(floors[i].floorNo==current_floor){
        if(height-i*FLOOR_HEIGHT>=0&&height-i*FLOOR_HEIGHT<=1)
          floors[i].state="ON";
        else
          floors[i].state="OFF";
      }else{
        floors[i].state="OFF";
      }
      printf("%s\t",floors[i].state);
    }
    printf("\n");
    msleep(SENSOR_TIME_INTERVAL);
  }

  if (msgctl(msqid, IPC_RMID, NULL) == -1) {
    perror("msgctl");             
  }

  return 1;
}//main

void sendMsgRequest(){
  setupBufMessage(&buf, MSG_TYPE_SENSOR_REQUEST, 0, 0);
  msgsnd(msqid, &buf, sizeof(struct msgbuf) - sizeof(long), 0);
}

void waitMsg(long type){
  int size = msgrcv(msqid, &buf, sizeof(struct msgbuf) - sizeof(long), type, 0);
  if (size == -1){
    perror("msgrcv");
    exit(1);
  }
}