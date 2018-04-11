all: control_test

CXX=g++
LDFLAGS=-lpthread
# DEPS =
CFLAGS = -O3 -g -Wall

control_test: control.cc # $(DEPS)
	$(CXX) $(CFLAGS)  $< $(LDFLAGS) -o $@
