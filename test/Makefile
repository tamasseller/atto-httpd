OUTPUT = httpd-test

SOURCES += TestMain.cpp
SOURCES += TestUXml.cpp
SOURCES += TestParser.cpp
SOURCES += TestKeywords.cpp
SOURCES += TestKvParser.cpp
SOURCES += TestHexParser.cpp
SOURCES += TestUrlParser.cpp
SOURCES += TestAuthDigest.cpp
SOURCES += TestPathParser.cpp
SOURCES += TestSplitParser.cpp
SOURCES += TestHttpLogicDav.cpp
SOURCES += TestHttpLogicErrors.cpp
SOURCES += TestHttpLogicNormal.cpp
SOURCES += TestDavRequestParser.cpp
SOURCES += TestHttpLogicOutput.cpp
SOURCES += TestTemporaryStringBuffer.cpp
SOURCES += TestConstantStringMatcher.cpp
ifeq ("x","y")
endif

SOURCES += ../../../lib/md5/md5.c
SOURCES += ../../../lib/http-parser/http_parser.c

INCLUDE_DIRS += ..
INCLUDE_DIRS += ../../../lib/http-parser
INCLUDE_DIRS += ../../../lib/md5
INCLUDE_DIRS += ../../../lib/pet

LIBS += CppUTest
LIBS += CppUTestExt

COMMONFLAGS += -O0
COMMONFLAGS += -g3
COMMONFLAGS += --coverage
COMMONFLAGS += -fdelete-null-pointer-checks
COMMONFLAGS += -fmax-errors=5
COMMONFLAGS += -Wall -Wextra -Wno-unused

CXXFLAGS += -std=c++11
CXXFLAGS += -DCPPUTEST_USE_MEM_LEAK_DETECTION=0
CXXFLAGS += -fno-exceptions

USESANITIZERS=1

ifeq ($(USESANITIZERS), 1)
CXXFLAGS += -fsanitize=address
CXXFLAGS += -fsanitize=leak
CXXFLAGS += -fsanitize=undefined
CXXFLAGS += -fsanitize=vla-bound
CXXFLAGS += -fsanitize=null
CXXFLAGS += -fsanitize=return
CXXFLAGS += -fsanitize=bounds	
CXXFLAGS += -fsanitize=object-size
CXXFLAGS += -fsanitize=bool
CXXFLAGS += -fsanitize=enum
CXXFLAGS += -fsanitize=vptr
CXXFLAGS += -fchkp-check-incomplete-type
CXXFLAGS += -fchkp-narrow-bounds
CXXFLAGS += -fchkp-first-field-has-own-bounds
CXXFLAGS += -fchkp-narrow-to-innermost-array
CXXFLAGS += -fchkp-check-read
CXXFLAGS += -fchkp-check-write
CXXFLAGS += -fchkp-store-bounds
CXXFLAGS += -fchkp-instrument-calls
CXXFLAGS += -fchkp-instrument-marked-only
CXXFLAGS += -fchkp-use-wrappers
CXXFLAGS += -fstack-protector
CXXFLAGS += -fstack-protector-strong
CXXFLAGS += -fstack-check
endif

TEST_CMD_PREFIX=LD_LIBRARY_PATH=/opt/gcc-5.3.0/lib64/:/opt/gcc-5.3.0/lib32/
CXX=/opt/gcc-5.3.0/bin/g++
CC=/opt/gcc-5.3.0/bin/gcc
CXXFLAGS += $(COMMONFLAGS)
CFLAGS += $(COMMONFLAGS)
LD=$(CXX) 

CPPUTEST_FLAGS += -c

include ../../../ultimate-makefile/Makefile.ultimate