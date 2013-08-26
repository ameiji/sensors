#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/sysctl.h>

#include <gtk/gtk.h>
#include <cairo.h>




#define PROGNAME "SENSORS"

#define ICONFILE "/usr/local/share/sensors/icons/cpu.png"

#define REFRESH 3 	/* default refresh time,  sec. */
#define MAXREFRESH 3600 /* max refresh time, sec. */
#define MAXNCPU 64 	/* kern.smp.maxcpus */


#define L1 "CPU" /* text labels */
#define L2 "Freq:"
#define L3 "Temp:"
#define L4 "User:"
#define L5 "Sys:"
#define L6 "Total:"
#define ERRLABEL "<i>error</i>"

#define FREQ 1 /* labels' indices */
#define TEMP 2
#define USER 3
#define SYS 4
#define TOTAL 5

#define OS_MIB "kern.osrelease"
#define NCPU_MIB "kern.smp.cpus"
#define FREQ_MIB "dev.cpu.0.freq"
#define CP_TIMES_MIB "kern.cp_times"

#define CP_STATES 5
#define CP_USER 0
#define CP_NICE 1
#define CP_SYS  2
#define CP_INTR 3
#define CP_IDLE 4


