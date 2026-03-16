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
			fprintf(stderr, "Failed to open initctl fifo: %s\n", strerror(errno));
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
			fprintf(stderr, "Failed to write to /run/initctl: %s\n", strerror(errno));
			return;
		}
		p += r;
		bytes -= r;
	} while (bytes > 0);
}

#define NAME	"INIT_HALT"
#define VALUE	"POWEROFF"

void sysvinit_runlevel(char level)
{
	struct init_request request;

	if (!level)
		return;

	request = (struct init_request) {
		.magic = INIT_MAGIC,
		.sleeptime = 0,
		.cmd = INIT_CMD_RUNLVL,
		.runlevel = level,
	};

	sysv_send_cmd(&request);
}

void sysv_shutdown()
{
	struct init_request	request;
	size_t			nl;
	size_t			vl;

	memset(&request, 0, sizeof(request));
	request.magic = INIT_MAGIC;
	request.cmd   = INIT_CMD_SETENV;
	nl = strlen(NAME);
	vl = strlen(VALUE);
	if (nl + vl + 3 >= (int)sizeof(request.i.data))
		return;

	memcpy(request.i.data, NAME, nl);
	request.i.data[nl] = '=';
	memcpy(request.i.data + nl + 1, VALUE, vl);
	sysv_send_cmd(&request);

	sysvinit_runlevel('0');
}

void shutdown()
{
	if (access("/run/initctl", F_OK) == 0)
		sysv_shutdown();
	else
		shutdown_fallback();
}

int main(void)
{
	shutdown();
}
