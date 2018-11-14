/*
 * Publisher.cpp
 *
 *  Created on: Nov 14, 2018
 *      Author: jinbro
 */

/*
 * Subscriber.cpp
 *
 *  Created on: Nov 14, 2018
 *      Author: jinbro
 */
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include "json/json.h"
#include <string>
#include <string.h>
#include <vector>

using namespace std;

//함수 정의
int getChildPort(string buf, int pos);
void error_handling(string msg);
int getChildNum(string buf);
struct sockaddr_in* returnSockAddr(int port);
//전역변수 정의
#define EPOLL_SIZE 256
#define BUF_SIZE 100

vector<int> sub_roots;

int main(int argc, char *argv[]) {

	vector<int> childsockets;

	Json::Value info;
	info["type"] = "pub";
	Json::FastWriter writer;

	struct epoll_event *ep_events;
	struct epoll_event event;
	int epfd, event_cnt;

	string pubinfo = writer.write(info);
	cout << pubinfo << endl;
	const char *buf = pubinfo.c_str();
	char char_port[BUF_SIZE];
	int str_len;
	int main_serv_sock;

	struct sockaddr_in* main_serv_addr;

	socklen_t addr_size;

	if (argc != 3) {
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}

	//Create Epoll
	epfd = epoll_create(EPOLL_SIZE);
	ep_events = (struct epoll_event*) malloc(
			sizeof(struct epoll_event) * EPOLL_SIZE);

	//For Connection To the Main Server - Initializing
	main_serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	main_serv_addr = returnSockAddr(atoi(argv[2]));
	event.events = EPOLLIN;
	event.data.fd = main_serv_sock;
	epoll_ctl(epfd, EPOLL_CTL_ADD, main_serv_sock, &event);

	//Connect to the main Server
	if (connect(main_serv_sock, (struct sockaddr*) main_serv_addr,
			sizeof(*main_serv_addr)) == -1)
		error_handling("connect() error!");
	str_len = write(main_serv_sock, buf, BUF_SIZE);

	//THREAD 로 처리 해야하나? 하나는 Server로부터 Sub에 대한 정보를 받고, 하나는 받은 Socket에게 정보를 전달?
	while (1) {
		char buf[BUF_SIZE];
		event_cnt = epoll_wait(epfd, ep_events, EPOLL_SIZE, -1);
		if (event_cnt == -1) {
			puts("epoll wait() error");
		}

		for (int i = 0; i < event_cnt; i++) {
			//서버로부터 자식들에 대한 정보를 받고 연결
			if (ep_events[i].data.fd == main_serv_sock) {
				char buf[BUF_SIZE];
				int strlen = read(main_serv_sock, buf, BUF_SIZE);
				sub_roots.clear();
				for (int idx = 0; idx < getChildNum(buf); idx++) {

					int child_serv_sock = socket(PF_INET, SOCK_STREAM, 0);
					struct sockaddr_in* child_serv_addr = returnSockAddr(
							getChildPort(buf, idx));
					event.events = EPOLLIN;
					event.data.fd = child_serv_sock;
					epoll_ctl(epfd, EPOLL_CTL_ADD, child_serv_sock, &event);
					//Connect to child Server
					if (connect(child_serv_sock,
							(struct sockaddr*) child_serv_addr,
							sizeof(*child_serv_addr)) == -1)
						error_handling("connect() error!");
					sub_roots.push_back(child_serv_sock);
				}//FOR서
				//일단은 이 부분에서 큰 크기의 파일을 전송해보도록 하자 JSON 형식으로 attr 추가해서
			}//IF SERV
		}//FOR
	}//WHILE

	close(main_serv_sock);
	return 0;
}

struct sockaddr_in* returnSockAddr(int port) {
	struct sockaddr_in* sockaddr = (struct sockaddr_in*) malloc(
			sizeof(sockaddr_in));
	memset(sockaddr, 0, sizeof(*sockaddr));
	sockaddr->sin_family = AF_INET;
	sockaddr->sin_addr.s_addr = htonl(INADDR_ANY);
	sockaddr->sin_port = htons(port);
	return sockaddr;
}

int getChildPort(string buf, int pos) {
	Json::Value array;
	Json::Value port;
	Json::Reader reader;
	reader.parse(buf, array);
	port = array[pos];
	return port.asInt();
}
int getChildNum(string buf) {
	Json::Value array;
	Json::Reader reader;
	reader.parse(buf, array);
	return array.size();
}

void error_handling(string msg) {
	cout << msg << endl;
	exit(1);
}



