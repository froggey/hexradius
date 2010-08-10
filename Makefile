CXX ?= g++
CXXFLAGS ?= -Wall
INCLUDES ?=
LIBS ?= -lSDL -lSDL_image

OBJS := src/main.o src/loadimage.o

all: octradius

octradius: $(OBJS)
	$(CXX) $(CXXFLAGS) -o octradius $(OBJS) $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<
