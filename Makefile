all: control_test main2

CXX=g++
LDFLAGS=-L/usr/local/lib -lpapi -lpthread
DEPS = probe.h cpufreq.h
CFLAGS = -O3 -g -Wall

control_test: control.cc # $(DEPS)
	$(CXX) $(CFLAGS)  $< $(LDFLAGS) -o $@


main2: main2.cc ${DEPS}
	$(CXX) $(CFALGS)  $< $(LDFLAGS) -o $@
