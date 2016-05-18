#define MYPORT 7400

/** 
 * communication message between clients and server
 */
// response connect request: server -> client
#define CONNECT_ACCEPTED 0x01 
#define CONNECT_REJECTED 0x02

// response join request: server -> client
#define JOIN_ACCEPTED 0x03    
#define JOIN_REJECTED 0x04  

// notify elevator state: server -> floor
#define ELEVATOR_MOVING 0x05 
#define ELEVATOR_STOP 0x06   
#define ELEVATOR_APPEAR 0x07

// request join to server: floor -> server
#define REQUEST_JOIN 0x11  

 // request move to floor abc:floor -> server
#define REQUEST_FLOOR 0x12 

// request join to server: elevator -> server
#define ELEVATOR_REQUEST_JOIN 0x20

// update info about current height
#define ELEVATOR_REQUEST_UPDATE 0x21

// notify elevator move successfully
#define ELEVATOR_REQUEST_SUCCESS 0x22

// request move follow up/down direct: server -> elevator 
#define LIFT_UP 0x40
#define LIFT_DOWN 0x41
#define LIFT_STOP 0x42

// shutdown client: client -> server
#define REQUEST_SHUTDOWN 0x33
// shutdown server: server -> client
#define SHUTDOWN_SERVER 0x34

/**
 * message type
 */
// request to liftCtrl
#define MSG_TYPE_CTRL_REQUEST 0x01
// notify to liftCtrl
#define MSG_TYPE_CTRL_NOTIFY 0x02
// callback from liftCtrl
#define MSG_TYPE_CTRL_CALLBACK 0x03
// request from liftSensor
#define MSG_TYPE_SENSOR_REQUEST 0x12
// response to liftSensor
#define MSG_TYPE_SENSOR_RESPONSE 0x13

struct message{
	int type;
	double floor;
};

struct msgbuf {
	long mtype;
  struct message command;
};

void setupMessage(struct message *msg, int _type, double _floor){
	msg->type = _type;
	msg->floor = _floor;
}

void setupBufMessage(struct msgbuf *buf, long _mtype, int _type, double _floor){
	buf->mtype =  _mtype;
	buf->command.type = _type;
	buf->command.floor = _floor;
}

void setupBufMessage2(struct msgbuf *buf, long _mtype, struct message msg){
	setupBufMessage(buf, _mtype, msg.type, msg.floor);
}