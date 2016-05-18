Elevator simulation
============================================

This repo is for ITSS project

+ Use C socket & thread
+ Default host: local host
+ With 5 floors

##Quick start:
		make
		sh script.sh

##Usage:

+ Lift Management (liftMng)

		./liftMng

		+ press quit to exit

+ Lift Controller (liftCtrl)
		
		./liftCtrl

+ Floor 1 (opePanel1)

		./opePanel1

	+ press number from 1 to 5 to call request elevator;

+ Floor 2 to 5 (opePanelX)

		./opePanelX [FloorNo]

	+ press number other than 1 to call request;

+ Lift Body (LiftBody)

		./liftBody

+ Lift Sensor (liftSensor)

		./liftsensor

##Configure
	
+ `message.h`: configure `MY_PORT`
+ `client.h`: configure `MAX_CLIENTS` (elevator + floor client)
+ `elevator.h`: configure `FLOOR_HEIGHT`, `ELEVATOR_HEIGHT`, `SENSOR_TIME_INTERVAL`, `ELEVATOR_SPEED`, `ELEVATOR_WAITING_TIME`

