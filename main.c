#define NQ_SYSTEMD
#define NQ_SHUTDOWN_IMPL
#include "shutdown.h"

int main(void)
{
	shutdown();
}
