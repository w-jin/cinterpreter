CXX := g++
LLVMVERSION := 6.0
RTTIFLAG := -fno-rtti
CXXFLAGS := $(shell llvm-config-$(LLVMVERSION) --cxxflags) $(RTTIFLAG)
LDFLAGS := $(shell llvm-config-$(LLVMVERSION) --ldflags)
SOURCES = cinterpreter.cpp
OBJECTS = $(SOURCES:.cpp=.o)
EXES = $(OBJECTS:.o=)

STATIC_LIBS= \
	-lclangFrontend \
	-lclangTooling \
	-lclangParse \
	-lclangSema \
	-lclangAnalysis \
	-lclangEdit \
	-lclangAST \
	-lclangLex \
	-lclangBasic \
	-lclangDriver \
	-lclangSerialization \

DYNAMIC_LIBS= \
	-lLLVM \
	-lclang \

CLANG_LIBS=$(LDFLAGS) -Wl,-Bstatic $(STATIC_LIBS) -Wl,-Bdynamic $(DYNAMIC_LIBS)

all: $(OBJECTS) $(EXES)
%: %.o
	$(CXX) -o $@ $< $(CLANG_LIBS)

clean:
	rm *.o cinterpreter
