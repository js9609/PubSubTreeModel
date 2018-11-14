/*
 * Publisher.h
 *
 *  Created on: Nov 14, 2018
 *      Author: jinbro
 */

#ifndef PUBLISHER_H_
#define PUBLISHER_H_


class Publisher{
private:
	int sock_fd = -1;

public:
	Publisher();
	~Publisher();

	int getSock_fd();
	void setSock_fd(int sock_fd);
};


#endif /* PUBLISHER_H_ */
