CC = gcc
AR = ar
RANLIB = ranlib
CFLAGS = -Wall -Wextra -O2 -g
LIB_NAME = libktp.a

lib:
	@gcc -c ktp.c -o ktp.o
	@ar rcs libktp.a ktp.o
	@ranlib libktp.a

run1:
	@gcc user1.c -o u1 -L. -lktp
	@./u1

run2:
	@gcc user2.c -o u2 -L. -lktp
	@./u2

init:
	@gcc ktp_init.c -o ktpinit
	@./ktpinit

clean:
	@rm -f ktp.o libktp.a u1 u2 ktpinit
