build:
	gcc -o user user.c
	gcc -o server server.c
clean:
	rm user
	rm server