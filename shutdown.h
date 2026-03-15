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


int sysd_shutdown()
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

#elif defined(NQ_SYSV) 

void sysv_shutdown()
{

}

#elif defined(NQ_RCD) 

void rcd_shutdown()
{

}

#elif defined(NQ_OPENRC)

void orc_shutdown()
{

}

#elif defined(NQ_INITRC)

void irc_shutdown()
{	
	
}

#endif

#endif

#endif
