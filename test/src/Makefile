CC = gcc
CFLAGS = -c -o3 -DNDEBUG -Wall -Wextra -std=c99 -pedantic -Wno-unused-function -Wfatal-errors
CFLAGS_D = -c -g3 -O0 -DDEBUG -Wall -Wextra -std=c99 -pedantic -Wno-unused-function -Wfatal-errors
LDFLAGS = -L../../include -L../include  #-lpcap
LDFLAGS2 = -L../../include -L../include  -Wl,--wrap=fstat -Wl,--wrap=fstat64 -Wl,--wrap=malloc -Wl,--wrap=fread
LDFLAGS9 := -L../../include -L../include  -Wl,--wrap=fopen -Wl,--wrap=fopen64 -Wno-nonnull
LDFLAGS_D = -g -L../../include -L../include  #-lpcap
LDFLAGS2_D = -g -L../../include -L../include  -Wl,--wrap=fstat -Wl,--wrap=fstat64 -Wl,--wrap=malloc -Wl,--wrap=fread
LDFLAGS9_D :=  -g -L../../include -L../include -Wl,--wrap=fopen -Wl,--wrap=fopen64 -Wno-nonnull
LDLIBS = -luuid -lcmocka -pthread
TESTSRCDIR := .
AUXOBJDIR_D := ../../build/debug
AUXOBJDIR := ../../build/release
AUXSRCS := $(filter-out ../../src/main.c, $(wildcard ../../src/*.c))
AUXSRCS := $(filter-out ../../src/rm_cmd.c, $(AUXSRCS))
AUXOBJS := $(patsubst %/%.c, %/$(AUXOBJDIR)/%.o, $(AUXSRCS))
AUXOBJS_D := $(patsubst %/%.c, %/$(AUXOBJDIR_D)/%.o, $(AUXSRCS))
SRCDIR := .

SRCS := $(wildcard *.c)
_SRCS := $(filter-out md5_test.c, $(SRCS))
_TESTOBJECTS := $(patsubst %.c,%.o,$(_SRCS))
INCLUDES += -I. -I../../include -I../../include/twlist/include -I../include

TESTOUTPUTDIR_D := ../build/debug
TESTOBJECTS_D := $(patsubst %,$(TESTOUTPUTDIR_D)/%,$(_TESTOBJECTS))
TESTOUTPUTDIR := ../build/release
TESTOBJECTS := $(patsubst %,$(TESTOUTPUTDIR)/%,$(_TESTOBJECTS))


all :	test test-debug


test:	$(TESTOUTPUTDIR)/test_rm_main1 $(TESTOUTPUTDIR)/test_rm_main2 $(TESTOUTPUTDIR)/test_rm_main3 $(TESTOUTPUTDIR)/test_rm_main4 \
		$(TESTOUTPUTDIR)/test_rm_main5 $(TESTOUTPUTDIR)/test_rm_main6 $(TESTOUTPUTDIR)/test_rm_main7 $(TESTOUTPUTDIR)/test_rm_main8 \
		$(TESTOUTPUTDIR)/test_rm_main9 $(TESTOUTPUTDIR)/test_rm_main10


$(TESTOUTPUTDIR)/test_rm_main1:	$(AUXOBJS) $(TESTOUTPUTDIR)/test_rm1.o $(TESTOUTPUTDIR)/test_rm_main1.o
	$(CC) $(INCLUDES) $(AUXOBJS) $(LDFLAGS) $ $(TESTOUTPUTDIR)/test_rm1.o $(TESTOUTPUTDIR)/test_rm_main1.o -o $@ $(LDLIBS)

$(TESTOUTPUTDIR)/test_rm_main2:	$(AUXOBJS) $(TESTOUTPUTDIR)/test_rm2.o $(TESTOUTPUTDIR)/test_rm_main2.o
	$(CC) $(INCLUDES) $(AUXOBJS) $(LDFLAGS2) $ $(TESTOUTPUTDIR)/test_rm2.o $(TESTOUTPUTDIR)/test_rm_main2.o -o $@ $(LDLIBS)

$(TESTOUTPUTDIR)/test_rm_main3:	$(AUXOBJS) $(TESTOUTPUTDIR)/test_rm3.o $(TESTOUTPUTDIR)/test_rm_main3.o
	$(CC) $(INCLUDES) $(AUXOBJS) $(LDFLAGS) $ $(TESTOUTPUTDIR)/test_rm3.o $(TESTOUTPUTDIR)/test_rm_main3.o -o $@ $(LDLIBS)

$(TESTOUTPUTDIR)/test_rm_main4:	$(AUXOBJS) $(TESTOUTPUTDIR)/test_rm4.o $(TESTOUTPUTDIR)/test_rm_main4.o
	$(CC) $(INCLUDES) $(AUXOBJS) $(LDFLAGS) $ $(TESTOUTPUTDIR)/test_rm4.o $(TESTOUTPUTDIR)/test_rm_main4.o -o $@ $(LDLIBS)

$(TESTOUTPUTDIR)/test_rm_main5:	$(AUXOBJS) $(TESTOUTPUTDIR)/test_rm5.o $(TESTOUTPUTDIR)/test_rm_main5.o
	$(CC) $(INCLUDES) $(AUXOBJS) $(LDFLAGS) $ $(TESTOUTPUTDIR)/test_rm5.o $(TESTOUTPUTDIR)/test_rm_main5.o -o $@ $(LDLIBS)

$(TESTOUTPUTDIR)/test_rm_main6:	$(AUXOBJS) $(TESTOUTPUTDIR)/test_rm6.o $(TESTOUTPUTDIR)/test_rm_main6.o
	$(CC) $(INCLUDES) $(AUXOBJS) $(LDFLAGS) $ $(TESTOUTPUTDIR)/test_rm6.o $(TESTOUTPUTDIR)/test_rm_main6.o -o $@ $(LDLIBS)

$(TESTOUTPUTDIR)/test_rm_main7:	$(AUXOBJS) $(TESTOUTPUTDIR)/test_rm7.o $(TESTOUTPUTDIR)/test_rm_main7.o
	$(CC) $(INCLUDES) $(AUXOBJS) $(LDFLAGS) $ $(TESTOUTPUTDIR)/test_rm7.o $(TESTOUTPUTDIR)/test_rm_main7.o -o $@ $(LDLIBS)

$(TESTOUTPUTDIR)/test_rm_main8:	$(AUXOBJS) $(TESTOUTPUTDIR)/test_rm8.o $(TESTOUTPUTDIR)/test_rm_main8.o
	$(CC) $(INCLUDES) $(AUXOBJS) $(LDFLAGS) $ $(TESTOUTPUTDIR)/test_rm8.o $(TESTOUTPUTDIR)/test_rm_main8.o -o $@ $(LDLIBS)

$(TESTOUTPUTDIR)/test_rm_main9:	$(AUXOBJS) $(TESTOUTPUTDIR)/test_rm9.o $(TESTOUTPUTDIR)/test_rm_main9.o
	$(CC) $(INCLUDES) $(AUXOBJS) $(LDFLAGS9) $ $(TESTOUTPUTDIR)/test_rm9.o $(TESTOUTPUTDIR)/test_rm_main9.o -o $@ $(LDLIBS)

$(TESTOUTPUTDIR)/test_rm_main10:	$(AUXOBJS) $(TESTOUTPUTDIR)/test_rm10.o $(TESTOUTPUTDIR)/test_rm_main10.o
	$(CC) $(INCLUDES) $(AUXOBJS) $(LDFLAGS) $ $(TESTOUTPUTDIR)/test_rm10.o $(TESTOUTPUTDIR)/test_rm_main10.o -o $@ $(LDLIBS)


test-debug:	$(TESTOUTPUTDIR_D)/test_rm_main1 $(TESTOUTPUTDIR_D)/test_rm_main2 $(TESTOUTPUTDIR_D)/test_rm_main3 $(TESTOUTPUTDIR_D)/test_rm_main4 $(TESTOUTPUTDIR_D)/test_rm_main5 $(TESTOUTPUTDIR_D)/test_rm_main6 $(TESTOUTPUTDIR_D)/test_rm_main7 $(TESTOUTPUTDIR_D)/test_rm_main8 $(TESTOUTPUTDIR_D)/test_rm_main9 $(TESTOUTPUTDIR_D)/test_rm_main10


$(TESTOUTPUTDIR_D)/test_rm_main1:	$(AUXOBJS_D) $(TESTOUTPUTDIR_D)/test_rm1.o $(TESTOUTPUTDIR_D)/test_rm_main1.o
	$(CC) $(INCLUDES) $(AUXOBJS_D) $(LDFLAGS_D) $(TESTOUTPUTDIR_D)/test_rm1.o $(TESTOUTPUTDIR_D)/test_rm_main1.o -o $@ $(LDLIBS)

$(TESTOUTPUTDIR_D)/test_rm_main2:	$(AUXOBJS_D) $(TESTOUTPUTDIR_D)/test_rm2.o $(TESTOUTPUTDIR_D)/test_rm_main2.o
	$(CC) $(INCLUDES) $(AUXOBJS_D) $(LDFLAGS2_D) $(TESTOUTPUTDIR_D)/test_rm2.o $(TESTOUTPUTDIR_D)/test_rm_main2.o -o $@ $(LDLIBS)

$(TESTOUTPUTDIR_D)/test_rm_main3:	$(AUXOBJS_D) $(TESTOUTPUTDIR_D)/test_rm3.o $(TESTOUTPUTDIR_D)/test_rm_main3.o
	$(CC) $(INCLUDES) $(AUXOBJS_D) $(LDFLAGS_D) $(TESTOUTPUTDIR_D)/test_rm3.o $(TESTOUTPUTDIR_D)/test_rm_main3.o -o $@ $(LDLIBS)

$(TESTOUTPUTDIR_D)/test_rm_main4:	$(AUXOBJS_D) $(TESTOUTPUTDIR_D)/test_rm4.o $(TESTOUTPUTDIR_D)/test_rm_main4.o
	$(CC) $(INCLUDES) $(AUXOBJS_D) $(LDFLAGS_D) $(TESTOUTPUTDIR_D)/test_rm4.o $(TESTOUTPUTDIR_D)/test_rm_main4.o -o $@ $(LDLIBS)

$(TESTOUTPUTDIR_D)/test_rm_main5:	$(AUXOBJS_D) $(TESTOUTPUTDIR_D)/test_rm5.o $(TESTOUTPUTDIR_D)/test_rm_main5.o
	$(CC) $(INCLUDES) $(AUXOBJS_D) $(LDFLAGS_D) $(TESTOUTPUTDIR_D)/test_rm5.o $(TESTOUTPUTDIR_D)/test_rm_main5.o -o $@ $(LDLIBS)

$(TESTOUTPUTDIR_D)/test_rm_main6:	$(AUXOBJS_D) $(TESTOUTPUTDIR_D)/test_rm6.o $(TESTOUTPUTDIR_D)/test_rm_main6.o
	$(CC) $(INCLUDES) $(AUXOBJS_D) $(LDFLAGS_D) $(TESTOUTPUTDIR_D)/test_rm6.o $(TESTOUTPUTDIR_D)/test_rm_main6.o -o $@ $(LDLIBS)

$(TESTOUTPUTDIR_D)/test_rm_main7:	$(AUXOBJS_D) $(TESTOUTPUTDIR_D)/test_rm7.o $(TESTOUTPUTDIR_D)/test_rm_main7.o
	$(CC) $(INCLUDES) $(AUXOBJS_D) $(LDFLAGS_D) $(TESTOUTPUTDIR_D)/test_rm7.o $(TESTOUTPUTDIR_D)/test_rm_main7.o -o $@ $(LDLIBS)

$(TESTOUTPUTDIR_D)/test_rm_main8:	$(AUXOBJS_D) $(TESTOUTPUTDIR_D)/test_rm8.o $(TESTOUTPUTDIR_D)/test_rm_main8.o
	$(CC) $(INCLUDES) $(AUXOBJS_D) $(LDFLAGS_D) $(TESTOUTPUTDIR_D)/test_rm8.o $(TESTOUTPUTDIR_D)/test_rm_main8.o -o $@ $(LDLIBS)

$(TESTOUTPUTDIR_D)/test_rm_main9:	$(AUXOBJS_D) $(TESTOUTPUTDIR_D)/test_rm9.o $(TESTOUTPUTDIR_D)/test_rm_main9.o
	$(CC) $(INCLUDES) $(AUXOBJS_D) $(LDFLAGS9_D) $(TESTOUTPUTDIR_D)/test_rm9.o $(TESTOUTPUTDIR_D)/test_rm_main9.o -o $@ $(LDLIBS)

$(TESTOUTPUTDIR_D)/test_rm_main10:	$(AUXOBJS_D) $(TESTOUTPUTDIR_D)/test_rm10.o $(TESTOUTPUTDIR_D)/test_rm_main10.o
	$(CC) $(INCLUDES) $(AUXOBJS_D) $(LDFLAGS_D) $(TESTOUTPUTDIR_D)/test_rm10.o $(TESTOUTPUTDIR_D)/test_rm_main10.o -o $@ $(LDLIBS)


test-check:	test
	$(TESTOUTPUTDIR)/test_rm_main1
	$(TESTOUTPUTDIR)/test_rm_main2
	$(TESTOUTPUTDIR)/test_rm_main3
	$(TESTOUTPUTDIR)/test_rm_main4
	$(TESTOUTPUTDIR)/test_rm_main5
	$(TESTOUTPUTDIR)/test_rm_main6
	$(TESTOUTPUTDIR)/test_rm_main7
	$(TESTOUTPUTDIR)/test_rm_main8
	$(TESTOUTPUTDIR)/test_rm_main9
	$(TESTOUTPUTDIR)/test_rm_main10


test-check-debug:	test-debug
	$(TESTOUTPUTDIR_D)/test_rm_main1
	$(TESTOUTPUTDIR_D)/test_rm_main2
	$(TESTOUTPUTDIR_D)/test_rm_main3
	$(TESTOUTPUTDIR_D)/test_rm_main4
	$(TESTOUTPUTDIR_D)/test_rm_main5
	$(TESTOUTPUTDIR_D)/test_rm_main6
	$(TESTOUTPUTDIR_D)/test_rm_main7
	$(TESTOUTPUTDIR_D)/test_rm_main8
	$(TESTOUTPUTDIR_D)/test_rm_main9
	$(TESTOUTPUTDIR_D)/test_rm_main10


$(TESTOUTPUTDIR)/%.o: $(TESTSRCDIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

$(TESTOUTPUTDIR_D)/%.o: $(TESTSRCDIR)/%.c
	$(CC) $(CFLAGS_D) -Wno-nonnull $(INCLUDES) $< -o $@

$(TESTOUTPUTDIR)/test_rm8.o: $(TESTSRCDIR)/test_rm8.c
	$(CC) $(CFLAGS) -Wno-nonnull $(INCLUDES) $< -o $@

$(TESTOUTPUTDIR)/test_rm9.o: $(TESTSRCDIR)/test_rm9.c
	$(CC) $(CFLAGS) -Wno-nonnull $(INCLUDES) $< -o $@

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
