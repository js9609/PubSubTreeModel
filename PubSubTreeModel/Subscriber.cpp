/*
 * Subscriber.cpp
 *
 *  Created on: Nov 14, 2018
 *      Author: jinbro
 */

#include "json/json.h"
#include <iostream>
#include <string>
#include "Subscriber.h"
using namespace std;

Subscriber::Subscriber() {
}
Subscriber::Subscriber(string ipaddr, int port, vector<string> subscription) {
	this->ipaddr = ipaddr;
	this->port = port;
	this->subscription = subscription;
	remain_child = CHILD_N;
}
bool Subscriber::addChild(Subscriber *child) {
	if (remain_child < 1) {
		return false;
	} else {
		int idx = CHILD_N - remain_child;
		remain_child--;
		this->child[idx] = child;
		return true;
	}
}

//TODO
bool Subscriber::delChild(Subscriber *child) {
	return true;
}

//Getter
int Subscriber::getRemain_child() {
	return remain_child;
}
int Subscriber::getSock_fd() {
	return sock_fd;
}
int Subscriber::getPort() {
	return port;
}
string Subscriber::getIpaddr() {
	return ipaddr;
}
vector<string> Subscriber::getSubscription() {
	return subscription;
}
int Subscriber::getNumOfChilds(){
	return CHILD_N - remain_child;
}
Subscriber* Subscriber::getChild(int pos){
	return child[pos];
}
void Subscriber::setChild(int pos, Subscriber* sub){
	child[pos] = sub;
}

//Setter
void Subscriber::setSock_fd(int sock_fd) {
	this->sock_fd = sock_fd;
}
void Subscriber::setIpaddr(string ipaddr) {
	this->ipaddr = ipaddr;
}
void Subscriber::setPort(int port) {
	this->port = port;
}
void Subscriber::setSubscription(vector<string> subscription) {
	this->subscription = subscription;
}

bool Subscriber::hasChild() {
	if (remain_child != CHILD_N)
		return true;
	else
		return false;
}

void Subscriber::toggleChanged(bool toggle){
	changed = toggle;
	return;
}
bool Subscriber::getChanged(){
	return changed;
}



