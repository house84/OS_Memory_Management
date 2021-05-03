CC = gcc
CFLAGS = -g 
LDLIBS = -lm -lpthread -lrt
LIBPATH = -L . -l 

TARGET1 = oss
OBJ1 = oss.o Q.o

TARGET2 = user_proc
OBJ2 = user.o

TARGETLIB = libsharedFunc.a
LIBOBJ = sharedFunc.o
LIBC = sharedFunc.c

HEADERS = headers.h shared.h oss.h user.h sharedFunc.h Q.h

.SUFFIXES: .c .o

ALL: $(TARGETLIB) $(TARGET1) $(TARGET2)

$(TARGET1): $(OBJ1)
	$(CC) $(CFLAGS) -o $(TARGET1) $(OBJ1) $(TARGETLIB) $(LDLIBS)

$(TARGET2): $(OBJ2) 
	$(CC) $(CFLAGS) -o $(TARGET2) $(OBJ2) $(TARGETLIB) $(LIDLIBS)

$(TARGETLIB): $(LIBOBJ) $(HEADERS)
	ar rs $@ $^

$(LIBOBJ): $(LIBC)
	$(CC) -c $(LIBC)

clean: 
	rm -f $(TARGET1) $(TARGET2) $(TARGETLIB) *.o
