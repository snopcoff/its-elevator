#define CLIENT_FLOOR 0x0
#define CLIENT_ELEVATOR 0x1

#define MAX_CLIENTS 6


typedef struct clientInfo_{
	int sockfd;
	int clientType;
	double clientInfo;
} ClientInfo;