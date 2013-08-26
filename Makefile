PROG = sensors

SRCS = sensors.c sensors.h

CFLAGS = -O -std=c99

BDECFLAGS =     -W -Wall -ansi -pedantic -Wbad-function-cast -Wcast-align \
               -Wcast-qual -Wchar-subscripts -Winline \
               -Wmissing-prototypes -Wnested-externs -Wpointer-arith \
               -Wredundant-decls -Wshadow -Wstrict-prototypes -Wwrite-strings

GTK_FLAGS = `pkg-config --libs --cflags gtk+-2.0`

CFLAGS += ${BDECFLAGS} ${GTK_FLAGS}

.include <bsd.prog.mk>


