CPPFLAGS = $(shell pkg-config --cflags opencv) 
LDLIBS = $(shell pkg-config --libs opencv) -lthermalgrabber
