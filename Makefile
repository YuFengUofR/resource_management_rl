all: control_test main compute

CXX=g++
LDFLAGS=-L/usr/local/lib -lpapi -lpthread -lrt
DEPS = probe.h cpufreq.h
CFLAGS = -O3 -g -Wall

control_test: control.cc # $(DEPS)
	$(CXX) $(CFLAGS)  $< $(LDFLAGS) -o $@


main: main.cc ${DEPS}
	$(CXX) $(CFALGS)  $< $(LDFLAGS) -o $@

compute: compute.cc ${DEPS}
	$(CXX) $(CFALGS)  $< $(LDFLAGS) -o $@
