OUTPUT = httpd-test

SOURCES += TestMain.cpp
SOURCES += TestUXml.cpp
SOURCES += TestUJson.cpp
SOURCES += TestBase64.cpp
SOURCES += TestParser.cpp
SOURCES += TestKeywords.cpp
SOURCES += TestKvParser.cpp
SOURCES += TestHexParser.cpp
SOURCES += TestIntParser.cpp
SOURCES += TestUrlParser.cpp
SOURCES += TestJsonParser.cpp
SOURCES += TestAuthDigest.cpp
SOURCES += TestUJsonAbuse.cpp
SOURCES += TestPathParser.cpp
SOURCES += TestSplitParser.cpp
SOURCES += TestHttpLogicDav.cpp
SOURCES += TestHttpLogicErrors.cpp
SOURCES += TestHttpLogicNormal.cpp
SOURCES += TestHttpLogicOutput.cpp
SOURCES += TestDavRequestParser.cpp
SOURCES += TestTemporaryStringBuffer.cpp
SOURCES += TestConstantStringMatcher.cpp
SOURCES += ../md5/md5.c
SOURCES += ../http-parser/http_parser.c

SOURCES += ../pet/1test/TestRunner.cpp
SOURCES += ../pet/1test/FailureInjector.cpp
SOURCES += ../pet/1test/TraceOutput.cpp

SOURCES += ../pet/ubiquitous/PrintfWriter.cpp

INCLUDE_DIRS += .
INCLUDE_DIRS += ..
INCLUDE_DIRS += ../pet

COMMONFLAGS += -O0
COMMONFLAGS += -g3
COMMONFLAGS += --coverage
COMMONFLAGS += -fdelete-null-pointer-checks
COMMONFLAGS += -fmax-errors=5
COMMONFLAGS += -Wall -Wextra -Wno-unused
COMMONFLAGS += -fno-pic
COMMONFLAGS += -fno-pie

CXXFLAGS += -std=c++11
CXXFLAGS += -DCPPUTEST_USE_MEM_LEAK_DETECTION=0
CXXFLAGS += -fno-exceptions
CXXFLAGS += -fno-inline
CXXFLAGS += -DEXPECT_POOL_SIZE="(32767)"

USESANITIZERS=0

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

LIBS += gcov 
LDFLAGS += -static
LDFLAGS += -z norelro

CXX=x86_64-linux-gnu-g++-6
CC=x86_64-linux-gnu-gcc-6
CXXFLAGS += $(COMMONFLAGS)
CFLAGS += $(COMMONFLAGS)
LD=$(CXX) 

include ultimate-makefile/Makefile.ultimate
