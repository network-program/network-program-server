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
#include <chrono>
#include "include/server/network/http_protocol.h"
#define PACKET_SIZE 65535

class HttpServerSocket {
private:
	int listen_socket, accept_socket;
	struct sockaddr_in client_addr, server_addr;
	char* packet_buffer;

	void errquit(char* mesg) { perror(mesg); exit(0); }

	void create_socket(int host, int port) {
		// 소켓 생성
		if ((listen_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
			errquit("[Error]Socket creation failed.\n");

		// 소켓 Host 및 Port 정보 설정
		bzero((char*)&server_addr, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = htonl(host);
		server_addr.sin_port = htons(port);
	}

	void bind_socket() {
		// 소켓 binding
		if (bind(listen_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
			errquit("bind fail");
	}

	void wait_client(int backlog) {
		// 클라이언트로부터 연결요청을 기다림
		listen(listen_socket, backlog);
		if (listen_socket == -1)
			errquit("tcp_listen fail");
	}

public:
	HttpServerSocket() {
		this->packet_buffer = new char[PACKET_SIZE];
	}

	void open_socket(int host, int port, int backlog) {
		create_socket(host, port);
		bind_socket();
		wait_client(backlog);
	}

	void connect_client() {
		// 클라이언트의 연결요청을 accept 함
		int addrlen = sizeof(client_addr);
		accept_socket = accept(listen_socket, (struct sockaddr*)&client_addr, (socklen_t*)&addrlen);
	}

	void close_accpet_socket() {
		close(accept_socket);
	}

	void close_listen_socket() {
		delete packet_buffer;
		close(listen_socket);
	}

	std::string recv_packet() { // 패킷 수신
		recv(this->accept_socket, this->packet_buffer, PACKET_SIZE, 0);
		std::string packet_str(this->packet_buffer);
		return packet_str;
	}

	void send_packet(char* packet) { // 패킷 전송
		send(this->accept_socket, packet, std::string(packet).length() + 1, 0);
	}
};

int main(int argc, char* argv[]) {

	// 올바른 입력이 아닐 때에 대한 처리
	if (argc != 2) {
		printf("사용법 : %s [port]\n", argv[0]);
		exit(0);
	}

	// 소켓 생성 및 클라이언트의 연결 대기
	HttpServerSocket* http_socket = new HttpServerSocket();
	http_socket->open_socket(INADDR_ANY, atoi(argv[1]), 5);

	while (1) {
		// 클라이언트와 연결
		http_socket->connect_client();

		// Request 패킷을 받음
		std::string request = http_socket->recv_packet();

		// 받은 Request 패킷을 분석함
		network::HTTPProtocol parser;
		if (!parser.parse(request)) {
			std::cout << "[Error]Request parsing failed.\n";
			http_socket->close_accpet_socket();
			continue;
		}
		std::string http_method = parser.http_method();
		std::string request_target = parser.request_target();
		std::string http_version = parser.http_version();
		std::string content = parser.content();

		std::cout << "[Success]" << request << "\n";

		// Response 패킷을 만듬
		// 어떤식으로 Response 패킷을 만들지에 대한 로직이 필요. 아래는 임시 로직임
		network::HTTPProtocol protocol;
		if (http_method == "GET") {
			auto now = std::chrono::system_clock::now();
			std::time_t end_time = std::chrono::system_clock::to_time_t(now);
			std::string temp_content = "{\"name\":\"test\", \"value\":\"GET response data\"}";
			protocol.response(200, "OK");
			protocol.add_header("Date", std::ctime(&end_time));
			protocol.add_header("Content-Length", std::to_string(temp_content.length()));
			protocol.add_header("Content-Type", "text/html; charset=UTF-8");
			protocol.set_content(temp_content);
		}
		else { // POST나 그 외의 방식일 경우
			auto now = std::chrono::system_clock::now();
			std::time_t end_time = std::chrono::system_clock::to_time_t(now);
			std::string temp_content = "{\"name\":\"test\", \"value\":\"POST response data\"}";
			protocol.response(200, "OK");
			protocol.add_header("Date", std::ctime(&end_time));
			protocol.add_header("Content-Length", std::to_string(temp_content.length()));
			protocol.add_header("Content-Type", "text/html; charset=UTF-8");
			protocol.set_content(temp_content);
		}

		// Response 패킷을 char* 형태로 변환
		auto generator = protocol.build();
		auto packet = generator.GenerateNext()->to_string();
		std::vector<char> writable(packet.begin(), packet.end());
		writable.push_back('\0');
		char* response_packet = &writable[0];

		// 만든 Response 패킷을 클라이언트로 전송
		http_socket->send_packet(response_packet);

		// 클라이언트와 연결되어있는 소켓 닫음
		http_socket->close_accpet_socket();
	}
	// 클라이언트의 연결을 기다리고 있는 소켓 닫음
	http_socket->close_listen_socket();

	// 메모리 해제 및 종료
	delete http_socket;
	exit(0);
}
