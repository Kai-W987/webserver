CXX = g++
CFLAGS = -g -Wall -std=c++11

TARGET = app
OBJS = main.cpp buffer.cpp epoller.cpp httpconn.cpp webserver.cpp threadpool.cpp locker.cpp log.cpp

all: $(OBJS)
	$(CXX) $(CFLAGS) -o $(TARGET) $(OBJS) -lpthread

.PHONY: clean
clean:
	rm -f $(TARGET)