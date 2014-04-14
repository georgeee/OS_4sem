CXX := clang
CXXFLAGS := 

SOURCES = main.c
OBJECTS = $(SOURCES:.c=.o)


all : main_
	sudo chmod 4755 main

main_ : main
	sudo chown root:root main

%.o : %.c
	$(CXX) -c -o $@ $< $(CXXFLAGS)

main : $(OBJECTS)
	$(CXX) -o $@ $< $(CXXFLAGS)

clean :
	rm -f main $(OBJECTS)
