#include <stdio.h>
#include <poll.h>

#define MAIN 

#include <qb/qbdefs.h>
#include <qb/qblog.h>
#include <qb/qbloop.h>
#include <qb/qbutil.h>
#include <qb/qbipcs.h>
#include <corosync/corotypes.h>
#include <corosync/cpg.h>
#include <corosync/quorum.h>
#include <./include/darkside.h>
#include <./include/subs.h>
#include <./include/message.h>

#define NOT_FOUND -1
static int search_member( unsigned int );

struct deliver_fn_context {
	qb_loop_t *qb_l;
	cpg_handle_t cpg_h;
	qb_loop_timer_handle *qb_tm;
};
static qb_loop_t *main_poll_handle = NULL;
static qb_loop_timer_handle tm_handle;
static qb_loop_timer_handle tm_handle2;
static struct cpg_name group={8, GRP_DARKSIDE};
static int have_quorum = 0;
/*
*/
static int32_t sig_exit_handler (int num, void *data) {
	return 0;
}
/*
*/
static void timer_fn_timeout ( void *context ) {
	struct message_hello2 *mh = NULL;
	struct iovec msg;
	static int i = 0;

printf("#### TIMER EXPIRE(%d)### \n", i++);
fflush(stdout);
	if ( context != NULL ) {
		struct deliver_fn_context *fc = (struct deliver_fn_context *)context;
		qb_loop_t *handle = fc->qb_l;	
		cpg_handle_t ch = fc->cpg_h;
		qb_loop_timer_handle *tm_h = fc->qb_tm;
	
		mh = makeMessage_hello("TIMER1");
		msg.iov_base = (void *)mh;	
		msg.iov_len = sizeof(struct message_hello2);	

		if ( CS_OK != cpg_mcast_joined(ch,CPG_TYPE_AGREED,&msg,1)) return;

		qb_loop_timer_add( handle, QB_LOOP_MED, 10000*QB_TIME_NS_IN_MSEC,
			context, timer_fn_timeout, tm_h);
	}
	if (mh != NULL) {
		free(mh);
	}
}
/*
*/
static void update_member_status( uint32_t nodeid, enum DarkSideStatus status ) {

	int idx = 0;
	
	if ( NOT_FOUND != (idx = search_member(nodeid))) {
printf("Update node status [%d] -> [%d]\n", ds_instance->ds_member.member[idx].node_status, status ); 
		ds_instance->ds_member.member[idx].node_status = status;
	}

}
/*
*/
static int process_message( uint32_t type, void *msg, uint32_t nodeid, size_t length ) {

	struct message_hello2 *mh;
	struct message_status *ms;

	switch ( type ) {
		case MESSAGE_TYPE_HELLO : 
				mh = (struct message_hello2 *)msg;
printf("Delivered hello msg(length = %d): %s(from %x)\n", length, mh->message, nodeid);
				break;
		case MESSAGE_TYPE_STATUS:
				ms = (struct message_status *)msg;
printf("Delivered statuc msg(length = %d): %d(from %x)\n", length, ms->status, nodeid);
				update_member_status(nodeid, ms->status);
				break;
		default:
			printf("IGNORE MESSAGE TYPE\n");
	}
}
/*
*/
static void deliver_message(
	cpg_handle_t handle, const struct cpg_name *group_name,
	uint32_t nodeid, uint32_t pid,
	void *msg, size_t msg_len ) {

	if ( search_member(nodeid) != NOT_FOUND ) {
printf("Delivered nodeid: %x msg_len: %d\n", nodeid, msg_len);
		if ( msg != NULL && msg_len > 0 ){
			struct message_head *h =  (struct message_head *)msg;
			process_message(h->message_type, msg, nodeid, msg_len);	
		}
	} else {
printf("Deviered Not Found Member ignore msg\n");
	}
}
/*
*/
static void dump_cpg_address_list( char *list_name, const struct cpg_address *p, size_t ent ) {
	int i;
	
	for ( i = 0; i < ent; i++ ) {
		printf("%s nodeid %x(%d), pid %d\n", list_name, (p + i)->nodeid, (p + i)->nodeid, (p + i)->pid);
	}
}
/*
*/
static int get_member_status( unsigned int nodeid ) {

	int idx = NOT_FOUND;
	idx = search_member(nodeid);
	if ( idx != NOT_FOUND ) {
printf("FOUND nodeid[%d]\n", nodeid);
		return ds_instance->ds_member.member[idx].node_status;
	} else {
printf("DS_UNNOON nodeid[%d]\n", nodeid);
		return DS_UNKNOWN;
	}
}
/* 
*/
static int search_member( unsigned int nodeid ) {
	int i, rc = NOT_FOUND;
	
	for ( i = 0; i < ds_instance->ds_member.member_cnt; i++ ){
		if ( ds_instance->ds_member.member[i].node_id == nodeid ) {
			rc = i;
			break;
		}
	}
	return( rc );
}
/*
*/
#define NOT_CHANGE 0
#define MEMBER_CHANGE 1
static int update_member(  const cpg_handle_t handle, const struct cpg_address *member_list, size_t member_list_entries,
				const struct cpg_address *left_list, size_t left_list_entries,
				const struct cpg_address *joined_list, size_t joined_list_entries) {
	const struct cpg_address *p;
	int i, idx, rc = NOT_CHANGE;
	int online_member_cnt = ds_instance->ds_member.member_cnt;

	/* member add (from member_list)*/
	for ( i = 0; i < member_list_entries; i++ ){
		printf("member search : %x(%d)\n", (member_list+i)->nodeid, (member_list+i)->nodeid);
		idx = search_member( (member_list + i)->nodeid );
		if ( idx == NOT_FOUND ) {
			ds_instance->ds_member.member[ds_instance->ds_member.member_cnt].node_status = DS_INIT;
			ds_instance->ds_member.member[ds_instance->ds_member.member_cnt].node_id = (member_list+i)->nodeid;
			ds_instance->ds_member.member_cnt++;	
			online_member_cnt++;
		 	rc = MEMBER_CHANGE;
			printf("MEMBER_CHANGE : member : %x(%d)\n", (member_list+i)->nodeid,
				(joined_list+i)->nodeid);
		}
	}

	/* left_member(mark left) */
	for ( i = 0; i < left_list_entries; i++ ){
		idx = search_member( (left_list + i)->nodeid );
		if ( idx != NOT_FOUND ) {
			ds_instance->ds_member.member[idx].node_status = DS_UNKNOWN;
			rc = MEMBER_CHANGE;
			online_member_cnt--;
			printf("MEMBER_CHANGE : member left : %x(%d)\n", (left_list+i)->nodeid, (left_list+i)->nodeid);
		} else {
			printf("NOT MEMBER_CHANGE : member left : %x(%d)\n", (left_list+i)->nodeid, (left_list+i)->nodeid);
		}
	}
	/* joined_member(display info) */
	for ( i = 0; i < joined_list_entries; i++ ){
		printf("joined search : %x(%d)\n", (joined_list+i)->nodeid, (joined_list+i)->nodeid);
		idx = search_member( (joined_list + i)->nodeid );
		if ( idx == NOT_FOUND ) {
			printf("MEMBER_CHANGE : member joined : %x(%d)\n", (joined_list+i)->nodeid,
				(joined_list+i)->nodeid);
		}
		if ( ds_instance->self_node == (joined_list+i)->nodeid ) {
			start_child(handle);
		}
	}
	printf("TOTAL ONLINE MEMBER CNT : %d\n", online_member_cnt);
	return(rc);
}
/*
*/
static void confch_notify(
	cpg_handle_t handle,
	const struct cpg_name *group_name,
	const struct cpg_address *member_list, size_t member_list_entries,
	const struct cpg_address *left_list, size_t left_list_entries,
	const struct cpg_address *joined_list, size_t joined_list_entries) {

	struct message_hello2 *mh = NULL;
	int idx = NOT_FOUND;

	dump_cpg_address_list("member_list", member_list, member_list_entries);	
	dump_cpg_address_list("left_list", left_list, left_list_entries);	
	dump_cpg_address_list("joined_list", joined_list, joined_list_entries);	

	if ( MEMBER_CHANGE == (update_member( handle, member_list, member_list_entries, left_list, left_list_entries,
				joined_list, joined_list_entries)) ) {
		/* send Hello */
		mh = makeMessage_hello("HELLO");
		sendCpgMessage(handle, (void *)mh, sizeof(struct message_hello2)); 
		free(mh);
	}
	/* set Master */
printf("NOW MASTER : %x\n", ds_instance->master_node);
	if ( ds_instance->master_node == 0 ) {
printf("SET MASTER1\n");
fflush(stdout);
		ds_instance->master_node = member_list_entries > 0 ? (member_list + 0)->nodeid : 0;
		printf(" master_node(1) : %x(%d)\n", ds_instance->master_node, ds_instance->master_node);
	} else if ( DS_UNKNOWN == get_member_status(ds_instance->master_node) ) {
printf("SET MASTER2\n");
fflush(stdout);
		ds_instance->master_node = member_list_entries > 0 ? (member_list + 0)->nodeid : 0;
		printf(" master_node(2) : %x(%d)\n", ds_instance->master_node, ds_instance->master_node);
	} else {
		;
	}

	
}
/*
*/
static int distpatch_fn( int fd, int events, void *context ) {

	if ( context != NULL ) {
		cpg_handle_t handle = (cpg_handle_t)context;	
		cpg_dispatch(handle, CS_DISPATCH_ONE);
	}
}
/*
*/
static cpg_callbacks_t c_cb = {
	&deliver_message,
	&confch_notify,
};
static int connect_cpg( cpg_handle_t *h, unsigned int *nodeid ) {
	int rc ;

	if ( CS_OK != (rc = cpg_initialize(h, &c_cb))) goto exit_label;	
	if ( CS_OK != (rc = cpg_local_get(*h,nodeid))) goto exit_label;
	rc = cpg_join(*h, &group);
exit_label:
	return(rc);
}
/*
*/
static void
quorum_notification(quorum_handle_t handle,
                         uint32_t quorate,
                         uint64_t ring_id, uint32_t view_list_entries, uint32_t * view_list) {

	if ( quorate != have_quorum ) {
		printf(" QUORATE : %s \n", quorate ? "acquired" : "lost");
		have_quorum = quorate;
	} else {
		printf(" QUORATE : %s \n", quorate ? "retained" : "still lost");
	}
	
}
/*
*/
quorum_callbacks_t q_cb = {
    .quorum_notify_fn = quorum_notification
};
static int connect_quorum( quorum_handle_t *h ) {
	int rc;
	uint32_t type = 0;
	int quorate = 0;

	
	if ( CS_OK != (rc = quorum_initialize(h, &q_cb, &type))) goto exit_label;

	if ( type != QUORUM_SET ) goto exit_label;
	
	if ( CS_OK != (rc = quorum_getquorate(*h, &quorate))) goto exit_label;
	have_quorum = quorate;
	printf("Quorum %s\n", quorate ? "acquired" : "lost");
 
exit_label:
	if ( rc != CS_OK) {
		printf("ERROR");
	}
	return(rc);
}
/*
*/
static int initialize( unsigned int *nodeid ) {

	int rc;
	struct message_status *mh = NULL;

	ds_instance = calloc(1, sizeof(DarkSideInstance_t));
	ds_instance->ds_status = DS_INIT;


	if ( CS_OK != (rc = connect_cpg(&ds_instance->c_handle, nodeid))) {
		goto exit_label;
	}
	
	if ( CS_OK != (rc = connect_quorum(&ds_instance->q_handle))) { 
		goto exit_label;
	}

	ds_instance->ds_status = DS_STARTUP;
	ds_instance->self_node = *nodeid;
	
	/* send status */
	mh = makeMessage_status(ds_instance->ds_status);
	sendCpgMessage(ds_instance->c_handle, (void *)mh, sizeof(struct message_status)); 
	free(mh);

exit_label:
	printf("initialize() rc : %d nodeid : %x(%d)\n", rc, ds_instance->self_node, ds_instance->self_node);
	return(rc);
}
/*
*/
void finalize(){
	if (ds_instance->c_handle != 0) {
		cpg_leave(ds_instance->c_handle, &group); 
		cpg_finalize(ds_instance->c_handle);
		ds_instance->c_handle = 0;
	}
	if (ds_instance->q_handle != 0) {
		quorum_finalize(ds_instance->q_handle);
		ds_instance->q_handle = 0;
	}
}
/*
*/
int start_child( const cpg_handle_t handle ){
	int rc = CS_OK;
	struct message_status *mh = NULL;

	ds_instance->ds_status = DS_STARTUP_END;

	/* send status */
	mh = makeMessage_status(ds_instance->ds_status);
	sendCpgMessage(handle, (void *)mh, sizeof(struct message_status)); 
	free(mh);

exit_label:
	return(rc);
}
/*
*/
int wait_child_up(){
	int rc = CS_OK;

	ds_instance->ds_status = DS_OPERATION;
exit_label:
	return(rc);
}
/*
*/
int main( int argc, char** argv, char** env ) {
	unsigned int nodeid = 0;
	int fd;
	struct deliver_fn_context cn;
	struct deliver_fn_context cn2;

	ds_instance = NULL;
	initialize(&nodeid);	

	cpg_fd_get (ds_instance->c_handle, &fd);
	
	main_poll_handle = qb_loop_create();

//	print_test();

	qb_loop_signal_add(main_poll_handle, QB_LOOP_HIGH,
		SIGTERM, NULL, sig_exit_handler, NULL);

	qb_loop_poll_add(main_poll_handle, QB_LOOP_MED,
		fd, POLLIN, (void *)ds_instance->c_handle, distpatch_fn);

	cn.qb_l = main_poll_handle;
	cn.cpg_h = ds_instance->c_handle;
	cn.qb_tm = &tm_handle;

	cn2.qb_l = main_poll_handle;
	cn2.cpg_h = ds_instance->c_handle;
	cn2.qb_tm = &tm_handle2;

	qb_loop_timer_add(main_poll_handle, QB_LOOP_MED,  10000*QB_TIME_NS_IN_MSEC,
		(void *)&cn, timer_fn_timeout, &tm_handle);
	
	qb_loop_run(main_poll_handle);
	
	qb_loop_destroy(main_poll_handle);

	finalize();
	
	printf("test OK\n");
}
