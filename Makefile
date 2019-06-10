CC = gcc
CFLAGS = -Wall -g


all: demo

demo: ht_main_test.o Primary_Hash_Functions.o Primary_Hash_Utilities.o Secondary_Hash_Functions.o Secondary_Hash_Utilities.o BF_64.a 
	$(CC) -o demo mymain.o Primary_Hash_Functions.o Primary_Hash_Utilities.o Secondary_Hash_Functions.o Secondary_Hash_Utilities.o BF_64.a -static $(CFLAGS)

mymain.o: ht_main_test.c BF.h Primary_Hash_Functions.h Secondary_Hash_Functions.h
	$(CC) -c ht_main_test.c $(CFLAGS)

Primary_Hash_Functions.o: Primary_Hash_Functions.c Primary_Hash_Utilities.h
	$(CC) -c Primary_Hash_Functions.c $(CFLAGS)

Primary_Hash_Utilities.o: Primary_Hash_Utilities.c Primary_Hash_Utilities.h
	$(CC) -c Primary_Hash_Utilities.c $(CFLAGS)

Secondary_Hash_Functions.o: Secondary_Hash_Functions.c Secondary_Hash_Functions.h
	$(CC) -c Secondary_Hash_Functions.c $(CFLAGS)

Secondary_Hash_Utilities.o: Secondary_Hash_Utilities.c Secondary_Hash_Utilities.h
	$(CC) -c Secondary_Hash_Utilities.c $(CFLAGS)


clean: 
	rm demo ht_main_test.o Primary_Hash_Functions.o Primary_Hash_Utilities.o Secondary_Hash_Functions.o Secondary_Hash_Utilities.o




