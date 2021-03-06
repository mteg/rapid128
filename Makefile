CC	= gcc 
CFLAGS  = -O3 -Wall -D_GNU_SOURCE -funroll-loops -DFLIP_CAPS -DSHADE_CAPS
# -DNEGATIVE_CAPS

SOURCES	= infr.c line.c page.c loadpgm.c pgmimages.c codetree2.c codetabs.c readbits.c options.c
OBJECTS	= ${SOURCES:.c=.o}
OUT	= rapid128.a
LIBS	= -lm -lrt
LDFLAGS = 


all: rapid128
	@echo Build DONE.

rapid128: $(OUT) rapid128.c
	$(CC) $(LDFLAGS) -o rapid128 rapid128.c rapid128.a $(LIBS)
	
$(OUT): $(OBJECTS)
	ar cr $(OUT) $(OBJECTS)

clean:
	rm -f $(OBJECTS) *~ $$$$~* *.bak core config.log $(OUT) rapid128

