#define MESSAGE_TYPE_HELLO 0
#define MESSAGE_TYPE_STATUS 1
/* 
*/
struct message_head {
        uint32_t destination_nodeid;
        uint32_t source_nodeid;
	uint32_t message_type;	
	uint32_t dummy;	
};
/* 
*/
struct message_hello2 {
	struct message_head head;
	char message[32];
};
/* 
*/
struct message_status {
	struct message_head head;
	enum DarkSideStatus status;
};
/*
*/
int sendCpgMessage( cpg_handle_t , void *, size_t );
struct message_hello2 *makeMessage_hello( char * );
struct message_status *makeMessage_status( enum DarkSideStatus);

