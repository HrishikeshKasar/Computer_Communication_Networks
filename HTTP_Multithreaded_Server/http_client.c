#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <netdb.h>




int main(int argc, char *argv[]) {

	if(argc < 5) {
		printf("\nUsage :- %s hostname port command resource.\n",argv[0]);
		return 0;
	}

	char response[4096];
	char request[1024];
	struct hostent *server;
	struct sockaddr_in remote_address;
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

	server = gethostbyname(hostname);
	if(server == NULL) {
		printf("Error no such host");
	}

	bzero((char *)&remote_address, sizeof(remote_address));
	remote_address.sin_family = AF_INET;
	bcopy((char *) server->h_addr, (char *)&remote_address.sin_addr.s_addr, server->h_length);
	remote_address.sin_port = htons(port);


	connect(client_socket, (struct sockaddr *) &remote_address, sizeof(remote_address));

	sprintf(request,"%s %s HTTP/1.1\r\nHost: %s\r\n\r\n",command,file_path,hostname);
	
	fp = fopen("./put_data.txt", "r");
	fseek(fp,0, SEEK_END);
	long fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char * file_data = malloc(fsize +1);
	fread(file_data, fsize,1 , fp);
	file_data[fsize] = 0;
	strcat(request, file_data);

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
	close(client_socket);
	fclose(fp);
	free(file_data);

	return 0;


}
