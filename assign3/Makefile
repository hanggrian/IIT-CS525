CC=gcc
CFLAGS=-I.
DEPS = dberror.h storage_mgr.h buffer_mgr.h dt.h buffer_mgr_stat.h expr.h rm_serializer.h record_mgr.h test_helper.h
OBJ = dberror.o storage_mgr.o buffer_mgr.o buffer_mgr_stat.o expr.o rm_serializer.o record_mgr.o 


# %.o: %.c $(DEPS)
# 	$(CC) -c -o $@ $< $(CFLAGS)

all: test_assign3_1 test_expr

test_assign3_1.o: test_assign3_1.c
	$(CC) -c test_assign3_1.c

test_assign3_1: $(OBJ) test_assign3_1.o
	$(CC) -o $@ $^ $(CFLAGS)

test_expr.o: test_expr.c
	$(CC) -c test_expr.c

test_expr: $(OBJ) test_expr.o
	$(CC) -o $@ $^ $(CFLAGS)

dberror.o: dberror.c dberror.h
	$(CC) -c dberror.c

record_mgr.o: record_mgr.c record_mgr.h tables.h buffer_mgr.h storage_mgr.h
	$(CC) -c record_mgr.c

buffer_mgr.o: buffer_mgr.c buffer_mgr.h dberror.h storage_mgr.h
	$(CC) -c buffer_mgr.c

buffer_mgr_stat.o: buffer_mgr_stat.c buffer_mgr_stat.h buffer_mgr.h
	$(CC) -c buffer_mgr_stat.c

storage_mgr.o: storage_mgr.c storage_mgr.h dberror.h
	$(CC) -c storage_mgr.c

rm_serializer.o: rm_serializer.c dberror.h tables.h record_mgr.h
	$(CC) -c rm_serializer.c

expr.o: expr.c dberror.h record_mgr.h expr.h tables.h
	$(CC) -c expr.c

.PHONY : clean
clean :
	$(RM) *.o test_assign3_1 -r
	$(RM) *.o test_expr -r

