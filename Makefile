CXXFLAGS := -O3 -ggdb
LDFLAGS :=


all: server client

%: %.cc common.h
	${CXX} ${CXXFLAGS} -o $@ $< ${LDFLAGS}

clean:
	${RM} server client
