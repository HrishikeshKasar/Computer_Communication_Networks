#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <math.h>
#include <stdint.h>
#include<pthread.h>

#define MAX_NEIGHBOR_SIZE 1024

struct NeighborNodesInfo {
	char node_id;
	int port_no;
};


struct RoutingTable {
	int my_no_of_neighbors;
	int nodes_in_rt;
	char my_node_id;
	int my_port_no;
	char dest_node[MAX_NEIGHBOR_SIZE];
	char next_hop[MAX_NEIGHBOR_SIZE];
	int node_cost[MAX_NEIGHBOR_SIZE];
};

struct RoutingTable my_routing_table;
struct RoutingTable send_routing_table;
struct RoutingTable file_routing_table;
struct RoutingTable recvd_routing_table;
struct RoutingTable * Neighbor_Routing_Tables;
struct NeighborNodesInfo * neighbor_info_arr;
int no_of_neighbors = 0;
char input_filename[MAX_NEIGHBOR_SIZE] = "./";
char comm_success[MAX_NEIGHBOR_SIZE] = {0};
unsigned int output_number = 0;

/*
 * calculates port number on the basis of the name of the node.
 * It assumed that the nodes may have names from a to z
 * */
int calc_port_no(char node_id) {

	int looper = 0;
	int port_no = 10000;

	for(looper='a';looper<='z';looper++) {
		if(node_id == looper) {
			return port_no;
		}
		port_no++;
	}
}


/*
 * Checks for any link updates from the file
 * */
void UpdateNeighborDetails(void) {

	FILE *ip_fp;
	int looper  = 0;
	char * line = NULL;
	size_t len = 0;
	char  * cost;

	ip_fp = fopen(input_filename, "r");
	file_routing_table.my_node_id = input_filename[2];
	getline(&line,&len,ip_fp);
	no_of_neighbors = atoi(line);
	file_routing_table.my_no_of_neighbors = no_of_neighbors;
	for(looper=0;looper<no_of_neighbors;looper++) {
		getline(&line,&len,ip_fp);
		strtok_r(line," ",&cost);
		file_routing_table.node_cost[looper] = atoi(cost);
		file_routing_table.dest_node[looper] = *line;
		file_routing_table.next_hop[looper] = my_routing_table.dest_node[looper];
	}
	fclose(ip_fp);	

	for(looper=0;looper<no_of_neighbors;looper++) {
		if(file_routing_table.node_cost[looper] <= my_routing_table.node_cost[looper]) {
			my_routing_table.node_cost[looper] = file_routing_table.node_cost[looper];
			my_routing_table.dest_node[looper] = file_routing_table.dest_node[looper];
			my_routing_table.next_hop[looper] = file_routing_table.next_hop[looper];
		} else {
			if(my_routing_table.next_hop[looper] == file_routing_table.next_hop[looper]) {
				my_routing_table.node_cost[looper] = file_routing_table.node_cost[looper];
				my_routing_table.dest_node[looper] = file_routing_table.dest_node[looper];
				my_routing_table.next_hop[looper] = file_routing_table.next_hop[looper];
			}
		}	
	}
}



/*
 * pthread handler for sending data to neighbors
 */
