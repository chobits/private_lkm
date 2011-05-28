/*
 * TODO: checking for buffer overflow
 */
#include "asec.h"
#include "iostream.h"

/*
 * alloc and init iostream
 * return 0 for success, negative value for error
 */
int init_iostream(struct file *file)
{
	struct iostream *fio;
	/* zeor alloc init most members in fio */
	fio = kzalloc(sizeof(*fio), GFP_NOFS);
	if (!fio)
		return -ENOMEM;
	if (file->f_mode & FMODE_READ)
		fio->read_buf = kzalloc(INTERNAL_BUFSIZE, GFP_NOFS);
	if (file->f_mode & FMODE_WRITE)
		fio->write_buf = kzalloc(INTERNAL_BUFSIZE, GFP_NOFS);

	file->private_data = fio;
	return 0;
}

void fini_iostream(struct file *file)
{
	struct iostream *fio;
	fio = file->private_data;
	if (!fio)
		return;
	file->private_data = NULL;
	kfree(fio->read_buf);
	kfree(fio->write_buf);
	kfree(fio);
}

void ior_set_eof(struct iostream *io)
{
	io->eof |= IO_READ_EOF;
}

int ior_eof(struct iostream *io)
{
	return (io->eof & IO_READ_EOF);
}

/*
 * return 1 when user buf is filled
 * 0 for not filled
 * negative value for error
 */
int ior_flush(struct iostream *io)
{
	int len;
	len = io->rtail - io->rhead;
	if (len == 0)
		return 0;
	if (len > io->user_read_len)
		len = io->user_read_len;
	/* Is user buf filled? */
	if (len == 0)
		return 1;
	if (copy_to_user(io->user_read_buf, io->read_buf + io->rhead, len))
		return -EFAULT;
	/* update associated member */
	io->rhead += len;
	/* restart buffer */
	if (io->rhead == io->rtail)
		io->rhead = io->rtail = 0;
	io->user_read_buf += len;
	io->user_read_len -= len;
	/* Is user buf filled? */
	if (io->user_read_len == 0)
		return 1;
	return 0;
}

void ior_string(struct iostream *io, char *p)
{
	int len;
	if (!p)
		return;
	len = strlen(p);
	/* FIXME: need robust error handling */
	if (len + io->rtail >= INTERNAL_BUFSIZE)
		return;
	strcpy(io->read_buf + io->rtail, p);
	io->rtail += len;
}

void ior_space(struct iostream *io)
{
	ior_string(io, " ");
}

void ior_newline(struct iostream *io)
{
	ior_string(io, "\n");
}

/*
 * return line lenth or error,
 * emit tail newline
 */
int iow_getline(struct iostream *io, char **linp)
{
	int c, linelen;
	linelen = 0;
	while (io->user_write_len > 0) {
		if (get_user(c, io->user_write_buf))
			return -EFAULT;
		io->user_write_buf++;
		io->user_write_len--;
		if (c == '\n') {
			/* set return value */
			linelen = io->wtail - io->whead;
			*linp = io->write_buf + io->whead;
			/* emitting tail newline and add tail terminal */
			io->write_buf[io->wtail++] = '\0';
		}
		io->write_buf[io->wtail++] = c;
	}
	return linelen;
}

void iow_sync(struct iostream *io)
{
	if (io->wtail < io->whead)
		asecerr("iostream write buf pos corrupts");
	else if (io->wtail == io->whead)
		io->wtail = io->whead = 0;
}
