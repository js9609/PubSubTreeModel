/*
 * main.cpp
 *
 *  Created on: Nov 14, 2018
 *      Author: jinbro
 */

#include <iostream>
#include <stdio.h>
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
#include "Subscriber.h"
using namespace std;

//Function 선언문
void error_handling(string msg); //Error Handling
bool isDependent(Subscriber *child, Subscriber *parent);
bool insertSubscriber(Subscriber *child, Subscriber *parent);
void printTree(Subscriber* sub);
void updateTree(Subscriber* child);
bool contains(string subscription_child, string subscription_parent);
string returnAttr(string buf);
string returnPort(int port);
string returnType(string buf);
void sendInfo(Subscriber* sub);
void sendToAllChild(Subscriber* sub);
void sendRootInfo(int sock_fd);
/******************/

//Define 정의
#define EPOLL_SIZE 256
#define BUF_SIZE 100

//***************/

//전역변수
vector<Subscriber*> vector_root;
int port = 9000;
//
int main(int argc, char *argv[]) {

	if (argc != 2) {
		printf("Usage : %s <port> \n", argv[0]);
		exit(1);
	}
	/** 서버 생성 **/
	struct sockaddr_in serv_addr, clnt_addr;
	socklen_t addr_size;
	int serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(atoi(argv[1]));

	if (bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1)
		error_handling("bind() error");
	if (listen(serv_sock, 5) == -1)
		error_handling("listen() error");

	cout << "Server" << endl;
	/**/

	//epoll
	struct epoll_event* ep_events;
	struct epoll_event event;
	int epfd, event_cnt;
	/**/
	epfd = epoll_create(EPOLL_SIZE);
	ep_events = (struct epoll_event*) malloc(
			sizeof(struct epoll_event) * EPOLL_SIZE);
	//이벤트 처리에 서버 소켓 추가
	event.events = EPOLLIN;
	event.data.fd = serv_sock;
	epoll_ctl(epfd, EPOLL_CTL_ADD, serv_sock, &event);

	while (1) {
		event_cnt = epoll_wait(epfd, ep_events, EPOLL_SIZE, -1);
		if (event_cnt == -1) {
			puts("epoll wait() error");
			break;
		}
		for (int idx = 0; idx < event_cnt; idx++) {
			//서버 소켓으로 들어온 신호 -- 연결 (Client.connect)
			if (ep_events[idx].data.fd == serv_sock) {
				addr_size = sizeof(clnt_addr);
				int clnt_sock = accept(serv_sock, (struct sockaddr*) &clnt_addr,
						&addr_size);
				event.events = EPOLLIN;
				event.data.fd = clnt_sock;
				epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sock, &event);
				printf("connected client : %d\n", clnt_sock);
			} else {
				char buf[BUF_SIZE];
				int clnt_sock = ep_events[idx].data.fd;
				int str_len = read(clnt_sock, buf, BUF_SIZE);

				//IF CLOSE CONNECTION OCCUR
				if (str_len == 0) {
					epoll_ctl(epfd, EPOLL_CTL_DEL, clnt_sock, NULL);
					cout << "disconnected " << endl;
				} else {
					if (returnType(buf).compare("sub") == 0) {
						// IF NEW SUBSCRIBER ADDED
						Subscriber* sub = new Subscriber();
						sub->setPort(port);
						sub->setSock_fd(clnt_sock);
						sub->setSubscription(returnAttr(buf));
						//Send Back to Subscriber;
						string str_port = returnPort(port);
						char *char_port = new char[str_port.length() + 1];
						strcpy(char_port, str_port.c_str());
						write(clnt_sock, char_port, BUF_SIZE);
						delete[] char_port;
						port++;
						updateTree(sub);
						for (int idx = 0; idx < vector_root.size(); idx++) {
							printTree(vector_root.at(idx));
							cout << endl;
						}
					} else {
						sendRootInfo(clnt_sock);
						//Publisher 정의
					}
					for (int idx = 0; idx < vector_root.size(); idx++)
						sendToAllChild(vector_root.at(idx));

				}
			}
		}
	}

}

void printTree(Subscriber* sub) {
	cout << "Port: " << sub->getPort() << ", Subscription: "
			<< sub->getSubscription() << " " << sub->getChanged() << "-->";
	for (int i = 0; i < sub->getNumOfChilds(); i++) {
		printTree(sub->getChild(i));
	}

}

