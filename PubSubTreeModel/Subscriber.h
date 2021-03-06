/*
 * Subscriber.h
 *
 *  Created on: Nov 14, 2018
 *      Author: jinbro
 */

#ifndef SUBSCRIBER_H_
#define SUBSCRIBER_H_
#define CHILD_N 2
#include "json/json.h"
#include <iostream>
#include <string>
#include <vector>
using namespace std;

class Subscriber
{
        private:
                Subscriber *child[CHILD_N];
                int sock_fd= -1;
                int remain_child = CHILD_N;
                int port = -1;
                string ipaddr = "";
                vector<string> subscription;
                // 보류
                bool changed = false; // 추후 Child가 변경 될 시 표시함 --> 변경 알리고 난 후 다시  False 로 처리

        public:
                Subscriber();
                Subscriber(string ipaddr, int port, vector<string> subscription);
                ~Subscriber();

                bool addChild(Subscriber *child);
                bool delChild(Subscriber *child);

                int getSock_fd();
                int getRemain_child();
                int getPort();
                int getNumOfChilds();
                string getIpaddr();
                vector<string> getSubscription();
                bool getChanged();

                Subscriber* getChild(int pos);

                void setChild(int pos, Subscriber* sub);
                void setSock_fd(int sock_fd);
                void setIpaddr(string ipaddr);
                void setPort(int port);
                void setSubscription(vector<string> subscription);

                void toggleChanged(bool toggle);
                bool hasChild();

};
#endif /* SUBSCRIBER_H_ */
