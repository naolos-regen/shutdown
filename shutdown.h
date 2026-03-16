#ifndef NQ_SHUTDOWN
#define NQ_SHUTDOWN

void shutdown();

#ifdef NQ_SHUTDOWN_IMPL

#if defined(NQ_WINDOWS)

void win_shutdown()
{
	// TODO: There won't be any implementation yet
	win_shutdown();
}

#elif defined(NQ_SYSTEMD) 

#include <systemd/sd-bus.h>

#define SHUTDOWN_DEST		"org.freedesktop.login1"
#define SHUTDOWN_PATH		"/org/freedesktop/login1"
#define SHUTDOWN_INTERFACE	"org.freedesktop.login1.Manager"

#define SHUTDOWN_MEMBER		"PowerOff"
#define REBOOT_MEMBER		"Reboot"

#define nq_bus_call_method(bus, error, member)							\
				sd_bus_call_method						\
				(								\
							bus,					\
							SHUTDOWN_DEST,				\
							SHUTDOWN_PATH,				\
							SHUTDOWN_INTERFACE,			\
							member,					\
							&error,					\
							NULL,					\
							"b",					\
							1					\
				);
							

#define nq_bus_call_reboot(bus, error)		nq_bus_call_method(bus, error, REBOOT_MEMBER)
#define nq_bus_call_shutdown(bus, error)	nq_bus_call_method(bus, error, SHUTDOWN_MEMBER)


int sysv_shutdown()
{
	sd_bus *bus = NULL;
	sd_bus_error error = SD_BUS_ERROR_NULL;	
	int sd_return;
	
	sd_return = sd_bus_open_system(&bus);
	if (sd_return < 0)
		goto end;
	sd_return = nq_bus_call_shutdown(bus, error);
	if (sd_return < 0)
		goto cleanup_sd_bus;
	sd_bus_error_free(&error);
cleanup_sd_bus:
	sd_bus_unref(bus);
end:
	return sd_return < 0 ? 1 : 0;
}

void shutdown()
{
	if (!sysv_shutdown())
	{
		
	}
}

#elif defined(NQ_SYSV) || defined (NQ_OPENRC)

#include <errno.h>
#include <getopt.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <utmp.h>


#define INIT_MAGIC		0x03091969
#define INIT_CMD_START		0
#define INIT_CMD_RUNLVL		1
#define INIT_CMD_POWERFAIL	2
#define INIT_CMD_POWERFAILNOW	3
#define INIT_CMD_POWEROK	4
#define INIT_CMD_BSD		5
#define INIT_CMD_SETENV		6
#define INIT_CMD_UNSETENV	7

struct init_request_bsd
{
	char	gen_id[8];
	char	tty_id[16];
	char	host[64];
	char	term_type[16];
	int	signal;
	int	pid;
	char	exec_name[128];
	char	reserved[128];
};

struct init_request
{
	int	magic;
	int	cmd;
	int	runlevel;
	int	sleeptime;
	union {
		struct init_request_bsd	bsd;
		char			data[368];
	} i;
};

void sysv_send_cmd(struct init_request *request)
{
	int	fd;
	char	*p;
	size_t	bytes;
	ssize_t r;

	fd = open("/run/initctl", O_WRONLY | O_NONBLOCK | O_CLOEXEC | O_NOCTTY);
	if (fd < 0)
	{
		if (errno != ENOENT)
			fprintf(stderr, "Failed to open initctl fifo: %s", strerror(errno));
		return;
	}
	p = (char *) request;
	bytes = sizeof(*request);
	do {
		r = write (fd, p, bytes);
		if (r < 0)
		{
			if ((errno == EAGAIN) || (errno == EINTR))
				continue;
			fprintf(stderr, "Failed to write to /run/initctl: %s", strerror(errno));
			return;
		}
		p += r;
		bytes -= r;
	} while (bytes > 0);
}

void sysv_shutdown()
{
	const char * name = "INIT_HALT";
	const char * value = "POWEROFF";
	struct init_request	request;
	size_t			nl;
	size_t			vl;

	memset(&request, 0, sizeof(request));
	request.magic = INIT_MAGIC;
	request.cmd   = INIT_CMD_SETENV;
	nl = strlen(name);
	if (value)
		vl = strlen(value);
	else
		vl = 0;
	if (nl + vl + 3 >= (int)sizeof(request.i.data))
		return;

	memcpy(request.i.data, name, nl);
	if (value)
	{
		request.i.data[nl] = '=';
		memcpy(request.i.data + nl + 1, value, vl);
	}
	sysv_send_cmd(&request);
}	

#elif defined(NQ_S6) 

void s6_shutdown()
{

}

#elif defined(NQ_DINIT)

void dinit_shutdown()
{	
	
}

#endif

#endif

#endif
