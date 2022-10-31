#include "./socket.h"

extern struct stat_socket sock;

void send_msg(char *msg, int len);
void *handle_client(void *arg);

int main(int argc, char *argv[]) {
  sock_init(atoi(argv[1]));
	
  while (1)
  {
		sock_accept();
		
    pthread_create(&sock.t_id, NULL, handle_client, (void*)&sock.client_sock);
		pthread_detach(sock.t_id);
		printf("Connected client IP: %s \n", inet_ntoa(sock.client_addr.sin_addr));
	}
	close(sock.server_sock);

	return 0;
}

void *handle_client(void *arg) {
	int client_sock = *((int*)arg);
	int str_len = 0, i;
	char msg[sock.BUF_SIZE];

	while((str_len=read(client_sock, msg, sizeof(msg))) != 0) {
		send_msg(msg, str_len);
	}

	pthread_mutex_lock(&sock.mutx);
	for(i = 0; i < sock.client_cnt; i++) {
		if (client_sock == sock.client_socks[i]) {
			while(i++ < sock.client_cnt - 1)
				sock.client_socks[i] = sock.client_socks[i+1];
			break;
		}
	}

	sock.client_cnt--;
	pthread_mutex_unlock(&sock.mutx);
	close(client_sock);
	
	return NULL;
}

void send_msg(char *msg, int len) {
	int i;
	pthread_mutex_lock(&sock.mutx);
	for(i = 0; i < sock.client_cnt; i++) {
		write(sock.client_socks[i], msg, len);
	}

	pthread_mutex_unlock(&sock.mutx);
}