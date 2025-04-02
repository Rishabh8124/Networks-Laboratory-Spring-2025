s:
	@gcc mysmtp_server.c -o mysmtp_server
	@./mysmtp_server 2525

c:
	@gcc mysmtp_client.c -o mysmtp_client
	@./mysmtp_client 127.0.0.1 2525

clean:
	rm -f mysmtp_server mysmtp_client