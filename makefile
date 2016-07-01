CC=gcc
CFLAGS= -c -o 
READL_FLAG= -lreadline

SOURCES=shell.c	#Input C file
PROGRAMS=shell		#Output program, "name.out"
LIBRARIES=shell.o	#Object file
INCLUDES=		

OBJS=$(SOURCES:.c=.o)

all: 
	$(CC) -c $(SOURCES) -o $(LIBRARIES)
	$(CC) $(LIBRARIES) $(READL_FLAG) 


shell:
	#$(CC) -c $(SOURCES) $(LIBRARIES)
	$(CC) $(LIBRARIES) $(LDFLAGS) -o $(PROGRAMS) $(READL_FLAG) 



.PHONY: clean
clean:
	rm -rf *.o *.out *~ $(PROGRAMS)
