PROGRAM = tsdump

SOURCES = core/tsdump.c core/ts_output.c core/load_modules.c core/default_decoder.c utils/tsdstr.c utils/arib_parser.c utils/path.c utils/advanced_buffer.c utils/aribstr.c
MODULES = modules/mod_path_resolver.c modules/mod_log.c modules/mod_filein.c modules/mod_fileout.c modules/mod_cmdexec.c
MODULES_LINUX = modules/mod_bondriver.c modules/mod_arib25.c
MODULES := $(if $(shell uname -a | grep -i linux), $(MODULES) $(MODULES_LINUX), $(MODULES) )

CC := gcc

OBJS = $(SOURCES:.c=.o) $(MODULES:.c=.o)

#CFLAGS = -O3 -flto -Wall -I$(CURDIR)
CFLAGS = -Ofast -march=native -Wall -flto -I$(CURDIR)
#CFLAGS = -O0 -Wall -g -I$(CURDIR)

LDFLAGS = -flto -lm -ldl
LDFLAGS := $(if $(shell uname -a | grep -i linux), $(LDFLAGS) -larib25, $(LDFLAGS))
LDFLAGS := $(if $(shell uname -a | grep -i cygwin), $(LDFLAGS) -liconv, $(LDFLAGS))

$(PROGRAM): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $(PROGRAM)

#SUFFIXES: .o .c

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

.cpp.o:
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -f $(PROGRAM) $(OBJS)
