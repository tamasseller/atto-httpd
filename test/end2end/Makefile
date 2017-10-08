OUTPUT = httpd-end2end

SOURCES += End2end.cpp

SOURCES += ../../md5/md5.c
SOURCES += ../../http-parser/http_parser.c

INCLUDE_DIRS += ../..
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

CXX=x86_64-linux-gnu-g++-6
CC=x86_64-linux-gnu-gcc-6
CXXFLAGS += $(COMMONFLAGS)
CFLAGS += $(COMMONFLAGS)
LD=$(CXX) 

CPPUTEST_FLAGS += -c

all: $(OUTPUT)

include ../../../../ultimate-makefile/Makefile.ultimate