#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>



int shared_var = 1;


void * connection_handler(void *socket_fd) {

	int thread_quit = 1;
	char a;
	printf("\nclient started\n");
	while(thread_quit) {
		scanf("%c",&a);
		if(a == 'Q') {
			shared_var = 0;
			thread_quit = 0;
		}
	}
	pthread_exit(0);
}


int main(int argc, char *argv[]) {

	if(argc < 5) {
		printf("\nUsage :- %s hostname port command resource.\n",argv[0]);
	}

	char response[4096];
	char request[1024];
	FILE * fp;

	char * hostname = argv[1];
	printf("hostname :- %s\n",hostname);

	int port = atoi(argv[2]);
	printf("port :- %d\n",port);

	char * command = argv[3];
	printf("command :- %s\n",command);

	char * file_path = argv[4];
	printf("file_path :- %s\n",file_path);


	int client_socket;
	client_socket = socket(AF_INET, SOCK_STREAM, 0);

	// connect to an address
	struct sockaddr_in remote_address;
	remote_address.sin_family = AF_INET;
	remote_address.sin_port = htons(port);
	inet_aton(hostname,(struct in_addr *) &remote_address.sin_addr.s_addr);

	connect(client_socket, (struct sockaddr *) &remote_address, sizeof(remote_address));

	sprintf(request,"%s %s HTTP/1.1\r\nHost: %s\r\n\r\n",command,file_path,hostname);

	pthread_t thread_id;

	if( pthread_create( &thread_id , NULL ,  connection_handler , NULL ) < 0) {
		perror("could not create thread");
		return 1;
	}
	printf("\npthread created for client");
	
	fp = fopen("./put_data.txt", "r");
	fseek(fp,0, SEEK_END);
	long fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char * file_data = malloc(fsize +1);
	fread(file_data, fsize,1 , fp);
	strcat(request, file_data);

	while(shared_var) {	
		if(!strcmp(command,"GET")) {
			send(client_socket, request, strlen(request), 0);
			recv(client_socket, &response, sizeof(response), 0);
			printf("Response from the server: %s\n", response);
		} else if(!strcmp(command,"PUT")){
			send(client_socket, request, strlen(request), 0);
			recv(client_socket, &response, sizeof(response), 0);
			printf("PUT command sent to server succesfully\n");
			printf("Response from the server: %s\n", response);
		}

		sleep(2);
	}

	close(client_socket);
	fclose(fp);
	free(file_data);

	pthread_join(thread_id,NULL);
	//return 0;


}
