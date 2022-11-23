/*
 * version 1.0
 *
 * created 2022-11-01
 * 
 * By YIBEOMSEOK
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#include <iostream>

#define WEBSERVER_PORT 3000

struct stat_socket{
  /* default */
  int BUF_SIZE;
  int MAX_CLIENT;
  int client_cnt;
  int client_socks[256];

  /* not default */
  int server_sock, client_sock;
  struct sockaddr_in server_addr, client_addr;
  int client_addr_size;
  pthread_t t_id;

  pthread_mutex_t mutx;
} stat_socket = {100, 256, 0};

struct stat_socket sock;

void sock_init(int port_number);
void sock_accept();
void error_handling(char *msg);

void sock_init(int port_number) {
  
  pthread_mutex_init(&sock.mutx, NULL);
  sock.server_sock = socket(AF_INET, SOCK_STREAM, 0);

  memset(&sock.server_addr, 0, sizeof(&sock.server_addr));
  sock.server_addr.sin_family = AF_INET;
	sock.server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	sock.server_addr.sin_port = htons(port_number);

  printf("%s %d\n",inet_ntoa(sock.server_addr.sin_addr), port_number);

  if (bind(sock.server_sock, (struct sockaddr*)&(sock.server_addr), sizeof(sock.server_addr)) == -1)
    error_handling("bind error");
  if (listen(sock.server_sock, sock.MAX_CLIENT) == -1)
    error_handling("listen error");
}

void sock_accept(const char* ip_address, int port) {
  std::cout << "Accept socket " << ip_address << ":" << port << '\n';

  sock.client_addr.sin_addr.s_addr = inet_addr(ip_address);
  sock.client_addr.sin_port = port;
  sock.client_addr_size = sizeof(sock.client_addr);
  socklen_t client_addr_size = sizeof(sock.client_addr);
  sock.client_sock = accept(sock.server_sock, (struct sockaddr*)&sock.client_addr, &client_addr_size);

  sock.client_socks[sock.client_cnt++] = sock.client_sock;
}

void error_handling(char *msg) {
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}






/* 아래 함수들은 구현중. 사용 금지 */

/*

void *sock_handle_client(void *arg) {
  int client_sock = *((int*)arg);
  int str_len = 0;
  int i;
  //char msg[]

  return NULL;
}

void sock_mutex_lock() {
  pthread_mutex_lock(&sock.mutx);
}

void sock_mutex_unlock() {
  pthread_mutex_unlock(&sock.mutx);
}


void sock_send_msg(char *msg, int len) {

}

// 사용자가 구현해서 넣어야 할 함수 형식
void *todofunc(void *arg) { // 반드시 지켜져야 함
  int client_sock = *((int*)arg); // 현재 할당된 client socket을 알고자 하면 입력
  
  // 아래부터는 데이터 읽는 방식. 사용자마다 서로 다를 수 있다.
  int str_len = 0, i;             // 데이터 읽는 법 
  char msg[sock.BUF_SIZE];
  
  while((str_len = read(client_sock, msg, sizeof(msg))) != 0) {
    // read한 protocol format 분석
    // read한 것을 바탕으로 할 일
  }

}

void sock_pthread_create(void (*p_todofunc)()) {
  pthread_create(&sock.t_id, NULL, p_todofunc, (void*)&sock.client_addr);
}

*/
