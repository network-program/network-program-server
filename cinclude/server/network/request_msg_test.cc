/*
 * for HTTP protocol communication server
 * Created by Yi BeomSeok
*/

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>

#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <map>
#include <chrono>

#include "socket.h"

#include "server/network/http_protocol.h"

#include "json/json.h"

using namespace std;

extern struct stat_socket sock;

void send_msg(const std::string& msg);
void *handle_client(void *arg);

void signal_handler(int sig) {
	std::cout << "Signal " << sig << '\n';
	// if (sig != EXIT_SUCCESS) {
		close(sock.server_sock);
		exit(0);
	// }
}

std::map<unsigned long long, std::pair<std::string, std::string>> message_history;

int main(int argc, char *argv[]) {
  sock_init(atoi(argv[1]));

	signal(SIGINT, signal_handler);
	
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

using key_value = std::unordered_map<std::string, std::string>;

key_value ParseJson(const std::string& data) {

}

void *handle_client(void *arg) {
	std::cout << __func__ << '\n';
	int client_sock = *((int*)arg);
	int str_len = 0, i;
	char buf[100'000] = {};
  
	if(read(client_sock, buf, 100'000) < 0) {
    perror("state code 500\n");
  } else {
		do {
			printf("HTTP Request:\n%s\n", buf); // TEST

			network::HTTPProtocol parser;
			const auto b = parser.parse(buf);
			if (!b) {
				std::cerr << "Failed to parse HTTP request!\n";
				break;
			}

			Json::Value root;
			Json::Reader reader;
			const auto success = reader.parse(parser.content(), root);
			if (!success) {
				std::cerr << "Failed to parse!\n";
				break;
			}

			const auto now = std::chrono::system_clock::now();
			const auto t = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

			const auto name = root["name"].asString();
			const auto chat = root["chat"].asString();

			std::cout << "name: " << name << '\n';
			std::cout << "chat: " << chat << '\n';

			message_history.emplace(t, std::make_pair(name, chat));

			std::cout << "Content: " << parser.content() << '\n';

			string response = 
					"HTTP/1.1 200 OK\r\n"
					"Server: Apache\r\n"
					"Date: Sun, 6 Nov 2022 20:54:51 GMT\r\n"
					"\r\n"
					"[{\"name\":\"이범석\",\"chat\":\"응안녕못해\"},"
					 "{\"name\":\"이용규\",\"chat\":\"난너가싫어\"}]";
			std::cout << "\n\nSending Response:\n\n" << response << '\n';

			send_msg(response);
		} while (false);
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
