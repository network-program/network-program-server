#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUF_SIZE 100
#define MAX_CLIENT 256

void *handle_client(void *arg);
void send_msg(char *msg, int len);
void error_handling(char *msg);

int client_cnt = 0;
int client_socks[MAX_CLIENT];
pthread_mutex_t mutx;

int main(int argc, char *argv[]) {
	
	int server_sock, client_sock;
	struct sockaddr_in server_addr, client_addr;
	int client_addr_size;
	pthread_t t_id;
	
	if(argc < 2) {
		printf("Input PORT NUMBER\n");
		exit(1);
	}	

	pthread_mutex_init(&mutx, NULL);
	server_sock = socket(AF_INET, SOCK_STREAM, 0);

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(atoi(argv[1]));

	if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
		error_handling("bind error");
	if (listen(server_sock, MAX_CLIENT) == -1)
		error_handling("listen error");
	
	while(1) {
		client_addr_size = sizeof(client_addr);
		client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_size);

		pthread_mutex_lock(&mutx);
		client_socks[client_cnt++] = client_sock;
		pthread_mutex_unlock(&mutx);
		
		pthread_create(&t_id, NULL, handle_client, (void*)&client_sock);
		pthread_detach(t_id);
		printf("Connected client IP: %s \n", inet_ntoa(client_addr.sin_addr));
	}
	close(server_sock);

	return 0;
}

void *handle_client(void *arg) {
	int client_sock = *((int*)arg);
	int str_len = 0, i;
	char msg[BUF_SIZE];

	while((str_len=read(client_sock, msg, sizeof(msg))) != 0) {
		send_msg(msg, str_len);
	}

	pthread_mutex_lock(&mutx);
	for(i = 0; i < client_cnt; i++) {
		if (client_sock == client_socks[i]) {
			while(i++ < client_cnt - 1)
				client_socks[i] = client_socks[i+1];
			break;
		}
	}

	client_cnt--;
	pthread_mutex_unlock(&mutx);
	close(client_sock);
	
	return NULL;
}

void send_msg(char *msg, int len) {
	int i;
	pthread_mutex_lock(&mutx);
	for(i = 0; i < client_cnt; i++) {
		write(client_socks[i], msg, len);
	}

	pthread_mutex_unlock(&mutx);
}
void error_handling(char *msg) {
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}

