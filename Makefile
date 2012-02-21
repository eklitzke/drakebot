CC := g++
CFLAGS := -g -Os
LDFLAGS := -lboost_system -lboost_program_options -lpthread -lssl -lcrypto

.PHONY: all
all: drakebot

bot.o: bot.cc bot.h
	$(CC) $(CFLAGS) -c $< -o $@

drakebot: bot.o main.cc
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

.PHONY: tiny
tiny: drakebot
	strip -s $<

.PHONY: clean
clean:
	rm -f drakebot *.o
