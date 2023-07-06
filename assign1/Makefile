CC=gcc
CFLAGS=-I.
DEPS = dberror.h storage_mgr.h test_helper.h
OBJ = dberror.o storage_mgr.o test_assign1_1.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

test_assign1: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
	
.PHONY : clean
clean :
	$(RM) *.o test_assign1 -r
