# Output executables
SERVER_BIN = server
CLIENT_BIN = client

# Default target
run: $(SERVER_BIN) $(CLIENT_BIN)
	@./$(SERVER_BIN) & \
	gnome-terminal --tab -- bash -c "./$(CLIENT_BIN); exec bash"

$(SERVER_BIN): wordserver.c
	@gcc wordserver.c -o server

$(CLIENT_BIN): wordclient.c
	@gcc wordclient.c -o client

clean:
	@rm -f $(SERVER_BIN) $(CLIENT_BIN)