//return json to string
string returnAttr(string buf) {
	Json::Value attrbuf;
	Json::Value attr;
	Json::Reader reader;
	reader.parse(buf, attrbuf);
	attr = attrbuf["attr"];
	return attr.asString();
}

//return port to Json
string returnPort(int port) {
	Json::Value portbuf;
	Json::FastWriter writer;
	portbuf["port"] = port;
	string ret = writer.write(portbuf);
	return ret;
}

//return json to string
string returnType(string buf) {
	Json::Value typebuf;
	Json::Value type;
	Json::Reader reader;
	reader.parse(buf, typebuf);
	type = typebuf["type"];
	return type.asString();
	//sub of pub
}
//DFS 서치 //TODO
void updateTree(Subscriber* child) {
	for (int idx = 0; idx < vector_root.size(); idx++) {
		if (isDependent(vector_root.at(idx), child)) {
			child->addChild(vector_root.at(idx));
			child->toggleChanged(true);
			vector_root.at(idx) = child;
			return;
		}
	}

	for (int idx = 0; idx < vector_root.size(); idx++) {
		if (insertSubscriber(child, vector_root.at(idx))) {
			return;
		}
	}
	vector_root.push_back(child);
	return;
}

bool insertSubscriber(Subscriber *child, Subscriber *parent) {
	if (!isDependent(child, parent)) {
		return false;
	}
	if (!parent->hasChild()) {
		parent->addChild(child);
		parent->toggleChanged(true);
		return true;
	}
	for (int cdx = 0; cdx < parent->getNumOfChilds(); cdx++) {
		if (insertSubscriber(child, parent->getChild(cdx))) {
			return true;
		} else {
			if (isDependent(parent->getChild(cdx), child)) {
				Subscriber* tmp = parent->getChild(cdx);
				parent->setChild(cdx, child);
				child->addChild(tmp);
				parent->toggleChanged(true);
				child->toggleChanged(true);
				return true;
			} else if (parent->getRemain_child() > 0) {
				parent->addChild(child);
				parent->toggleChanged(true);
				return true;
			} else
				return false;

		}
	}
	return false;
}
//Subscription이 종속관계에 있는지 파악
bool isDependent(Subscriber *child, Subscriber *parent) {
	return contains(child->getSubscription(), parent->getSubscription());
}

//TODO
bool contains(string subscription_child, string subscription_parent) {

	if (subscription_child.length() < subscription_parent.length())
		return true;
	else
		return false;

}

bool isSubset(string child, string parent) {
	Json::Value childarray, parentarray;
	Json::Reader reader;;
	reader.parse(child, childarray);
	reader.parse(parent, parentarray);

	for (int cdx = 0; cdx < childarray.size(); cdx++) {
		for (int pdx = 0; pdx < parentarray.size(); pdx++) {
			parentarray[pdx].compare(childarray[cdx]);
		}
	}
	return true;
}

void error_handling(string msg) {
	cout << msg << endl;
	exit(1);
}

void sendToAllChild(Subscriber* sub) {
	sendInfo(sub);
	for (int idx = 0; idx < sub->getNumOfChilds(); idx++) {
		sendInfo(sub->getChild(idx));
	}
}

void sendInfo(Subscriber* sub) {
	Json::Value childs;
	Json::FastWriter writer;
	if (sub->getChanged()) {
		cout << "sent to " << sub->getPort() << endl;
		for (int idx = 0; idx < sub->getNumOfChilds(); idx++) {
			childs[idx] = sub->getChild(idx)->getPort();
		}
		string tmp = writer.write(childs);
		const char*buf = tmp.c_str();
		write(sub->getSock_fd(), buf, BUF_SIZE);
		sub->toggleChanged(false);
	}
}

void sendRootInfo(int sock_fd) {
	Json::Value root;
	Json::FastWriter writer;

	for (int idx = 0; idx < vector_root.size(); idx++) {
		root[idx] = vector_root.at(idx)->getPort();
	}
	string str = writer.write(root);
	const char*buf = str.c_str();
	write(sock_fd, buf, BUF_SIZE);
	return;

}
//안쓸거 같은것둘

