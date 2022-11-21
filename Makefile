CXXFLAGS := -O3 -ggdb
LDFLAGS :=


all: server client

%: %.cc
	${CXX} ${CXXFLAGS} -o $@ $< ${LDFLAGS}

clean:
	${RM} server client
