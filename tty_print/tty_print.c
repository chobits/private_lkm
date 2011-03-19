#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/init.h>
#include <linux/sched.h> /*for current*/
#include <linux/tty.h>
static int tty_print(char *str)
{
	struct tty_struct *tty_p = current->signal->tty;

	if(tty_p == NULL)
		return -1;

/* also ok!!
 * tty_p->ops->write(tty_p, str, strlen(str));
 * tty_p->ops->write(tty_p, "\015\012", 2);
 */

/* 
 * my_tty->driver is a struct which holds the tty's functions,
 * one of which (write) is used to write strings to the tty. 
 * It can be used to take a string either from the user's or 
 * kernel's memory segment.
 *
 * The function's 1st parameter is the tty to write to,
 * because the same function would normally be used for all 
 * tty's of a certain type. 
 * The 2nd parameter is a pointer to a string.
 * The 3rd parameter is the length of the string.
 */
	tty_p->driver->ops->write(tty_p, str, strlen(str));
	
/* 
 * ttys were originally hardware devices, which (usually) 
 * strictly followed the ASCII standard.  In ASCII, to move to
 * a new line you need two characters, a carriage return and a
 * line feed.  On Unix, the ASCII line feed is used for both 
 * purposes - so we can't just use \n, because it wouldn't have
 * a carriage return and the next line will start at the
 * column right after the line feed.
 *
 * This is why text files are different between Unix and 
 * MS Windows.  In CP/M and derivatives, like MS-DOS and 
 * MS Windows, the ASCII standard was strictly adhered to,
 * and therefore a newline requirs both a LF and a CR.
 */
	tty_p->driver->ops->write(tty_p, "\015\012", 2);
}

static int __init tp_init(void)
{
	if (tty_print("hello init") < 0)
		return -1;
	return 0;
}

static void __exit tp_exit(void)
{
	tty_print("farewell exit");
}

MODULE_AUTHOR("Wang Bo");
MODULE_LICENSE("GPL");
module_init(tp_init);
module_exit(tp_exit);
