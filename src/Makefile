CC			= gcc
CFLAGS			= -c -Wall -Wextra -D_GNU_SOURCE -Wfatal-errors -std=gnu99
CPPFLAGS 		+= 		#compiler flags
LDFLAGS			= -lpthread 	#-lpcap
SRCDIR 			= .
TESTSRCDIR		= ../test/src
DEBUGOUTPUTDIR 		= ../build/debug
RELEASEOUTPUTDIR	= ../build/release
#TESTOUTPUTDIR		= ../test/build/release
#TESTOUTPUTDIR_D		= ../test/build/debug
SOURCES			= main.c rm.c rm_rx.c rm_tx.c rm_error.c rm_core.c rm_do_msg.c	\
				rm_session.c rm_signal.c rm_serialize.c rm_util.c	\
				md5.c
#TESTSOURCES		= ../test/src/test_rsyncme.c
INCLUDES		= -I. -I../include -I../include/twlist/include
_OBJECTS		= $(SOURCES:.c=.o)
_TESTOBJECTS		= $(TESTSOURCES:.c=.o)
DEBUGOBJECTS 		= $(patsubst %,$(DEBUGOUTPUTDIR)/%,$(_OBJECTS))
RELEASEOBJECTS 		= $(patsubst %,$(RELEASEOUTPUTDIR)/%,$(_OBJECTS))
#TESTOBJECTS 		= $(patsubst %,$(TESTOUTPUTDIR)/%,$(_TESTOBJECTS))
#TESTOBJECTS_D 		= $(patsubst %,$(TESTOUTPUTDIR_D)/%,$(_TESTOBJECTS))
DEBUGTARGET		= ../build/debug/rsyncme_d
RELEASETARGET		= ../build/release/rsyncme_d
CMDDEBUGTARGET		= ../build/debug/rsyncme
CMDRELEASETARGET	= ../build/release/rsyncme

debugall:	$(SOURCES) $(DEBUGTARGET)
releaseall:	$(SOURCES) $(RELEASETARGET)

# additional flags
# CONFIG_DEBUG_LIST	- extensive debugging of list with external debugging
# 			functions
debug:		CFLAGS += -DDEBUG -g
debug:		debugall

release:	CFLAGS += -DNDEBUG -o2
release: 	releaseall

test-debug:	CFLAGS += -DDEBUG -g
test-debug:	debugall
	cd $(TESTSRCDIR) && make test-debug

test:	CFLAGS += -DNDEBUG -o2
test:	releaseall
	cd $(TESTSRCDIR) && make test

test-check:	test
	cd $(TESTSRCDIR) && make test-check
test-check-debug:	test
	cd $(TESTSRCDIR) && make test-check-debug

rsyncme-debug:	rm_cmd.c ../build/debug/rsyncme
rsyncme-debug:	CFLAGS += -DDEBUG -g
rsyncme-release:	rm_cmd.c ../build/release/rsyncme
rsyncme-release:	CFLAGS += -DNDEBUG

$(DEBUGTARGET): $(DEBUGOBJECTS) 
	$(CC) $(LDFLAGS) $(DEBUGOBJECTS) -o $@

$(RELEASETARGET): $(RELEASEOBJECTS) 
	$(CC) $(LDFLAGS) $(RELEASEOBJECTS) -o $@

$(CMDDEBUGTARGET): ../build/debug/rsyncme.o 
	$(CC) $(LDFLAGS) $(CMDDEBUGTARGET).o -o $@

../build/debug/rsyncme.o: rm_cmd.c
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

$(CMDRELEASETARGET): ../build/release/rsyncme.o 
	$(CC) $(LDFLAGS) $(CMDRELEASETARGET).o -o $@

../build/release/rsyncme.o: rm_cmd.c
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

$(DEBUGOUTPUTDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

$(RELEASEOUTPUTDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

#$(TESTOUTPUTDIR)/%.o: $(TESTSRCDIR)/%.c
#	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@
.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

clean:
	rm -rf $(DEBUGOBJECTS) $(DEBUGTARGET) $(RELEASEOBJECTS) $(RELEASETARGET) \
	$(CMDDEBUGTARGET) $(CMDDEBUGTARGET).o $(CMDRELEASETARGET) $(CMDRELEASETARGET).o
test-clean:
	#rm -rf $(TESTOBJECTS)
	cd $(TESTSRCDIR) && make clean
test-clean-debug:
	#rm -rf $(TESTOBJECTS_D)
	cd $(TESTSRCDIR) && make clean-debug
test-clean-logs:
	cd $(TESTSRCDIR) && make clean-logs
test-clean-logs-debug:
	cd $(TESTSRCDIR) && make clean-logs-debug
test-clean-all:
	cd $(TESTSRCDIR) && make clean && make clean-logs && make clean-debug && make clean-logs-debug
clean-all:
	make clean
	cd $(TESTSRCDIR) && make clean && make clean-logs && make clean-debug && make clean-logs-debug
