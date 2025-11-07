CC=g++
CFLAGS+= -g

OBJDIR=obj

SRCS=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp, $(OBJDIR)/%.o, $(SRCS))

LIBS=-lpthread -LCommandParser -lcli -lrt		

testapp.exe:${OBJS} CommandParser/libcli.a
	${CC} ${CFLAGS} ${OBJS} -o $@ ${LIBS}

$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	${CC} ${CFLAGS} -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

CommandParser/libcli.a:
	(cd CommandParser; make)

clean:
	rm -f $(OBJS)
	rmdir $(OBJDIR) 2>/dev/null || true
	rm -f *exe
	(cd CommandParser; make clean)
