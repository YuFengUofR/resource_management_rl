# Reinforcement Learning on Resource Management

This project is using reinforcement learning on real-time resource scheduling in a system runtime. 

What we used in this proejct.

  - PARSEC: a runtime benchmark suit. http://parsec.cs.princeton.edu/parsec3-doc.htm
  - PAPI: a toolkit to inspect different hardware counter, http://icl.cs.utk.edu/papi/
  - DVFS: dynamic voltage and frequency scaling.
  - Message Queue: a share queue for information communication, http://man7.org/linux/man-pages/man7/mq_overview.7.html

### What's inside

  - main.cc: the runtime harness to support the RL.
  - freqcpu.h: a wrapper of default OS DVFS, can use it to change the CPU governor and frequency.
  - probe.h: a wrapper of PAPI to get the hardware counter of a particular program.
  - msg_control.h: use message queue to send message between the benchmark program and main program. The main program serves as a server to *ONLY* collect data from the benchmark program.


### Installation

##### Install PAPI
In stall the PAPI package from source. First, clone the PAPI package from source.
```sh
$ cd ~
$ git clone https://bitbucket.org/icl/papi.git
$ cd papi
$ git pull https://bitbucket.org/icl/papi.git
```
Follow the general installation instruction.
```sh
$ ./configure
$ make 
$ make test
```
To test your install, you can run verbose tests:
```
$ ./run_tests.sh -v
```
To install directly, you can use:
```
$ make install-all
```
##### Install PARSEC

To install PARSEC, first, download the PARSEC package from http://parsec.cs.princeton.edu/parsec3-doc.htm. You can also follow the instruction in that website.

```sh
cd ~
$ wget http://parsec.cs.princeton.edu/download/3.0/parsec-3.0-core.tar.gz 
```
Unpack PARSEC 3.0 package
```sh
$ tar -xzf parsec-3.0.tar.gz
$ cd parsec-3.0 
$ ls 
```
Setup environment variable before running PARSEC.
```
$ source env.sh
```
`NOTE`: you might need to set this path to `parsecmgmt` every time.

Untar the `pkgs.tar.gz` from this repository, and replace the `pkgs` folder in the `parsec-3.0`. The modified `pkgs` folder makes some changes, so that benchmarks in the folder can communicate with each other.
```
$ tar-xvzf pkgs.tar.gz
```

Build/run/uninstall benchmarks
```
$ parsecmgmt -a build -p all
```
To run a parsec benchmark, using command such as:
```sh
$ sudo parsecmgmt -a run -p -i sim<small|middle|large>
```

If this cannot work, you might need to dig into each benchmark suit and make them manually :). 

##### Install this runtime

Clone this repo and 
```sh
$ make all 
```
To run,
```sh
$ sudo ./main
```




