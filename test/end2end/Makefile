OUTPUT = httpd-end2end

SOURCES += End2end.cpp

SOURCES += ../../md5/md5.c
SOURCES += ../../http-parser/http_parser.c

INCLUDE_DIRS += ../..
INCLUDE_DIRS += ../../http-parser
INCLUDE_DIRS += ../../md5
INCLUDE_DIRS += ../../pet

COMMONFLAGS += -O0
COMMONFLAGS += -g3
COMMONFLAGS += --coverage
COMMONFLAGS += -fdelete-null-pointer-checks
CXXFLAGS += -std=c++11
COMMONFLAGS += -fmax-errors=5
#COMMONFLAGS += -Wall -Wextra -Wno-unused

CFLAGS += $(COMMONFLAGS)
CXXFLAGS += $(COMMONFLAGS)
CXXFLAGS += -std=c++11

TEST_CMD_PREFIX=LD_LIBRARY_PATH=/opt/gcc-5.3.0/lib64/:/opt/gcc-5.3.0/lib32/
CXX=/opt/gcc-5.3.0/bin/g++
CC=/opt/gcc-5.3.0/bin/gcc
CXXFLAGS += $(COMMONFLAGS)
CFLAGS += $(COMMONFLAGS)
LD=$(CXX) 

CPPUTEST_FLAGS += -c

all: $(OUTPUT)

include ../../../../ultimate-makefile/Makefile.ultimate