#ifndef _MSG_H_
#define _MSG_H_

#include <mmu.h>
#include <queue.h>

// Lab 4-1 Extra
#define LOG2NMSG 5
#define NMSG (1 << LOG2NMSG)
#define MSGX(msgid) ((msgid) & (NMSG - 1))

/* define the status of Msg block */
enum {
	MSG_FREE, // free Msg
	MSG_SENT, // message has been sent but has not been received
	MSG_RECV, // message has been received
};

// Control block of a message.
struct Msg {
	TAILQ_ENTRY(Msg) msg_link;

	u_int msg_tier;
	u_int msg_status;
	u_int msg_value;
	u_int msg_from;
	u_int msg_perm;
	struct Page *msg_page;
};

TAILQ_HEAD(Msg_list, Msg);

extern struct Msg msgs[];
extern struct Msg_list msg_free_list;

void msg_init(void);
u_int msg2id(struct Msg *m);

#endif // !_MSG_H_
