all: receiver.c sender.c
	gcc -g -o exec_receiver receiver.c -lpthread
	gcc -g -o exec_sender sender.c -lm

r: receiver.c
	gcc -g -o exec_receiver receiver.c -lpthread 

clean: 
	rm -rf exec_receiver
	rm -rf exec_sender
