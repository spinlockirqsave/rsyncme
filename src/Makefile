CC			= gcc
CFLAGS			= -c -Wall -Wextra -D_GNU_SOURCE -Wfatal-errors -std=gnu99
CPPFLAGS 		+= 		#compiler flags
LDFLAGS			= -lpthread 	#-lpcap
SRCDIR 			= .
TESTSRCDIR		= ../test/src
DEBUGOUTPUTDIR 		= ../build/debug
RELEASEOUTPUTDIR	= ../build/release
TESTOUTPUTDIR		= ../test/build
SOURCES			= main.c rm.c rm_rx.c rm_tx.c rm_error.c rm_core.c rm_do_msg.c \
				rm_session.c rm_signal.c
TESTSOURCES		= ../test/src/test_rsyncme.c
INCLUDES		= -I. -I../include -I../include/twlist/include
_OBJECTS		= $(SOURCES:.c=.o)
_TESTOBJECTS		= $(TESTSOURCES:.c=.o)
DEBUGOBJECTS 		= $(patsubst %,$(DEBUGOUTPUTDIR)/%,$(_OBJECTS))
RELEASEOBJECTS 		= $(patsubst %,$(RELEASEOUTPUTDIR)/%,$(_OBJECTS))
TESTOBJECTS 		= $(patsubst %,$(TESTOUTPUTDIR)/%,$(_TESTOBJECTS))
DEBUGTARGET		= ../build/debug/rsyncme
RELEASETARGET		= ../build/release/rsyncme

debugall:	$(SOURCES) $(DEBUGTARGET)
releaseall:	$(SOURCES) $(RELEASETARGET)

# additional flags
# CONFIG_DEBUG_LIST	- extensive debugging of list with external debugging
# 			functions
debug:		CFLAGS += -DDEBUG -g
debug:		debugall

release:	CFLAGS += -DNDEBUG
release: 	releaseall

test:		CFLAGS += -DDEBUG -g
test:		debugall
	cd $(TESTSRCDIR) && make
test-check:	test
	cd $(TESTSRCDIR) && make test-check

$(DEBUGTARGET): $(DEBUGOBJECTS) 
	$(CC) $(LDFLAGS) $(DEBUGOBJECTS) -o $@

$(RELEASETARGET): $(RELEASEOBJECTS) 
	$(CC) $(LDFLAGS) $(RELEASEOBJECTS) -o $@

#test: $(TESTSOURCES)
#	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

$(DEBUGOUTPUTDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

$(RELEASEOUTPUTDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

#$(TESTOUTPUTDIR)/%.o: $(TESTSRCDIR)/%.c
#	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@
.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

clean:
	rm -rf $(DEBUGOBJECTS) $(DEBUGTARGET) $(RELEASEOBJECTS) $(RELEASETARGET) $(TESTOBJECTS)
	cd $(TESTSRCDIR) && make clean
