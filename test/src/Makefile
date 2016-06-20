CC = gcc
CFLAGS = -c -Wall -o3 -Wextra -D_GNU_SOURCE -Wfatal-errors -std=gnu99
CFLAGS_D = -c -g3 -O0 -DDEBUG -Wall -Wextra -D_GNU_SOURCE -Wfatal-errors -std=gnu99
LDFLAGS = -lpthread -lcmocka -L../../include -L../include  #-lpcap
LDFLAGS2 = -lpthread -lcmocka -L../../include -L../include  -Wl,--wrap=fstat -Wl,--wrap=fstat64 -Wl,--wrap=malloc -Wl,--wrap=fread
LDFLAGS9 := -lpthread -lcmocka -L../../include -L../include  -Wl,--wrap=fopen -Wl,--wrap=fopen64

LDFLAGS_D = -g -lpthread -lcmocka -L../../include -L../include  #-lpcap
LDFLAGS2_D = -g -lpthread -lcmocka -L../../include -L../include  -Wl,--wrap=fstat -Wl,--wrap=fstat64 -Wl,--wrap=malloc -Wl,--wrap=fread
LDFLAGS9_D :=  -g -lpthread -lcmocka -L../../include -L../include -Wl,--wrap=fopen -Wl,--wrap=fopen64
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


test:	$(TESTOUTPUTDIR)/test_rm_main1 $(TESTOUTPUTDIR)/test_rm_main2 $(TESTOUTPUTDIR)/test_rm_main3 $(TESTOUTPUTDIR)/test_rm_main4 $(TESTOUTPUTDIR)/test_rm_main5 $(TESTOUTPUTDIR)/test_rm_main6 $(TESTOUTPUTDIR)/test_rm_main7 $(TESTOUTPUTDIR)/test_rm_main8 $(TESTOUTPUTDIR)/test_rm_main9


$(TESTOUTPUTDIR)/test_rm_main1:	$(AUXOBJS) $(TESTOUTPUTDIR)/test_rm1.o $(TESTOUTPUTDIR)/test_rm_main1.o
	$(CC) $(INCLUDES) $(AUXOBJS) $(LDFLAGS) $ $(TESTOUTPUTDIR)/test_rm1.o $(TESTOUTPUTDIR)/test_rm_main1.o -o $@

$(TESTOUTPUTDIR)/test_rm_main2:	$(AUXOBJS) $(TESTOUTPUTDIR)/test_rm2.o $(TESTOUTPUTDIR)/test_rm_main2.o
	$(CC) $(INCLUDES) $(AUXOBJS) $(LDFLAGS2) $ $(TESTOUTPUTDIR)/test_rm2.o $(TESTOUTPUTDIR)/test_rm_main2.o -o $@

$(TESTOUTPUTDIR)/test_rm_main3:	$(AUXOBJS) $(TESTOUTPUTDIR)/test_rm3.o $(TESTOUTPUTDIR)/test_rm_main3.o
	$(CC) $(INCLUDES) $(AUXOBJS) $(LDFLAGS) $ $(TESTOUTPUTDIR)/test_rm3.o $(TESTOUTPUTDIR)/test_rm_main3.o -o $@

$(TESTOUTPUTDIR)/test_rm_main4:	$(AUXOBJS) $(TESTOUTPUTDIR)/test_rm4.o $(TESTOUTPUTDIR)/test_rm_main4.o
	$(CC) $(INCLUDES) $(AUXOBJS) $(LDFLAGS) $ $(TESTOUTPUTDIR)/test_rm4.o $(TESTOUTPUTDIR)/test_rm_main4.o -o $@

$(TESTOUTPUTDIR)/test_rm_main5:	$(AUXOBJS) $(TESTOUTPUTDIR)/test_rm5.o $(TESTOUTPUTDIR)/test_rm_main5.o
	$(CC) $(INCLUDES) $(AUXOBJS) $(LDFLAGS) $ $(TESTOUTPUTDIR)/test_rm5.o $(TESTOUTPUTDIR)/test_rm_main5.o -o $@

$(TESTOUTPUTDIR)/test_rm_main6:	$(AUXOBJS) $(TESTOUTPUTDIR)/test_rm6.o $(TESTOUTPUTDIR)/test_rm_main6.o
	$(CC) $(INCLUDES) $(AUXOBJS) $(LDFLAGS) $ $(TESTOUTPUTDIR)/test_rm6.o $(TESTOUTPUTDIR)/test_rm_main6.o -o $@

$(TESTOUTPUTDIR)/test_rm_main7:	$(AUXOBJS) $(TESTOUTPUTDIR)/test_rm7.o $(TESTOUTPUTDIR)/test_rm_main7.o
	$(CC) $(INCLUDES) $(AUXOBJS) $(LDFLAGS) $ $(TESTOUTPUTDIR)/test_rm7.o $(TESTOUTPUTDIR)/test_rm_main7.o -o $@

$(TESTOUTPUTDIR)/test_rm_main8:	$(AUXOBJS) $(TESTOUTPUTDIR)/test_rm8.o $(TESTOUTPUTDIR)/test_rm_main8.o
	$(CC) $(INCLUDES) $(AUXOBJS) $(LDFLAGS) $ $(TESTOUTPUTDIR)/test_rm8.o $(TESTOUTPUTDIR)/test_rm_main8.o -o $@

