#include <iostream>
#include "src/processpool/processpool.h"
#include <pthread.h>
#include <netinet/in.h>
struct T
{
    void init(int a, int b, sockaddr_in c) {}
    void process()
    {
        printf("call process\n");
    }
};
int main()
{
    processpool<T> p = processpool<T>::getInstance(3,5);
    if(&p != nullptr){
        p.run();
    }
}