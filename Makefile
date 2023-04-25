CC = g++
CFLAGS = -g
MD = @mkdir -p
RM = rm -rf

HEADER_INSTALL_DIR = /usr/local/include
LIBRARY_INSTALL_DIR = /usr/local/lib

LIB_DIR = lib
BIN_DIR = bin
LIB_WYT = libwyt.a
LIB_ORCHIS = liborchis.a

WYT_H = $(wildcard wyt/*.h)
WYT_C = 
WYT_O = 

test: hash


hash: CHECK_DIR
	@echo link test application about $@ ...
	@$(CC) $(CFLAGS) test/$@.cc  -L lib -o $(BIN_DIR)/$@.elf $(DEPS)
	@echo build $@ test application finished



%.o : %.cc
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	$(RM) $(BIN_DIR)/*.elf

CHECK_DIR:
	$(MD) $(LIB_DIR) $(BIN_DIR)
