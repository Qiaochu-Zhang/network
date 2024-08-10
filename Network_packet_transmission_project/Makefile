CC=gcc
CFLAGS=-Wall -g

#all: serverM serverD serverS serverU client
all: serverM serverD serverS serverU client
serverM: serverM.o
	$(CC) $(CFLAGS) -o serverM serverM.o

serverD: serverD.o
	$(CC) $(CFLAGS) -o serverD serverD.o

serverS: serverS.o
	$(CC) $(CFLAGS) -o serverS serverS.o

serverU: serverU.o
	$(CC) $(CFLAGS) -o serverU serverU.o

client: client.o
	$(CC) $(CFLAGS) -o client client.o

serverM.o: serverM.c
	$(CC) $(CFLAGS) -c serverM.c

serverD.o: serverD.c
	$(CC) $(CFLAGS) -c serverD.c

serverS.o: serverS.c
	$(CC) $(CFLAGS) -c serverS.c

serverU.o: serverU.c
	$(CC) $(CFLAGS) -c serverU.c

client.o: client.c
	$(CC) $(CFLAGS) -c client.c

clean:
	rm -f serverM serverD serverS serverU client *.o
