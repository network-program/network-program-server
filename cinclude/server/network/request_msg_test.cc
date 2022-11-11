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
#include <mutex>

#include "socket.h"

#include "server/network/http_protocol.h"

#include "json/json.h"

using namespace std;

extern struct stat_socket sock;

void send_msg(const std::string& msg, int client);
void *handle_client(void *arg);

void signal_handler(int sig) {
	std::cout << "Signal " << sig << '\n';
	// if (sig != EXIT_SUCCESS) {
		close(sock.server_sock);
		exit(0);
	// }
}

std::map<unsigned long long, std::pair<std::string, std::string>> message_history;
std::mutex history_m;

int main(int argc, char *argv[]) {
  sock_init(atoi(argv[1]));

	signal(SIGINT, signal_handler);

	message_history.emplace(10, std::make_pair("James", "Hi"));
	message_history.emplace(20, std::make_pair("Nana", "Hi to you too"));

	// int x = 1;
	// handle_client(&x);
	
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
	std::cout << __func__ << '\n';
	int client_sock = *((int*)arg);
	int str_len = 0, i;
	std::string buf(100'000, '\0');
	// buf = "POST /user HTTP/1.1\r\n"
	// 			"\r\n"
	// 			"{\"name\": \"이민호\", \"chat\": \"안녕하세요\"}";
	// buf = "GET / HTTP/1.1\r\n"
				// "From-Time: 3\r\n"
				// "\r\n";
  
	if(read(client_sock, buf.data(), buf.size()) < 0) {
	// if (false) {
    perror("state code 500\n");
  } else {
		do {
			printf("HTTP Request:\n%s\n", buf.data()); // TEST

			network::HTTPProtocol parser;
			const auto b = parser.parse(buf);
			if (!b) {
				std::cerr << "Failed to parse HTTP request!\n";
				break;
			}

			std::cout << "Request type: " << parser.http_method() << '\n';

			if (const auto& method = parser.http_method(); method == "POST") {
				std::cout << "Content: " << parser.content() << '\n';
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
				
				{
					std::lock_guard lck(history_m);
					message_history.emplace(t, std::make_pair(name, chat));
				}

				string response = 
						"HTTP/1.1 200 OK\r\n"
						"\r\n";
				send_msg(response, client_sock);
			} else if (method == "GET") {
    		const auto& header = parser.header();
				const auto it = header.find("from_time");
				if (it == header.end()) {
					std::cerr << "Header " << "from_time" << " Not found!\n";

					std::string res = 
					"HTTP/1.1 200 OK\r\n"
					"Server: Apache\r\n"
					"Date: Sun, 6 Nov 2022 20:54:51 GMT\r\n"
					"\r\n";
					// "["
						// "{\"name\":\"이범석\", \"chatKey\":\"안녕못해요\"},"
						// "{\"name\":\"이민호\", \"chatKey\":\"나도안녕못해\"},"
						// "{\"name\":\"이용규\", \"chatKey\":\"나도마찬가지\"}"
					// "]";
					send_msg(res, client_sock);
				} else {
					std::cout << "from_time: " << it->second << '\n';
					const auto t = std::stoll(it->second);

					std::unique_lock lck(history_m);
					auto lb = message_history.lower_bound(t);

					std::string res = 
					"HTTP/1.1 200 OK\r\n"
					"Server: Apache\r\n"
					"Date: Sun, 6 Nov 2022 20:54:51 GMT\r\n"
					"\r\n"
					"[";

					if (lb != message_history.end()) {
						res += "{\"name\":\"" + lb->second.first + "\",\"chatKey\":\"" + lb->second.second + "\"}";
					}
					++lb;

					while (lb != message_history.end()) {
						res += ",{\"name\":\"" + lb->second.first + "\",\"chatKey\":\"" + lb->second.second + "\"}";
						++lb;
					}
					lck.unlock();
					
					res += "]";

					send_msg(res, client_sock);
				}
			}

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

void send_msg(const std::string& msg, int client_sock) {
	std::cout << "Sending Response to " << client_sock << ": \n";
	std::cout << msg << "\n\n";
  int i;

  pthread_mutex_lock(&sock.mutx);
  // for(i = 0; i < sock.client_cnt; i++) {
    write(client_sock, msg.c_str(), msg.size());
  // }
  pthread_mutex_unlock(&sock.mutx);
}
