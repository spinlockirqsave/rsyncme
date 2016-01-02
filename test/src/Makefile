CC                      = gcc
CFLAGS                  = -c -g -DDEBUG -Wall -Wextra -D_GNU_SOURCE -Wfatal-errors -std=gnu99 $(INCLUDES)
CPPFLAGS                +=              #compiler flags
LDFLAGS                 = -lpthread -lcmocka -L../../include $(AUXOBJECTS)   #-lpcap
TESTSRCDIR              = .
AUXOBJDIR		= ../../build/debug
AUXOBJECTS = $(filter-out $(AUXOBJDIR)/main.o, $(wildcard $(AUXOBJDIR)/*.o))
SRCDIR			= ../../src

SRCS = $(wildcard *.c)
_TESTOBJECTS = $(patsubst %.c,%,$(SRCS))
INCLUDES  := -I. -I../../include -I../../include/twlist/include

TESTOUTPUTDIR           = ../build
TESTOBJECTS             = $(patsubst %,$(TESTOUTPUTDIR)/%,$(_TESTOBJECTS))

##
all : $(TESTOBJECTS)

test-check:	all
	$(TESTOUTPUTDIR)/test_rsyncme

$(TESTOUTPUTDIR)/%: %.o
	$(CC) $(LDFLAGS) $< -o $@

$(TESTOUTPUTDIR)/%.o: $(TESTSRCDIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

.c.o:  
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@


clean:
	rm -rf $(TESTOUTPUTDIR)/*
