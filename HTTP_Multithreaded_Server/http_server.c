#include<stdio.h>
#include<string.h> 
#include<stdlib.h> 
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<pthread.h>
#include<fcntl.h>

void *connection_handler(void *);

int shared_var = 1;
int quit_var = 1;
int first_thread = 0;

void * quit_handler(void *socket_fd) {

	char a;
	int quit_thread = 1;
	printf("\nquit_handler started\n");
	while(quit_thread) {
		scanf("%c",&a);
		if(a == 'Q') {
			quit_var = 0;
			quit_thread = 0;
		}
	}
	pthread_exit(0);
}




int main(int argc , char *argv[])
{
	if(argc < 2) {
		printf("usage %s port",argv[0]);
		return 0;
	}

	int port = atoi(argv[1]);
	printf("port :- %d\n",port);

	int socket_desc, client_sock, struct_size;
	struct sockaddr_in server, client;

	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1) {
		printf("Could not create socket\n");
	}
	printf("\nSocket created");

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(port);

	if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0) {
		printf("bind failed\n");
		return 0;
	}
	printf("bind done\n");

	listen(socket_desc , 5);

	printf("\n==============================================================================================");
	printf("\n					Server Started						  ");
	printf("\n				      Waiting for clients					  ");
	printf("\n				Press Q and ENTER to shutdown server 				  ");
	printf("\n==============================================================================================\n");
	struct_size = sizeof(struct sockaddr_in);

	pthread_t thread_id;
	pthread_t thread_id2;

	if( pthread_create( &thread_id2 , NULL ,  quit_handler , NULL ) < 0) {
		printf("could not create thread");
		return 0;
	}
	printf("\npthread created for quit_handler");
	
	fcntl(socket_desc, F_SETFL, fcntl(socket_desc, F_GETFL, 0) | O_NONBLOCK);
	while(quit_var) {
		sleep(1);
		//printf("Inside while 1\n");
		client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&struct_size);
		if(client_sock > 0) {
			printf("Client connection accepted\n");

			if( pthread_create( &thread_id , NULL ,  connection_handler , (void*) &client_sock) < 0) {
				printf("Error in pthread_create\n");
				return 0;
			}
			if(first_thread == 0) {
				//do nothing
				first_thread++;
			} else {
				shared_var++;
			}
			printf("Client Service Thread created\n");
		}
	}

	if(first_thread) {
		while(shared_var) {
			sleep(1);
			printf("\n====================================Waiting for all threads to close=========================================\n");
		}
	}
	printf("\n===========================================All service threads closed==========================================");
	printf("\n===========================================Server Shutting Down================================================\n");
	close(socket_desc);

	return 0;
}

void GET_response(int socket_fd, char * request) {

	FILE *fp;
	char dot_operator[50] = ".";
	char * ptr;
	char * ptr2;
	char * ptr3;
	char * ptr4;
	char * cmd;
	char * file_path;
	char request_copy[128];
	char http_header_OK[4096] = "HTTP/1.1 200 OK\r\n\n";
	char http_header_404[1024] = "HTTP/1.1 404 Not Found\r\n\n";
	char response_data[4096];

	strcpy(request_copy, request);
	strtok_r (request_copy, " ", &ptr);
	strtok_r (ptr, " ", &ptr2);
	strtok_r (ptr2, "\r\n", &ptr3);
	strtok_r (ptr3, "\r\n", &ptr4);
	strcat(dot_operator, ptr);
	printf("%s",dot_operator);

	if( access(dot_operator, F_OK ) != -1 ) {
		fp = fopen(dot_operator, "r");
		fseek(fp,0, SEEK_END);
		long fsize = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		char * file_data = malloc(fsize +1);
		fread(file_data, fsize,1 , fp);
		file_data[fsize] = 0;
		strcat(http_header_OK, file_data);
		send(socket_fd, http_header_OK, sizeof(http_header_OK), 0);
		fclose(fp);
		free(file_data);
	} else {
		printf("\nNo such file\n");
		sprintf(response_data, "No such File\n");
		strcat(http_header_404,response_data);
		send(socket_fd, http_header_404, sizeof(http_header_404), 0);
	}
}

void PUT_response(int socket_fd,char * request) {

	FILE *fp;
	char dot_operator[50] = ".";
	char * ptr;
	char * ptr2;
	char * ptr3;
	char * ptr4;
	char * cmd;
	char * file_path;
	char request_copy[128];
	char http_header_OK[4096] = "HTTP/1.1 200 OK\r\n\n";
	char response_data[1024] = "File Created";

	strcpy(request_copy, request);
	strtok_r (request_copy, " ", &ptr);
	strtok_r (ptr, " ", &ptr2);
	strtok_r (ptr2, "\r\n", &ptr3);
	strtok_r (ptr3, "\r\n", &ptr4);
	strcat(dot_operator, ptr);
	printf("%s",dot_operator);
	fp = fopen(dot_operator, "w+");
	fprintf(fp, &ptr4[3]);
	fclose(fp);
	strcat(http_header_OK, response_data);
	send(socket_fd, http_header_OK, sizeof(http_header_OK), 0);
}



void *connection_handler(void *socket_desc)
{
	int socket_fd = *(int*)socket_desc;
	int read_size;
	char *message , client_message[2000];
	char response_data[1024] = "Invalid Command\n";
	char request[2048];
	char http_header_400[1024] = "HTTP/1.1 400 Bad Request\r\n\n";
	FILE *html_data;
	char request_copy[128];
	char * ptr;


	while((read_size = recv(socket_fd , request , 2000 , 0)) > 0 )  {


		strcpy(request_copy, request);
		strtok_r (request_copy, " ", &ptr);

		if(!strcmp(request_copy,"GET")) {
			printf("\nReceived GET command\n");
			GET_response(socket_fd,request);
		} else if(!strcmp(request_copy,"PUT")){
			printf("\nReceived PUT command\n");
			PUT_response(socket_fd,request);
		} else {
			strcat(http_header_400,response_data);
			send(socket_fd, http_header_400, sizeof(http_header_400), 0);
			printf("\nInvalid Command\n");
		}
	}

	if(read_size == 0) {
		puts("\nClient disconnected\n");
		fflush(stdout);
	} else if(read_size == -1) {
		printf("Receive failed\n");
	}

	printf("\n====================Thread returning =================\n");
	shared_var--;
	close(socket_fd);
	pthread_exit(0);
}

