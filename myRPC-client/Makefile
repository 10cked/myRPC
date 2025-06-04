CC := gcc  
#LIBDIR := ../libmysyslog  
CFLAGS := -Wall -O2
# -Wall -O2 -I$(LIBDIR)  
LDFLAGS := 
#-L$(LIBDIR) -lmysyslog  

SRC := src/myRPC-client.c  
OBJ := myRPC-client.o  
BIN := myRPC-client  

version     := 1.0
revision    := 0
architecture := all

.PHONY: all clean deb  

all: $(BIN)
#$(LIBDIR)/libmysyslog.so $(BIN)  

#$(LIBDIR)/libmysyslog.so:  
#	$(MAKE) -C $(LIBDIR)  

$(OBJ): $(SRC)  
	$(CC) $(CFLAGS) -c $< -o $@  

$(BIN): $(OBJ)  
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)  

deb: deb_client all
	@rm -rf MyRPC_client_$(version)-$(revision)_$(architecture)  

deb_client:
	mkdir -p MyRPC_client_$(version)-$(revision)_$(architecture)/DEBIAN  
	printf "Package: myrpc-client\nVersion: $(version)\nArchitecture: $(architecture)\nDepends: libmysyslog\nMaintainer: gandc <admin@gandc.ru>\nDescription: Client for remote command execution via myRPC\n" \
		> MyRPC_client_$(version)-$(revision)_$(architecture)/DEBIAN/control
	mkdir -p MyRPC_client_$(version)-$(revision)_$(architecture)/usr/local/bin  
	cp $(BIN) MyRPC_client_$(version)-$(revision)_$(architecture)/usr/local/bin  
	dpkg-deb --root-owner-group --build MyRPC_client_$(version)-$(revision)_$(architecture)  

clean:  
	rm -f $(OBJ) $(BIN)  
