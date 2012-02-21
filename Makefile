CC := g++
CFLAGS := -Os
LDFLAGS := -lboost_system -lboost_program_options -lpthread -lssl -lcrypto -lglog

.PHONY: all
all: drakebot

bot.o: bot.cc bot.h
	$(CC) -g $(CFLAGS) -c $< -o $@

drakebot: bot.o main.cc
	$(CC) -g $(CFLAGS) $(LDFLAGS) $^ -o $@

bot_opt.o: bot.cc bot.h
	$(CC) -DNDEBUG $(CFLAGS) -c $< -o $@

drakebot_opt: bot_opt.o main.cc
	$(CC) -DNDEBUG $(CFLAGS) $(LDFLAGS) $^ -o $@
	strip -s $@

.PHONY: clean
clean:
	rm -f drakebot drakebot_opt *.o
