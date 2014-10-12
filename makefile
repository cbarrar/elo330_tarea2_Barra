prog=csa.c
comp=gcc
out=csa
flag= -o $(out)
RM=rm
default:
	$(comp) $(flag) $(prog)

clean:
	$(RM) $(out)
