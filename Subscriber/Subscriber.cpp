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
int returnPort(string buf);
void error_handling(string msg);
int getChildNum(string buf);
struct sockaddr_in* returnSockAddr(int port);
//전역변수 정의
#define EPOLL_SIZE 256
#define BUF_SIZE 100

int main(int argc, char *argv[]) {

	vector<int> childsockets;

	Json::Value info;
	string str;
	cout << "Write Attr" << endl;
	cin >> str;
	info["attr"] = str;
	info["type"] = "sub";
	Json::FastWriter writer;

	struct epoll_event *ep_events;
	struct epoll_event event;
	int epfd, event_cnt;

	string attr = writer.write(info);
	cout << attr << endl;
	const char *buf = attr.c_str();
	char char_port[BUF_SIZE];
	int str_len;
	int main_serv_sock, my_serv_sock;

	struct sockaddr_in* main_serv_addr;
	struct sockaddr_in* my_serv_addr;

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
	str_len = read(main_serv_sock, char_port, BUF_SIZE);

	int port = returnPort(string(char_port));
	printf("PORT RECEIVED : %d \n", port);

	my_serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	my_serv_addr = returnSockAddr(port);

	if (bind(my_serv_sock, (struct sockaddr*) my_serv_addr,
			sizeof(*my_serv_addr)) == -1)
		error_handling("bind() error");
	if (listen(my_serv_sock, 5) == -1)
		error_handling("listen() error");
	cout << "My Server Opened in Port: " << port << endl;
	//Add my_server socket to epoll
	event.events = EPOLLIN;
	event.data.fd = my_serv_sock;
	epoll_ctl(epfd, EPOLL_CTL_ADD, my_serv_sock, &event);

	while (1) {
		char buf[BUF_SIZE];
		event_cnt = epoll_wait(epfd, ep_events, EPOLL_SIZE, -1);
		if (event_cnt == -1) {
			puts("epoll wait() error");
		}

		for (int i = 0; i < event_cnt; i++) {
			//서버로부터 자식들에 대한 정보를 받고 연결
			if (ep_events[i].data.fd == main_serv_sock) {
				int strlen = read(ep_events[i].data.fd, buf, BUF_SIZE);
				if (strlen != 0) {
					childsockets.clear();
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

						childsockets.push_back(child_serv_sock);
					}
					//printf("received msg = %s\n", buf); //MY CHILDS PORT AND ADDRESS WILL COME HERE
				} else
					break;
			}
			//Parent가 생기고 연결되면 실행됨
			else if (ep_events[i].data.fd == my_serv_sock) {


				int strlen = read(ep_events[i].data.fd, buf, BUF_SIZE);
				struct sockaddr_in clnt_addr;
				int clnt_sock = accept(my_serv_sock,
						(struct sockaddr*) &clnt_addr, &addr_size);
				event.events = EPOLLIN;
				event.data.fd = clnt_sock;
				epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sock, &event);
				cout <<"connected client : " << clnt_sock <<  endl;
			}
			//Parent로부터 메세지가 날라오면 오는 공간
			else {
				int clnt_sock = ep_events[i].data.fd;
				int strlen = read(clnt_sock, buf, BUF_SIZE);
				if(strlen != 0){
					cout << "Message Received " << endl;
					for(int idx = 0; idx < childsockets.size(); idx++){
						write(childsockets.at(idx), buf, BUF_SIZE);
					}
				}
				//부모가 사라짐
				else{
					epoll_ctl(epfd, EPOLL_CTL_DEL, clnt_sock, NULL);
					cout <<"disconnected client : " << clnt_sock <<  endl;
				}
				//Check whether it is close Connection;
				//If it is, Close the Socket
			}
		}
	}
	close(main_serv_sock);
	close(my_serv_sock);
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

void returnChildInfo(string buf) {

}
int returnPort(string buf) {
	Json::Value attrbuf;
	Json::Value attr;
	Json::Reader reader;
	reader.parse(buf, attrbuf);
	attr = attrbuf["port"];
	return attr.asInt();
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
