# Makefile for Proxy Lab 
#
# You may modify this file any way you like (except for the handin
# rule). You instructor will type "make" on your specific Makefile to
# build your proxy from sources.

CC = gcc
CFLAGS = -g -Og -Wall -Wshadow -Wextra
LDFLAGS = -lpthread

all: clear proxy

clear:
	@rm -f .depend

.depend:
	@$(CC) $(CFLAGS) -MM *.c > $@

include .depend

proxy: proxy.o csapp.o head.o body.o message.o request.o response.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%: %.o
	$(CC) $(filter %.o,$^) -o $@ $(LDFLAGS)

# Creates a tarball in ../proxylab-handin.tar that you can then
# hand in. DO NOT MODIFY THIS!
handin:
	(make clean; cd ..; tar cvf $(USER)-proxylab-handin.tar proxylab-handout --exclude tiny --exclude nop-server.py --exclude proxy --exclude driver.sh --exclude port-for-user.pl --exclude free-port.sh --exclude ".*")

clean: clear
	rm -f *~ *.o proxy core *.tar *.zip *.gzip *.bzip *.gz

.PHONY: all clear handin clean
