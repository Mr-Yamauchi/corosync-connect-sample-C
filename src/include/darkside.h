/*
*/

#define GRP_DARKSIDE "darkside"

enum DarkSideStatus {
	DS_UNKNOWN = -1,
	DS_INIT = 0,
	DS_STARTUP,
	DS_STARTUP_END,
	DS_PENDING,
	DS_OPERATION,
	DS_SHUTDOWN,
};
struct DarkSideNode {
	unsigned int node_id;
	enum DarkSideStatus node_status;	
};
typedef struct DarkSideNode DarkSideNode_t;

struct DarkSideMember {
	unsigned int member_cnt;
	DarkSideNode_t member[32];	
};
typedef struct DarkSideMember DarkSideMember_t;

struct DarkSideInstance {
	cpg_handle_t c_handle;
	quorum_handle_t q_handle;
	enum DarkSideStatus ds_status;
	unsigned int master_node;
	unsigned int self_node;
		
	DarkSideMember_t ds_member;
		
};
typedef struct DarkSideInstance DarkSideInstance_t;


/*
*/
#ifdef MAIN
#define EXTERNAL /* */
#else
#define EXTERNAL extern
#endif
EXTERNAL DarkSideInstance_t *ds_instance;