void * SendRTToNeighbors(void * not_used) {

	int sock_fd;
	struct sockaddr_in sender_sock_addr;
	unsigned int fromSize = 0;
	int ret_recvfrom;
	int looper =0;
	int inner_looper =0;

	for(looper=0;looper<no_of_neighbors;looper++) {


		send_routing_table.my_no_of_neighbors = my_routing_table.my_no_of_neighbors;
		send_routing_table.nodes_in_rt = my_routing_table.nodes_in_rt;
		send_routing_table.my_node_id = my_routing_table.my_node_id;
		send_routing_table.my_port_no = my_routing_table.my_port_no;
		for(inner_looper=0;inner_looper<my_routing_table.nodes_in_rt;inner_looper++) {
			send_routing_table.dest_node[inner_looper] = my_routing_table.dest_node[inner_looper];
			send_routing_table.next_hop[inner_looper]  = my_routing_table.next_hop[inner_looper];
			send_routing_table.node_cost[inner_looper] = my_routing_table.node_cost[inner_looper];
			if(send_routing_table.next_hop[inner_looper] == neighbor_info_arr[looper].node_id) {
				send_routing_table.node_cost[inner_looper] = 65535;
			}
		}

		if ((sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
			printf ("socket() failed");
		}

		memset(&sender_sock_addr, 0, sizeof(sender_sock_addr));
		sender_sock_addr.sin_family = AF_INET;
		sender_sock_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
		sender_sock_addr.sin_port = htons(neighbor_info_arr[looper].port_no);

		if (sendto(sock_fd,&send_routing_table,
					(sizeof(send_routing_table)),0,
					(struct sockaddr *)&sender_sock_addr,
					sizeof(sender_sock_addr)) != (sizeof(send_routing_table))) {
			printf("sendto() sent a different number of bytes than expected");
		}
		close (sock_fd);
	}

}


void PrintMyRoutingTable(void) {
	int looper = 0;

	printf("\n================== Output Number %d =====================\n",output_number++);
	for(looper=0;looper<my_routing_table.nodes_in_rt;looper++) {
		printf("Shortest path %c-%c: the next hop is %c and the cost is %d\n",my_routing_table.my_node_id,
				my_routing_table.dest_node[looper],
				my_routing_table.next_hop[looper],
				my_routing_table.node_cost[looper]);
	}
}


/*
 * Updates Routing table based on the received routing tables and also takes care of the looping issue
 * */
void UpdateMyRoutingTable(void) {
	int main_looper = 0;
	int inner_looper = 0;
	int looper_1 = 0;
	int looper_2 = 0;
	char recvd_routing_array_id[MAX_NEIGHBOR_SIZE] = {0};
	char recvd_routing_array_v[MAX_NEIGHBOR_SIZE] = {0};
	char recvd_routing_array_nh[MAX_NEIGHBOR_SIZE] = {0};
	int nodes_my_rt = 0;
	int nodes_recvd_rt = 0;
	int no_of_nodes = 0;
	int matched_node_cost = 0;
	int cost_currnt_node = 0;
	char matched_next_hop = 0;

	nodes_my_rt = my_routing_table.my_no_of_neighbors;
	for(main_looper=0; main_looper<my_routing_table.my_no_of_neighbors; main_looper++) {
		if(comm_success[main_looper] == 1) {
			nodes_recvd_rt = Neighbor_Routing_Tables[main_looper].nodes_in_rt;

			for(inner_looper=0; inner_looper<nodes_recvd_rt; inner_looper++) {
				recvd_routing_array_id[inner_looper] = Neighbor_Routing_Tables[main_looper].dest_node[inner_looper];
				recvd_routing_array_nh[inner_looper] = 0;
				recvd_routing_array_v[inner_looper] = 0;
			}
			for(inner_looper=0;inner_looper<nodes_my_rt;inner_looper++) {
				if(my_routing_table.dest_node[inner_looper] == Neighbor_Routing_Tables[main_looper].my_node_id) {
					cost_currnt_node = my_routing_table.node_cost[inner_looper];
				}
			}
			no_of_nodes = 0;
			for(looper_1=0;looper_1<my_routing_table.nodes_in_rt;looper_1++) {
				for(looper_2=0;looper_2<nodes_recvd_rt;looper_2++) { 
					if(my_routing_table.dest_node[looper_1] == Neighbor_Routing_Tables[main_looper].dest_node[looper_2]) {
						matched_node_cost = Neighbor_Routing_Tables[main_looper].node_cost[looper_2] 
							+ cost_currnt_node;
						if(my_routing_table.next_hop[looper_1] == Neighbor_Routing_Tables[main_looper].my_node_id) {
							my_routing_table.node_cost[looper_1] = matched_node_cost;
						} else if(my_routing_table.node_cost[looper_1] > matched_node_cost) {
							my_routing_table.node_cost[looper_1] = matched_node_cost;
							my_routing_table.next_hop[looper_1] = 
								Neighbor_Routing_Tables[main_looper].my_node_id;
						}
						recvd_routing_array_v[looper_2] = 1;
					}
				}
				no_of_nodes++;
			}
			for(looper_1=0;looper_1<nodes_recvd_rt;looper_1++) {
				if((recvd_routing_array_v[looper_1] == 0)) {
					if(Neighbor_Routing_Tables[main_looper].dest_node[looper_1] != my_routing_table.my_node_id) {
						my_routing_table.dest_node[no_of_nodes] = Neighbor_Routing_Tables[main_looper].dest_node[looper_1];	
						my_routing_table.node_cost[no_of_nodes] = Neighbor_Routing_Tables[main_looper].node_cost[looper_1] 
							+ cost_currnt_node;	
						my_routing_table.next_hop[no_of_nodes] = Neighbor_Routing_Tables[main_looper].my_node_id;
						no_of_nodes++;	
					}
				}
			}
			my_routing_table.nodes_in_rt = no_of_nodes;
		}
	}
}

void ClearRecvdArray(void) {

	int looper = 0;
	int inner_looper = 0;

	for(looper=0;looper<my_routing_table.my_no_of_neighbors;looper++) {
		Neighbor_Routing_Tables[looper].my_no_of_neighbors = 0;
		Neighbor_Routing_Tables[looper].nodes_in_rt = 0;
		Neighbor_Routing_Tables[looper].my_node_id = 0;
		Neighbor_Routing_Tables[looper].my_port_no = 0;
		for(inner_looper=0;inner_looper<MAX_NEIGHBOR_SIZE;inner_looper++) {
			Neighbor_Routing_Tables[looper].dest_node[inner_looper] = 0; 
			Neighbor_Routing_Tables[looper].next_hop[inner_looper] = 0;
			Neighbor_Routing_Tables[looper].node_cost[inner_looper] = 0;
		}
	}
}

int main(int argc, char *argv[]) {

	if(argc < 2) {
		printf("\nusage: %s inputfile\n",argv[0]);
		return 0;
	}

	FILE *ip_fp;
	char * neighbor_cost_file[128] = {0};
	int looper  = 0;
	char * line = NULL;
	size_t len = 0;
	char  * cost;
	struct sockaddr_in recvr_socket, sender_socket;
	int socket_fd,recv_len, slen = sizeof(sender_socket);
	pthread_t sender_thread_id;

	//============================================Initialization======================================================================
	strcat(input_filename, argv[1]);
	ip_fp = fopen(input_filename, "r");
	my_routing_table.my_node_id = input_filename[2];
	getline(&line,&len,ip_fp);
	no_of_neighbors = atoi(line);
	my_routing_table.my_no_of_neighbors = no_of_neighbors;
	my_routing_table.nodes_in_rt = no_of_neighbors;
	for(looper=0;looper<no_of_neighbors;looper++) {
		getline(&line,&len,ip_fp);
		strtok_r(line," ",&cost);
		my_routing_table.node_cost[looper] = atoi(cost);
		my_routing_table.dest_node[looper] = *line;
		my_routing_table.next_hop[looper] = my_routing_table.dest_node[looper];
	}
	my_routing_table.my_port_no = calc_port_no(my_routing_table.my_node_id);

	neighbor_info_arr = (struct NeighborNodesInfo *)malloc(no_of_neighbors * sizeof(neighbor_info_arr));
	for(looper=0;looper<no_of_neighbors;looper++) {
		neighbor_info_arr[looper].node_id = my_routing_table.dest_node[looper];
		neighbor_info_arr[looper].port_no = calc_port_no(neighbor_info_arr[looper].node_id);
	}
	printf("my node id : %c\nmy port no = %d\n",my_routing_table.my_node_id,my_routing_table.my_port_no);
	printf("my neighbors and their port numbers:\n");
	for(looper=0;looper<no_of_neighbors;looper++) {
		printf("%c -> %d\n",neighbor_info_arr[looper].node_id,neighbor_info_arr[looper].port_no);
	}
	Neighbor_Routing_Tables = (struct RoutingTable *)malloc(no_of_neighbors * sizeof(my_routing_table));
	fclose(ip_fp);	
	//===============================================================================================================================

	//================================================UDP Socket Calls=============================================================
	if ((socket_fd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		printf("socket");
		return 0;
	}

	memset((char *)&recvr_socket, 0, sizeof(recvr_socket));
	recvr_socket.sin_family = AF_INET;
	recvr_socket.sin_port = htons(my_routing_table.my_port_no);
	recvr_socket.sin_addr.s_addr = htonl(INADDR_ANY);

	struct timeval read_timeout;
	read_timeout.tv_sec = 1;
	read_timeout.tv_usec = 0;
	setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);

	if(bind(socket_fd, (struct sockaddr*)&recvr_socket, sizeof(recvr_socket)) == -1) {
		printf("bind");
		return 0;
	}

	while(1)
	{
		if( pthread_create( &sender_thread_id , NULL ,  SendRTToNeighbors , NULL ) < 0) {
			printf("could not create sender thread");
			return 0;
		}

		for(looper=0;looper<my_routing_table.my_no_of_neighbors;looper++) {
			if ((recv_len = recvfrom(socket_fd, &Neighbor_Routing_Tables[looper], sizeof(recvd_routing_table), 0,
							(struct sockaddr *)&sender_socket, &slen)) == -1) {
				//printf("Failed to communicate with %c\n",neighbor_info_arr[looper].node_id);
				comm_success[looper] = 0;
			} else {
				comm_success[looper] = 1;
			}

		}
		UpdateNeighborDetails();
		UpdateMyRoutingTable();
		PrintMyRoutingTable();
		ClearRecvdArray();
		sleep(2);
	}

	close(socket_fd);

	return 0;
}
