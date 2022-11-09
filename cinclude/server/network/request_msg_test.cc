/*
 * for HTTP protocol communication server
 * Created by Yi BeomSeok
*/

#include "./socket.h"
#include "./http_protocol.h"
#include <iostream>

using namespace std;

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
	char buf[BUFSIZ];
  
	if(read(client_sock, buf, BUFSIZ) < 0) {
    perror("state code 500");
  }
	printf("%s\n", buf); // TEST

	network::HTTPProtocol parser;
	parser.parse(buf);


  cout << "http version = " << parser.http_version() << '\n';
  cout << "status code = " << parser.status_code() << '\n';
  cout << "status_test = " << parser.status_text() << '\n';

	string response = 
			"HTTP/1.1 403 FORBIDDEN\r\n"
      "Server: Apache\r\n"
      "Content-Type: text/html; charset=iso-8895-1\r\n"
      "Date: Sun, 6 Nov 2022 20:54:51 GMT\r\n"
      "\r\n";

	send_msg(response);

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

void send_msg(const std::string& msg) {
  int i;

  pthread_mutex_lock(&sock.mutx);
  for(i = 0; i < sock.client_cnt; i++) {
    write(sock.client_socks[i], msg.c_str(), msg.size());
  }
  pthread_mutex_unlock(&sock.mutx);
}

void request_post(char* uri) {
	// uri가 뭔지 파악

	// post의 경우 뭘 하라는 건지 파악하고

	// 성공했다고 response 보내야 함
}

void request_get(char* uri) {
	// uri가 뭔지 파악

	// 원하는 것을 파악하고 response로 그것을 줘야함

	// 근데 json 형식으로 줘야 함
}