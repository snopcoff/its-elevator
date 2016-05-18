#include <unistd.h>
#include <time.h>

#define ELEVATOR_STATE_MOVING 0x01
#define ELEVATOR_STATE_STOP 0x02

#define FLOOR_HEIGHT 3
#define ELEVATOR_HEIGHT 1

#define SENSOR_TIME_INTERVAL 1000 // in miliseconds
#define ELEVATOR_SPEED 0.5 // m/s
#define ELEVATOR_WAITING_TIME 3000 // in miliseconds

typedef struct elevatorInfo_ {
	double height;
	int state; // moving/stop	
} ElevatorInfo;

int getFloor(double height){
  int floor = (int) (height / FLOOR_HEIGHT);
  if (height + ELEVATOR_HEIGHT > (floor + 1) * FLOOR_HEIGHT) return -1; // moving
  else return floor+1;
}

int msleep(long miliseconds){
	struct timespec tim;  
  tim.tv_sec = miliseconds / 1000;
  tim.tv_nsec = (miliseconds%1000) * 1000000L;
  nanosleep(&tim, NULL);
}