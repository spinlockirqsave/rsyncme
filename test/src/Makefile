CC                      = gcc
CFLAGS                  = -c -g -DDEBUG -Wall -Wextra -D_GNU_SOURCE -Wfatal-errors -std=gnu99 $(INCLUDES)
CPPFLAGS                +=              #compiler flags
LDFLAGS                 = -lpthread -lcmocka -L../../include -L../include  #-lpcap
TESTSRCDIR              = .
AUXOBJDIR		= ../../build/debug
AUXOBJECTS = $(filter-out $(AUXOBJDIR)/main.o, $(wildcard $(AUXOBJDIR)/*.o))
SRCDIR			= .

SRCS = $(wildcard *.c)
_SRCS = $(filter-out md5_test.c, $(SRCS))
_TESTOBJECTS = $(patsubst %.c,%.o,$(_SRCS))
INCLUDES  := -I. -I../../include -I../../include/twlist/include -I../include

TESTOUTPUTDIR           = ../build
TESTOBJECTS             = $(patsubst %,$(TESTOUTPUTDIR)/%,$(_TESTOBJECTS))

##
all : test

test:	$(TESTOUTPUTDIR)/test_rm_main

$(TESTOUTPUTDIR)/test_rm_main:	$(AUXOBJECTS) $(TESTOUTPUTDIR)/test_rm.o $(TESTOUTPUTDIR)/test_rm_main.o
	$(CC) $(AUXOBJECTS) $(LDFLAGS) $ $(TESTOUTPUTDIR)/test_rm.o $(TESTOUTPUTDIR)/test_rm_main.o -o $@


test-check:	all
	$(TESTOUTPUTDIR)/test_rm_main

$(TESTOUTPUTDIR)/%: %.o
	$(CC) $(AUXOBJECTS) $< -o $@ $(LDFLAGS)

$(TESTOUTPUTDIR)/%.o: $(TESTSRCDIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

.c.o:  
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@


clean:
	find $(TESTOUTPUTDIR) -maxdepth 1 -type f -exec rm -f {} \;
clean-logs:
	find $(TESTOUTPUTDIR)/log  -maxdepth 1 -not -path '*/\.*' -type f -exec rm -f {} \;
#clean-all:
#		make clean && make clean-logs
