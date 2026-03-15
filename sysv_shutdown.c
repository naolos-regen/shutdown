#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/param.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>
#include <utmp.h>
#include <syslog.h>
#include <sys/reboot.h>

#include <features.h>
#include <sys/utsname.h>

#ifdef RB_ENABLE_CAD
#  define BMAGIC_HARD		RB_ENABLE_CAD
#endif

#ifdef RB_DISABLE_CAD
#  define BMAGIC_SOFT		RB_DISABLE_CAD
#endif

#ifdef RB_HALT_SYSTEM
#  define BMAGIC_HALT		RB_HALT_SYSTEM
#else
#  define BMAGIC_HALT		RB_HALT
#endif

#define BMAGIC_REBOOT		RB_AUTOBOOT

#ifdef RB_POWER_OFF
#  define BMAGIC_POWEROFF	RB_POWER_OFF
#elif defined(RB_POWEROFF)
#  define BMAGIC_POWEROFF	RB_POWEROFF
#else
#  define BMAGIC_POWEROFF	BMAGIC_HALT
#endif

/* #define init_reboot(magic)	reboot(magic) */
/* add by limingth */
//#undef init_reboot(magic)
// init_reboot.cmt
#define init_reboot(magic)      printf("init_reboot: %d\n", magic)
#define PWRSTAT		"/var/run/powerstatus"	

void 
write_wtmp (char *user, char *id, int pid, int type, char *line)		
{
	int fd;
	struct utmp utmp;
	struct utsname uname_buf;
	struct timeval tv;
	
	/*
	 *	Can't do much if WTMP_FILE is not present or not writable.
	 */
	if (access(WTMP_FILE, W_OK) < 0)
		return;

	/*
	 *	Try to open the wtmp file. Note that we even try
	 *	this if we have updwtmp() so we can see if the
	 *	wtmp file is accessible.
	 */
	if ((fd = open(WTMP_FILE, O_WRONLY|O_APPEND)) < 0) return;

#ifdef INIT_MAIN
	/*
	 *	Note if we are going to write a boot record.
	 */
	if (type == BOOT_TIME) wrote_wtmp_reboot++;

	/*
	 *	See if we need to write a reboot record. The reason that
	 *	we are being so paranoid is that when we first tried to
	 *	write the reboot record, /var was possibly not mounted
	 *	yet. As soon as we can open WTMP we write a delayed boot record.
	 */
	if (wrote_wtmp_reboot == 0 && type != BOOT_TIME)
  		write_wtmp("reboot", "~~", 0, BOOT_TIME, "~");

	/*
	 *	Note if we are going to write a runlevel record.
	 */
	if (type == RUN_LVL) wrote_wtmp_rlevel++;

	/*
	 *	See if we need to write a runlevel record. The reason that
	 *	we are being so paranoid is that when we first tried to
	 *	write the reboot record, /var was possibly not mounted
	 *	yet. As soon as we can open WTMP we write a delayed runlevel record.
	 */
	if (wrote_wtmp_rlevel == 0 && type != RUN_LVL) {
		int runlevel = thislevel;
		int oldlevel = prevlevel;
		write_wtmp("runlevel", "~~", runlevel + 256 * oldlevel, RUN_LVL, "~");
	}
#endif

	/*
	 *	Zero the fields and enter new fields.
	 */
	memset(&utmp, 0, sizeof(utmp));
#if defined(__GLIBC__)
	gettimeofday(&tv, NULL);
	utmp.ut_tv.tv_sec = tv.tv_sec;
	utmp.ut_tv.tv_usec = tv.tv_usec;
#else
	time(&utmp.ut_time);
#endif
	utmp.ut_pid  = pid;
	utmp.ut_type = type;
	strncpy(utmp.ut_name, user, sizeof(utmp.ut_name));
	strncpy(utmp.ut_id  , id  , sizeof(utmp.ut_id  ));
	strncpy(utmp.ut_line, line, sizeof(utmp.ut_line));
        
        /* Put the OS version in place of the hostname */
        if (uname(&uname_buf) == 0)
		strncpy(utmp.ut_host, uname_buf.release, sizeof(utmp.ut_host));

#if HAVE_UPDWTMP
	updwtmp(WTMP_FILE, &utmp);
#else
	write(fd, (char *)&utmp, sizeof(utmp));
#endif
	close(fd);
}

static char *clean_env[] = {
	"HOME=/",
	"PATH=/bin:/usr/bin:/sbin:/usr/sbin",
	"TERM=dumb",
	"SHELL=/bin/sh",
	NULL,
};						

#define SPAWN(noerr, prog, ...)							\
	({									\
		pid_t pid, rc;							\
		int i = 0, status;						\
		int _result = 0;						\
		while ((pid = fork()) < 0 && i < 10) {				\
			perror("fork");						\
			sleep(5);						\
			i++;							\
		}								\
		if (pid < 0) {							\
			_result = -1;						\
		} else if (pid > 0) {						\
			while ((rc = wait(&status)) != pid)			\
				if (rc < 0 && errno == ECHILD) break;		\
			_result = (rc == pid) ? WEXITSTATUS(status) : -1;	\
		} else {							\
			if (noerr) fclose(stderr);				\
			char *argv[] = {(prog), __VA_ARGS__, NULL};		\
			chdir("/");						\
			__environ = clean_env;					\
			execvp(argv[0], argv);					\
			perror(argv[0]);					\
			exit(1);						\
			_result = 0;						\
		}								\
		_result;							\
	})


void sysv_shutdown()
{
	int i;

	for (i = 0; i < 3; i++)
		if (!isatty(i)) {
			close(i);
			open("/dev/null", O_RDWR);
		}
	for (i = 3; i < 20; i++) close(i);
	close(255);

	if (kill(1, SIGTSTP) < 0)
	{
		fprintf(stderr, "shutdown: can't handle idle init: %s.\r\n", strerror(errno));
		exit(1);
	}

	fprintf(stderr, "shutdown: sending all processes the TERM signal ...\r\n");
	(void) kill(-1, SIGTERM);
	fprintf(stderr, "shutdown: sending all processes to KILL signal.\r\n");
	(void) kill(-1, SIGKILL);
	sleep(1);

	write_wtmp("shutdown", "~~", 0, RUN_LVL, "~~");
#if defined(ACTION_OFF)
# if	(ACTION_OFF > 1) && (_BSD_SOURCE || (_XOPEN_SOURCE && _XOPEN_SOURCE < 500))
	if (acct(NULL))
		fprintf(stderr, "shutdown: can not stop process accounting: %s.\r\n", strerror(errno));
# elif (ACTION_OFF > 0)
	(void) SPAWN(1, "accton", "off", NULL);
# else
	(void) SPAWN(1, "accton", NULL);
# endif
#endif
	(void) SPAWN(1, "quotaoff", "-a", NULL);
	
	sync();
	fprintf(stderr, "shutdown: turning off swap\r\n");
	(void) SPAWN(0, "swapoff", "-a", NULL);
	fprintf(stderr, "shutdown: unmounting all file systems\r\n");
	(void) SPAWN(0, "unmount", "-a", NULL);

	init_reboot(BMAGIC_POWEROFF);
	exit(0);
}	

int main(void)
{
	printf("Hello, We are shutting down\n");
	sysv_shutdown();
}
