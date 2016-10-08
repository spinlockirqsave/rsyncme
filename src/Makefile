CC = gcc
CFLAGS	=
CFLAGS_RELEASE = -c -o3 -DNDEBUG -Wall -Wextra -std=c99 -pedantic -Wformat -Wno-unused-function -Wfatal-errors -Werror
CFLAGS_DEBUG = -c -g3 -O0 -DDEBUG -Wall -Wextra -std=c99 -pedantic -Wformat -Wno-unused-function -Wfatal-errors -Werror
CPPFLAGS += #compiler flags
LDFLAGS = #-lpcap
LDFLAGS_D = -g #-lpcap
LDLIBS := -luuid -pthread
SRCDIR = .
TESTSRCDIR = ../test/src
DEBUGOUTPUTDIR = ../build/debug
RELEASEOUTPUTDIR = ../build/release
TESTOUTPUTDIR = ../test/build/release
TESTOUTPUTDIR_D = ../test/build/debug
SOURCES = main.c rm_daemon.c rm_wq.c rm.c rm_tcp.c rm_rx.c rm_tx.c rm_error.c rm_core.c rm_do_msg.c rm_session.c rm_signal.c rm_serialize.c rm_util.c md5.c
#TESTSOURCES = ../test/src/test_rsyncme.c
INCLUDES = -I. -I../include -I../include/twlist/include
_OBJECTS = $(SOURCES:.c=.o)
_TESTOBJECTS = $(TESTSOURCES:.c=.o)
DEBUGOBJECTS = $(patsubst %,$(DEBUGOUTPUTDIR)/%,$(_OBJECTS))
RELEASEOBJECTS = $(patsubst %,$(RELEASEOUTPUTDIR)/%,$(_OBJECTS))
DEBUGOBJECTS_NO_MAIN = $(filter-out $(DEBUGOUTPUTDIR)/main.o, $(DEBUGOBJECTS))
RELEASEOBJECTS_NO_MAIN = $(filter-out $(RELEASEOUTPUTDIR)/main.o, $(RELEASEOBJECTS))
#TESTOBJECTS = $(patsubst %,$(TESTOUTPUTDIR)/%,$(_TESTOBJECTS))
#TESTOBJECTS_D = $(patsubst %,$(TESTOUTPUTDIR_D)/%,$(_TESTOBJECTS))
DEBUGTARGET = ../build/debug/rsyncme_d
RELEASETARGET = ../build/release/rsyncme_d
CMDDEBUGTARGET = ../build/debug/rsyncme
CMDRELEASETARGET = ../build/release/rsyncme

all:	release

# additional flags
# CONFIG_DEBUG_LIST	- extensive debugging of list with external debugging
# 			functions
debug:		CFLAGS += $(CFLAGS_DEBUG)
debug:		$(SOURCES) $(DEBUGTARGET) rsyncme-debug

release:	CFLAGS += $(CFLAGS_RELEASE)
release: 	$(SOURCES) $(RELEASETARGET) rsyncme-release

rsyncme-debug:		CFLAGS += $(CFLAGS_DEBUG)
rsyncme-debug:		$(CMDDEBUGTARGET)
	cp $(CMDDEBUGTARGET) $(TESTOUTPUTDIR_D)
rsyncme-release:	CFLAGS += $(CFLAGS_RELEASE)
rsyncme-release:	$(CMDRELEASETARGET)
	cp $(CMDRELEASETARGET) $(TESTOUTPUTDIR)

test-debug:	CFLAGS += $(CFLAGS_DEBUG)
test-debug:	debug
	make -C $(TESTSRCDIR) test-debug

test:	CFLAGS += $(CFLAGS_RELEASE)
test:	release
	cd $(TESTSRCDIR) && make test

test-check:	test
	cd $(TESTSRCDIR) && make test-check
test-check-debug:	test-debug
	cd $(TESTSRCDIR) && make test-check-debug

install:	all
	cp $(CMDRELEASETARGET) /usr/bin

$(DEBUGTARGET): $(DEBUGOBJECTS) 
	$(CC) $(LDFLAGS_D) $(DEBUGOBJECTS) -o $@ $(LDLIBS)

$(RELEASETARGET): $(RELEASEOBJECTS) 
	$(CC) $(LDFLAGS) $(RELEASEOBJECTS) -o $@ $(LDLIBS)

$(CMDDEBUGTARGET):	$(DEBUGOBJECTS_NO_MAIN) $(CMDDEBUGTARGET).o
	$(CC) $(LDFLAGS_D) $(DEBUGOBJECTS_NO_MAIN) $(CMDDEBUGTARGET).o -o $@ $(LDLIBS)

$(CMDDEBUGTARGET).o: rm_cmd.c
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LDLIBS)

$(CMDRELEASETARGET):	$(RELEASEOBJECTS_NO_MAIN) $(CMDRELEASETARGET).o
	$(CC) $(LDFLAGS) $(RELEASEOBJECTS_NO_MAIN) $(CMDRELEASETARGET).o -o $@ $(LDLIBS)

$(CMDRELEASETARGET).o: rm_cmd.c
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LDLIBS)

$(DEBUGOUTPUTDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LDLIBS)

$(RELEASEOUTPUTDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LDLIBS)

#$(DEBUGOBJECTS_NO_MAIN)/%.o: $(SRCDIR)/%.c	the rule is already included in rule for $(DEBUGOUTPUTDIR)/%.o
#	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LDLIBS)
#$(TESTOUTPUTDIR)/%.o: $(TESTSRCDIR)/%.c
#	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LDLIBS)
.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LDLIBS)

.PHONY: clean
clean:
	rm -rf $(DEBUGOBJECTS) $(DEBUGTARGET) $(RELEASEOBJECTS) $(RELEASETARGET) \
	$(CMDDEBUGTARGET) $(CMDDEBUGTARGET).o $(CMDRELEASETARGET) $(CMDRELEASETARGET).o
.PHONY: test-clean
test-clean:
	#rm -rf $(TESTOBJECTS)
	cd $(TESTSRCDIR) && make clean
.PHONY: test-clean-debug
test-clean-debug:
	#rm -rf $(TESTOBJECTS_D)
	cd $(TESTSRCDIR) && make clean-debug
.PHONY: test-clean-logs
test-clean-logs:
	cd $(TESTSRCDIR) && make clean-logs
.PHONY: test-clean-logs-debug
test-clean-logs-debug:
	cd $(TESTSRCDIR) && make clean-logs-debug
.PHONY: test-clean-all
test-clean-all:
	cd $(TESTSRCDIR) && make clean && make clean-logs && make clean-debug && make clean-logs-debug
.PHONY: clean-all
clean-all:
	make clean
	cd $(TESTSRCDIR) && make clean && make clean-logs && make clean-debug && make clean-logs-debug
