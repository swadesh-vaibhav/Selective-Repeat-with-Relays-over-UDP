make: client server relay
client: client.c packet.h
	gcc -o client client.c
relay: relay.c packet.h
	gcc -o relay relay.c
server: server.c packet.h
	gcc -o server server.c