$(TESTOUTPUTDIR)/test_rm_main9:	$(AUXOBJS) $(TESTOUTPUTDIR)/test_rm9.o $(TESTOUTPUTDIR)/test_rm_main9.o
	$(CC) $(INCLUDES) $(AUXOBJS) $(LDFLAGS9) $ $(TESTOUTPUTDIR)/test_rm9.o $(TESTOUTPUTDIR)/test_rm_main9.o -o $@


test-debug:	$(TESTOUTPUTDIR_D)/test_rm_main1 $(TESTOUTPUTDIR_D)/test_rm_main2 $(TESTOUTPUTDIR_D)/test_rm_main3 $(TESTOUTPUTDIR_D)/test_rm_main4 $(TESTOUTPUTDIR_D)/test_rm_main5 $(TESTOUTPUTDIR_D)/test_rm_main6 $(TESTOUTPUTDIR_D)/test_rm_main7 $(TESTOUTPUTDIR_D)/test_rm_main8 $(TESTOUTPUTDIR_D)/test_rm_main9


$(TESTOUTPUTDIR_D)/test_rm_main1:	$(AUXOBJS_D) $(TESTOUTPUTDIR_D)/test_rm1.o $(TESTOUTPUTDIR_D)/test_rm_main1.o
	$(CC) $(INCLUDES) $(AUXOBJS_D) $(LDFLAGS_D) $(TESTOUTPUTDIR_D)/test_rm1.o $(TESTOUTPUTDIR_D)/test_rm_main1.o -o $@

$(TESTOUTPUTDIR_D)/test_rm_main2:	$(AUXOBJS_D) $(TESTOUTPUTDIR_D)/test_rm2.o $(TESTOUTPUTDIR_D)/test_rm_main2.o
	$(CC) $(INCLUDES) $(AUXOBJS_D) $(LDFLAGS2_D) $(TESTOUTPUTDIR_D)/test_rm2.o $(TESTOUTPUTDIR_D)/test_rm_main2.o -o $@

$(TESTOUTPUTDIR_D)/test_rm_main3:	$(AUXOBJS_D) $(TESTOUTPUTDIR_D)/test_rm3.o $(TESTOUTPUTDIR_D)/test_rm_main3.o
	$(CC) $(INCLUDES) $(AUXOBJS_D) $(LDFLAGS_D) $(TESTOUTPUTDIR_D)/test_rm3.o $(TESTOUTPUTDIR_D)/test_rm_main3.o -o $@

$(TESTOUTPUTDIR_D)/test_rm_main4:	$(AUXOBJS_D) $(TESTOUTPUTDIR_D)/test_rm4.o $(TESTOUTPUTDIR_D)/test_rm_main4.o
	$(CC) $(INCLUDES) $(AUXOBJS_D) $(LDFLAGS_D) $(TESTOUTPUTDIR_D)/test_rm4.o $(TESTOUTPUTDIR_D)/test_rm_main4.o -o $@

$(TESTOUTPUTDIR_D)/test_rm_main5:	$(AUXOBJS_D) $(TESTOUTPUTDIR_D)/test_rm5.o $(TESTOUTPUTDIR_D)/test_rm_main5.o
	$(CC) $(INCLUDES) $(AUXOBJS_D) $(LDFLAGS_D) $(TESTOUTPUTDIR_D)/test_rm5.o $(TESTOUTPUTDIR_D)/test_rm_main5.o -o $@

$(TESTOUTPUTDIR_D)/test_rm_main6:	$(AUXOBJS_D) $(TESTOUTPUTDIR_D)/test_rm6.o $(TESTOUTPUTDIR_D)/test_rm_main6.o
	$(CC) $(INCLUDES) $(AUXOBJS_D) $(LDFLAGS_D) $(TESTOUTPUTDIR_D)/test_rm6.o $(TESTOUTPUTDIR_D)/test_rm_main6.o -o $@

$(TESTOUTPUTDIR_D)/test_rm_main7:	$(AUXOBJS_D) $(TESTOUTPUTDIR_D)/test_rm7.o $(TESTOUTPUTDIR_D)/test_rm_main7.o
	$(CC) $(INCLUDES) $(AUXOBJS_D) $(LDFLAGS_D) $(TESTOUTPUTDIR_D)/test_rm7.o $(TESTOUTPUTDIR_D)/test_rm_main7.o -o $@

$(TESTOUTPUTDIR_D)/test_rm_main8:	$(AUXOBJS_D) $(TESTOUTPUTDIR_D)/test_rm8.o $(TESTOUTPUTDIR_D)/test_rm_main8.o
	$(CC) $(INCLUDES) $(AUXOBJS_D) $(LDFLAGS_D) $(TESTOUTPUTDIR_D)/test_rm8.o $(TESTOUTPUTDIR_D)/test_rm_main8.o -o $@

$(TESTOUTPUTDIR_D)/test_rm_main9:	$(AUXOBJS_D) $(TESTOUTPUTDIR_D)/test_rm9.o $(TESTOUTPUTDIR_D)/test_rm_main9.o
	$(CC) $(INCLUDES) $(AUXOBJS_D) $(LDFLAGS9_D) $(TESTOUTPUTDIR_D)/test_rm9.o $(TESTOUTPUTDIR_D)/test_rm_main9.o -o $@


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


$(TESTOUTPUTDIR)/%.o: $(TESTSRCDIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

$(TESTOUTPUTDIR_D)/%.o: $(TESTSRCDIR)/%.c
	$(CC) $(CFLAGS_D) $(INCLUDES) $< -o $@


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
