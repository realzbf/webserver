CXX = g++
CFLAGS = -std=c++11 -O2 -Wall -g 

TARGET = server
OBJS = ./log/*.cc ./pool/*.cc ./http/*.cc ./server/*.cc \
       ./utils/*.cc ./main.cpp

all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o ./bin/$(TARGET)  -pthread

clean:
	rm -rf ../bin/$(OBJS) $(TARGET)
