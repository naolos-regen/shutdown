#include <systemd/sd-bus.h>
#include <stdio.h>

#define SHUTDOWN_DEST		"org.freedesktop.login1"
#define SHUTDOWN_PATH		"/org/freedesktop/login1"
#define SHUTDOWN_INTERFACE	"org.freedesktop.login1.Manager"
#define SHUTDOWN_MEMBER		"PowerOff"

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
							



#define nq_bus_call_shutdown(bus, error)							\
				sd_bus_call_method						\
				(								\
							bus,					\
							SHUTDOWN_DEST,				\
							SHUTDOWN_PATH,				\
							SHUTDOWN_INTERFACE,			\
							SHUTDOWN_MEMBER,			\
							&error,					\
							NULL,					\
							"b",					\
							1					\
				);
int sysd_shutdown()
{
	sd_bus *bus = NULL;
	sd_bus_error error = SD_BUS_ERROR_NULL;	
	int sd_return;
	
	sd_return = sd_bus_open_system(&bus);
	if (sd_return < 0)
	{
		fprintf(stderr, "opening bus failed: %s\n", strerror(-sd_return));
		goto end;
	}
	sd_return = nq_bus_call_shutdown(bus, error);
	if (sd_return < 0)
	{
		fprintf(stderr, "call failed: %s\n", error.message);
		goto cleanup_sd_bus;
	}
	sd_bus_error_free(&error);
cleanup_sd_bus:
	sd_bus_unref(bus);
end:
	return sd_return < 0 ? 1 : 0;
}

int main(void)
{
	return (sysd_shutdown());
}
