#include <stdio.h>
#include <poll.h>

#include <qb/qbdefs.h>
#include <qb/qblog.h>
#include <qb/qbloop.h>
#include <qb/qbutil.h>
#include <qb/qbipcs.h>
#include <corosync/corotypes.h>
#include <corosync/cpg.h>
#include <corosync/quorum.h>
#include <../include/darkside.h>
#include <../include/subs.h>
#include <../include/message.h>
/*
*/
int sendCpgMessage( cpg_handle_t handle, void *m, size_t length ) {
	struct iovec msg;
	int rc;
	
	msg.iov_base = m;
	msg.iov_len = length;
printf("length = %d\n", length);
	rc = cpg_mcast_joined(handle, CPG_TYPE_AGREED, &msg, 1);

/*
	free(m);
*/
	return(rc);
}
/*
*/
struct message_hello2 *makeMessage_hello( char *ms ) {
	struct message_hello2 *mh = NULL;	
	
	mh = calloc(1, sizeof(struct message_hello2));
	
	mh->head.destination_nodeid = 100;
	mh->head.source_nodeid = ds_instance->self_node;
	mh->head.message_type = MESSAGE_TYPE_HELLO;
	strcpy(mh->message, ms);

	return mh;
}
/*
*/
struct message_status *makeMessage_status( enum DarkSideStatus status ) {
	struct message_status *mh = NULL;

	mh = calloc(1, sizeof(struct message_status));
	mh->head.destination_nodeid = 100;
	mh->head.source_nodeid = ds_instance->self_node;
	mh->head.message_type = MESSAGE_TYPE_STATUS;
	mh->status = status;

	return mh;
}
