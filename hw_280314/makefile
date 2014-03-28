CXX := clang
CXXFLAGS := 

ASFDS_SOURCES = asfds.c
FDSCAT_SOURCES = fdscat.c

ASFDS_OBJECTS = $(ASFDS_SOURCES:.c=.o)
FDSCAT_OBJECTS = $(FDSCAT_SOURCES:.c=.o)

EXECS = asfds fdscat

all : $(EXECS)

%.o : %.c
	$(CXX) -c $< -o $@ $(CXXFLAGS)

asfds : $(ASFDS_OBJECTS)
	$(CXX) -o $@ $< $(CXXFLAGS)

fdscat: $(FDSCAT_OBJECTS)
	$(CXX) -o $@ $< $(CXXFLAGS)

clean:
	rm -f $(FDSCAT_OBJECTS) $(ASFDS_OBJECTS) $(EXECS)
