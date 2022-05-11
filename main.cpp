#include<iostream>
#include"locker/locker.h"
#include<pthread.h>
void* func(void*argc){
    printf("call thread\n");
    pthread_exit(NULL);
}
int main(){
    pthread_t a;
    a = pthread_create(&a, NULL,func,NULL);
    sem s;
}