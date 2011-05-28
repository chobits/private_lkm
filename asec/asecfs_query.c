/*
 * for linux 2.6.36
 */
#include "asec.h"

/* used for /sys/kernel/security/asec/querd read/write/poll */
struct query_entry {
	unsigned int syncnum;	/* syncronizing with userspace communication */
	unsigned int answer;	/* user-space answer for this query */
	char *query;		/* query string for user to read */
	int len;		/* string length of query */
	struct list_head list;	/* listed in query_list */
	char query_str[0];	/* query real addr */
};

static LIST_HEAD(query_list);
static DEFINE_SPINLOCK(query_list_lock);	/* protect query_list */
static DECLARE_WAIT_QUEUE_HEAD(query_wait);
static atomic_t query_users = ATOMIC_INIT(0);

/* if error happens, return 0 for allowing the operation */
int request_for_query(struct file *f)
{
	static unsigned int syncnum = 0;
	struct query_entry *qe;				
	int len, ret, i;
	char buf[128];

	asecdbg("in");
	if (atomic_read(&query_users) == 0)
		return 0;
	len = snprintf(buf, 128, "Q syncnum=%u\n", syncnum);
	/* alloc and init query_entry */
	qe = kmalloc(sizeof(*qe) + len + 1, GFP_KERNEL);
	if (!qe)
		return 0;
	/* init query _enty */
	INIT_LIST_HEAD(&qe->list);	
	qe->syncnum = syncnum;
	qe->answer = 0;
	memcpy(qe->query_str, buf, len + 1);
	qe->query = qe->query_str;
	qe->len = len;

	/* add entry to list, then query_read/write will get it from list */
	spin_lock(&query_list_lock);
	list_add(&qe->list, &query_list);
	spin_unlock(&query_list_lock);
	
	/* wait 10 s for asking users whether to allow the operation */
	for (i = 0; atomic_read(&query_users) && i < 100; i++) {
		/* wake up the process waiting in query poll */
		wake_up(&query_wait);
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(HZ / 10);
		if (qe->answer)
			break;
	}
	
	spin_lock(&query_list_lock);
	list_del(&qe->list);
	spin_unlock(&query_list_lock);
	
	switch (qe->answer) {
	case 2:
		ret = -EPERM;
		break;
	default:
		ret = 0;
		break;
	}
	kfree(qe);
	return ret;
}

static ssize_t query_read(struct file *filp, char __user *buf,
			size_t count, loff_t *pos)
{
	struct query_entry *qe = NULL;
	int found = 0;

	spin_lock(&query_list_lock);
	list_for_each_entry(qe, &query_list, list) {
		if (!qe->answer) {
			found = 1;
			break;
		}
	}
	spin_unlock(&query_list_lock);

	if (found) {
		asecdbg("[%s][len %d]\n", qe->query, qe->len);
		if (copy_to_user(buf, qe->query, qe->len))
			return -EFAULT;
		return qe->len;
	}

	return 0;
}

#define QUERY_WRITE_MAX_COUNT 128
static ssize_t query_write(struct file *filp, const char __user *buf,
			size_t count, loff_t *pos)
{
	char qbuf[QUERY_WRITE_MAX_COUNT];
	struct query_entry *qe = NULL;
	unsigned int syncnum, answer, found = 0;

	if (count > QUERY_WRITE_MAX_COUNT)
		return -EFBIG;
	if (copy_from_user(qbuf, buf, count))
		return -EFAULT;
	if (sscanf(buf, "A syncnum=%u answer=%u", &syncnum, &answer) != 2)
		return -EINVAL;

	spin_lock(&query_list_lock);
	list_for_each_entry(qe, &query_list, list) {
		if (qe->syncnum == syncnum && !qe->answer) {
			qe->answer = answer;
			found = 1;
			break;
		}
	}
	spin_unlock(&query_list_lock);

	if (!found) {
		asecerr("not found proper query(syncnum=%u)", syncnum);
		return -EINVAL;
	}

	return count;
}

/* use default poll handing func */
static unsigned int query_poll(struct file *file, poll_table *wait)
{
	struct query_entry *qe;
	unsigned int mask = 0;
	int found = 0;

	spin_lock(&query_list_lock);
	list_for_each_entry(qe, &query_list, list) {
		if (!qe->answer) {
			found = 1;
			break;
		}
	}
	spin_unlock(&query_list_lock);

	/* if not found, we should block */
	if (!found) {
		asecdbg("not found query entry->wait\n");
		poll_wait(file, &query_wait, wait);
	} else {
		mask = POLLIN;
	}
	return mask;
}

static int query_open(struct inode *inode, struct file *file)
{
	/* Only one user can handle query */
	if (atomic_read(&query_users))
		return -EACCES;
	atomic_inc(&query_users);
	return 0;
}

static int query_close(struct inode *inode, struct file *file)
{
	if (atomic_read(&query_users))
		atomic_dec(&query_users);
	return 0;
}

const struct file_operations query_ops = {
	.open = query_open,
	.release = query_close,
	.poll = query_poll,
	.read = query_read,
	.write = query_write,
};
