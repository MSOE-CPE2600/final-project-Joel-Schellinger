all:
	gcc chat_server.c -o chat_server -lpthread
	gcc chat_client.c -o chat_client -lpthread

