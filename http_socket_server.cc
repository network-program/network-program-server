#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <string>
#include "include/server/network/http_protocol.h"

#define PACKET_SIZE 65535

class HttpServerSocket {
private:
	int listen_socket, accept_socket;
	struct sockaddr_in client_addr, server_addr;
	char *packet_buffer;

	void errquit(char* mesg) { perror(mesg); exit(0); }

	void create_socket(int host, int port) {
		// ���� ����
		if ((listen_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
			errquit("[Error]Socket creation failed.\n");

		// ���� Host �� Port ���� ����
		bzero((char*)&server_addr, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = htonl(host);
		server_addr.sin_port = htons(port);
	}

	void bind_socket() {
		// ���� binding
		if (bind(listen_socket, (struct sockaddr*)&server_addr, sizeof(s                                                                                                             erver_addr)) < 0)
			errquit("bind fail");
	}

public:
	HttpServerSocket() {
		this->packet_buffer = new char[PACKET_SIZE];
	}

	void open_socket(int host, int port) {
		create_socket(host, port);
		bind_socket();
	}

	void wait_client(int backlog) {
		// Ŭ���̾�Ʈ�κ��� �����û�� ��ٸ�
		listen(listen_socket, backlog);
		if (listen_socket == -1)
			errquit("tcp_listen fail");

		// Ŭ���̾�Ʈ�� �����û�� accept ��
		int addrlen = sizeof(client_addr);
		accept_socket = accept(listen_socket, (struct sockaddr*)&client_                                                                                                             addr, (socklen_t*)&addrlen);
	}

	void close_socket() {
		delete packet_buffer;
		close(listen_socket);
		close(accept_socket);
	}

	std::string recv_packet() { // ��Ŷ ����
		recv(this->accept_socket, this->packet_buffer, PACKET_SIZE, 0);
		std::string packet_str(this->packet_buffer);
		return packet_str;
	}

	void send_packet(char *packet) { // ��Ŷ ����
		send(this->accept_socket, packet, PACKET_SIZE, 0);
	}
};

int main(int argc, char *argv[]) {

	// �ùٸ� �Է��� �ƴ� ���� ���� ó��
	if (argc != 2) {
		printf("���� : %s [port]\n", argv[0]);
		exit(0);
	}

	// ���� ���� �� Ŭ���̾�Ʈ�� ����
	HttpServerSocket *http_socket = new HttpServerSocket();
	http_socket->open_socket(INADDR_ANY, atoi(argv[1]));

	while (1) {
		// Ŭ���̾�Ʈ�� ������ ��ٸ�
		http_socket->wait_client(5);

		// Request ��Ŷ�� ����
		std::string request = http_socket->recv_packet();

		// GET, POST ��� ��ȸ
		if (request.rfind("GET", 0) == 0)
			std::cout << "GET ��� �Դϴ�.\n";
		else if (request.rfind("POST", 0) == 0)
			std::cout << "POST ��� �Դϴ�.\n";
		else
			std::cout << "GET �Ǵ� POST ����� �ƴմϴ�.\n";

		// Request ��Ŷ ����캸��
		std::cout << request << "\n";

		// Response ��Ŷ�� ����� Ŭ���̾�Ʈ�� ����
		// ���׷� ���� �ӽ� �ּ� ó��
		/*char *response =
				"HTTP/1.1 403 FORBIDDEN\r\n"
				"Server: Apache\r\n"
				"Content-Type: text/html; charset=iso-8895-1\r\n"
				"Date: Sun, 6 Nov 2022 20:54:51 GMT\r\n"
				"\r\n"
				"<!DOCTYPE HTML PUBLIC \"-//IETF/DTD HTML 2.0//EN\">"
				"<h1>FORBIDDEN</h1>";
		http_socket->send_packet(response);*/
	}
	// ���� ����
	http_socket->close_socket();

	// �޸� ���� �� ����
	delete http_socket;
	exit(0);
}
