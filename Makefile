CC	= gcc 
CFLAGS  = -O2 -Wall -D_GNU_SOURCE

SOURCES	= rapid128.c infr.c line.c page.c loadpgm.c pgmimages.c codetree2.c codetabs.c
OBJECTS	= ${SOURCES:.c=.o}
OUT	= rapid128
LIBS	= -lm -lrt

all: $(OUT)
	@echo Build DONE.
	
$(OUT): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $(OUT) $(OBJECTS) $(LIBS)

clean:
	rm -f $(OBJECTS) *~ $$$$~* *.bak core config.log $(OUT)

