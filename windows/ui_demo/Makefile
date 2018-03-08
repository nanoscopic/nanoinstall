CSRCS = xz/xz_crc32.c xz/xz_dec_stream.c xz/xz_dec_lzma2.c
#COS = xz/xz_crc32.o xz/xz_dec_stream.o xz/xz_dec_lzma2.o
#CCOS = xmlbare/misc.o xmlbare/parser.o xmlbare/red_black_tree.o xmlbare/stack.o xmlbare/string_tree.o
CCSRCS = ui_demo.c xmlbare/red_black_tree.c xmlbare/string_tree.c xmlbare/parser.c xmlbare/stack.c xmlbare/misc.c

HDRS = xmlbare/parser.h xmlbare/string_tree.h

CC = gcc -Wno-write-strings
CCC = g++

NO_UNUSED = -ffunction-sections -fdata-sections -Wl,--gc-sections
LIBFLAGS = -municode -loleaut32 -luuid -lgdi32 -lwinhttp 

CFLAGS = -g -Wall -pedantic  -O3 $(LIBFLAGS) $(NO_UNUSED)

PROGRAM = ui_demo.exe

.PHONY:	mem_check clean

all: $(PROGRAM)

xz_crc32.o: xz_crc32.c
	$(CC) -c xz_crc32.c
    
xz_dec_stream.o: xz_dec_stream.c
	$(CC) -c xz_dec_stream.c

xz_dec_lzma2.o: xz_dec_lzma2.c
	$(CC) -c xz_dec_lzma2.c

$(PROGRAM): 	$(CCSRCS) $(SRCS) $(COS)
	$(CCC) -o $(PROGRAM) $(CCSRCS) $(COS) $(CFLAGS)
	strip $(PROGRAM)

clean:			
	rm -f *.o *.lo *.la *.slo *~ *.exe






