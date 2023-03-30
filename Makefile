CC=cc
CFLAGS=-c -I./include
LNFLAGS=-lreadline

BINNAME=timer
OBJS=out/timer.o out/util.o out/time_funcs.o out/duration.o out/run.o out/category.o

out/%.o: src/%.c
	mkdir -p out
	$(CC) $(CFLAGS) $^ -o $@

$(BINNAME): $(OBJS)
	$(CC) $(LNFLAGS) $^ -o $@

clean:
	rm out/*.o $(BINNAME)
