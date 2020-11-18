all: lumberjack

lumberjack: lumberjack.c
	$(CC) -o $@ $^ $(INCLUDES) $(LIBS) $(LDFLAGS)

clean:
	rm -rf lumberjack

