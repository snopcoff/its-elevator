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
#include "client.h"
#include "message.h"
#include "floor.h"
#include "elevator.h"

void sendMsgMove(int direction, double currentFloor);
void waitMsg(long type);
int updateElevator(double height, int targetFloor);
int getFloor();

struct msgbuf buf;
int msqid;
key_t key;

ElevatorInfo elevator;

int main(int argc, char *argv[]) {

  /*LiftCtrl==================================================*/
  if(argc != 1){
    printf("Invalid parameter.\nUsage: liftctrl\n");
    exit(0);
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

  /* init elevator param */
  elevator.height = 0;
  elevator.state = ELEVATOR_STATE_STOP;

  // process message
  while(1){
    // wait for request
    waitMsg(MSG_TYPE_CTRL_REQUEST);
    printf("[LIFTCTRL]msg: %d(%f)\n", buf.command.type, buf.command.floor);
    int code = buf.command.type;
    if (code != REQUEST_FLOOR) {
      perror("invalid message request");
    }
    int floorNo = (int)buf.command.floor;
    sendMsgMove(LIFT_UP, -1);

    // wait for update elevator info
    while(1){
      waitMsg(MSG_TYPE_CTRL_NOTIFY);
      printf("[LIFTCTRL]msg: %d(%f)\n", buf.command.type, buf.command.floor);
      code = buf.command.type;
      if (code != ELEVATOR_REQUEST_UPDATE) {
        perror("invalid message update");
      }

      int check = updateElevator(buf.command.floor, floorNo);
      if (check) break;
      else sendMsgMove(LIFT_UP, getFloor(elevator.height));
    }

    sendMsgMove(LIFT_STOP, getFloor(elevator.height));

    waitMsg(MSG_TYPE_CTRL_NOTIFY);
    printf("[LIFTCTRL]msg: %d(%f)\n", buf.command.type, buf.command.floor);
    code = buf.command.type;
    if (code != ELEVATOR_REQUEST_SUCCESS) {
      perror("invalid message success");
    }
    sendMsgMove(LIFT_DOWN, getFloor(elevator.height));

    while(1){
      waitMsg(MSG_TYPE_CTRL_NOTIFY);
      printf("[LIFTCTRL]msg: %d(%f)\n", buf.command.type, buf.command.floor);
      code = buf.command.type;
      if (code != ELEVATOR_REQUEST_UPDATE) {
        perror("invalid message update");
      }

      updateElevator(buf.command.floor, 1);
      if(buf.command.floor == 0) break;
      sendMsgMove(LIFT_DOWN, getFloor(elevator.height));
    }

    sendMsgMove(LIFT_STOP, 1);

    waitMsg(MSG_TYPE_CTRL_NOTIFY);
    printf("[LIFTCTRL]msg: %d(%f)\n", buf.command.type, buf.command.floor);
    code = buf.command.type;
    if (code != ELEVATOR_REQUEST_SUCCESS) {
      perror("invalid message success");
    }
  }

  if (msgctl(msqid, IPC_RMID, NULL) == -1) {
    perror("msgctl");
  }

  return 1;
}//main

int updateElevator(double height, int targetFloor){
  // printf("height: %f\n", height);
  elevator.height = height;
  if ((targetFloor-1) * FLOOR_HEIGHT > height){
    return 0;
  } else {
    return 1;
  }
}

void waitMsg(long type){
  int size = msgrcv(msqid, &buf, sizeof(struct msgbuf) - sizeof(long), type, 0);
  if (size == -1){
    perror("msgrcv");
    exit(1);
  }
}

void sendMsgMove(int direction, double currentFloor){
  if (direction == LIFT_STOP){
    elevator.state = ELEVATOR_STATE_STOP;
  } else {
    elevator.state = ELEVATOR_STATE_MOVING;
  }

  setupBufMessage(&buf, MSG_TYPE_CTRL_CALLBACK, direction, currentFloor);  
  msgsnd(msqid, &buf, sizeof(struct msgbuf) - sizeof(long), 0);
}
