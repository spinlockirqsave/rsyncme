CC			= gcc
CFLAGS			= -c -Wall -o2 -Wextra -D_GNU_SOURCE -Wfatal-errors -std=gnu99 $(INCLUDES)
CFLAGS_D		= -c -g -DDEBUG -Wall -Wextra -D_GNU_SOURCE -Wfatal-errors -std=gnu99 $(INCLUDES)
#CFLAGS_D2		= -c -g -DDEBUG -Wall -Wextra -D_GNU_SOURCE -Wfatal-errors -std=gnu99 -Wl,--wrap=fread$(INCLUDES)
CPPFLAGS		+=              #compiler flags
LDFLAGS			= -lpthread -lcmocka -L../../include -L../include  #-lpcap
LDFLAGS2		= -lpthread -lcmocka -L../../include -L../include  -Wl,--wrap=fstat -Wl,--wrap=fstat64 -Wl,--wrap=malloc -Wl,--wrap=fread
TESTSRCDIR		= .
AUXOBJDIR_D		= ../../build/debug
AUXOBJECTS_D 		= $(filter-out $(AUXOBJDIR_D)/main.o, $(wildcard $(AUXOBJDIR_D)/*.o))
AUXOBJDIR		= ../../build/release
AUXOBJECTS 		= $(filter-out $(AUXOBJDIR)/main.o, $(wildcard $(AUXOBJDIR)/*.o))
SRCDIR			= .

SRCS = $(wildcard *.c)
_SRCS = $(filter-out md5_test.c, $(SRCS))
_TESTOBJECTS = $(patsubst %.c,%.o,$(_SRCS))
INCLUDES  := -I. -I../../include -I../../include/twlist/include -I../include

TESTOUTPUTDIR_D		= ../build/debug
TESTOBJECTS_D		= $(patsubst %,$(TESTOUTPUTDIR_D)/%,$(_TESTOBJECTS))
TESTOUTPUTDIR		= ../build/release
TESTOBJECTS		= $(patsubst %,$(TESTOUTPUTDIR)/%,$(_TESTOBJECTS))

##
all : test test-debug

test:	$(TESTOUTPUTDIR)/test_rm_main $(TESTOUTPUTDIR)/test_rm_main2

$(TESTOUTPUTDIR)/test_rm_main:	$(AUXOBJECTS) $(TESTOUTPUTDIR)/test_rm.o $(TESTOUTPUTDIR)/test_rm_main.o
	$(CC) $(AUXOBJECTS) $(LDFLAGS) $ $(TESTOUTPUTDIR)/test_rm.o $(TESTOUTPUTDIR)/test_rm_main.o -o $@

$(TESTOUTPUTDIR)/test_rm_main2:	$(AUXOBJECTS) $(TESTOUTPUTDIR)/test_rm2.o $(TESTOUTPUTDIR)/test_rm_main2.o
	$(CC) $(AUXOBJECTS) $(LDFLAGS2) $ $(TESTOUTPUTDIR)/test_rm2.o $(TESTOUTPUTDIR)/test_rm_main2.o -o $@

test-debug:	$(TESTOUTPUTDIR_D)/test_rm_main $(TESTOUTPUTDIR_D)/test_rm_main2

$(TESTOUTPUTDIR_D)/test_rm_main:	$(AUXOBJECTS_D) $(TESTOUTPUTDIR_D)/test_rm.o $(TESTOUTPUTDIR_D)/test_rm_main.o
	$(CC) $(AUXOBJECTS_D) $(LDFLAGS) $ $(TESTOUTPUTDIR_D)/test_rm.o $(TESTOUTPUTDIR_D)/test_rm_main.o -o $@

$(TESTOUTPUTDIR_D)/test_rm_main2:	$(AUXOBJECTS_D) $(TESTOUTPUTDIR_D)/test_rm2.o $(TESTOUTPUTDIR_D)/test_rm_main2.o
	$(CC) $(AUXOBJECTS_D) $(LDFLAGS2) $ $(TESTOUTPUTDIR_D)/test_rm2.o $(TESTOUTPUTDIR_D)/test_rm_main2.o -o $@

test-check:	test
	$(TESTOUTPUTDIR)/test_rm_main
	$(TESTOUTPUTDIR)/test_rm_main2

test-check-debug:	test
	$(TESTOUTPUTDIR_D)/test_rm_main
	$(TESTOUTPUTDIR)/test_rm_main2

$(TESTOUTPUTDIR)/%: $(AUXOBJECTS)/%.o
	$(CC) $(AUXOBJECTS) $< -o $@ $(LDFLAGS)

$(TESTOUTPUTDIR)/%.o: $(TESTSRCDIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

$(TESTOUTPUTDIR_D)/%: $(AUXOBJECTS_D)/%.o
	$(CC) $(AUXOBJECTS_D) $< -o $@ $(LDFLAGS)

$(TESTOUTPUTDIR_D)/%.o: $(TESTSRCDIR)/%.c
	$(CC) $(CFLAGS_D) $(INCLUDES) $< -o $@
#.c.o:  
#	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

clean:
	find $(TESTOUTPUTDIR) -maxdepth 1 -type f -exec rm -f {} \;
clean-debug:
	find $(TESTOUTPUTDIR_D) -maxdepth 1 -type f -exec rm -f {} \;
clean-logs:
	find $(TESTOUTPUTDIR)/log  -maxdepth 1 -not -path '*/\.*' -type f -exec rm -f {} \;
clean-logs-debug:
	find $(TESTOUTPUTDIR_D)/log  -maxdepth 1 -not -path '*/\.*' -type f -exec rm -f {} \;
clean-all:
		make clean && make clean-debug && make clean-logs && make clean-logs-debug
