#include <iostream>
#include "../src/threadpool/threadpool.h"
#include <pthread.h>
#include <netinet/in.h>
struct T
{
    static int v;
    static locker loc;
    int a, b;
    T(int a_ = 0, int b_ = 0) : a(a_), b(b_) {}

    void process()
    {
        printf("compute %d to %d\n", a, b);
        int sum = 0;
        for (int i = a + 1; i <= b; ++i)
            sum += i;
        loc.lock();
        v += sum;
        loc.unlock();
        printf("over %d, sum %d\n", a, v);
    }
};
int T::v = 0;
locker T::loc;
int main()
{
    threadpool<T> tp(8, 1000);
    T *task = new T[10];
    for (int i = 0; i < 10; i++)
    {
        task[i].a = 1000 * i;
        task[i].b = 1000 * i + 1000;
    }
    printf("---------\n");
    for (int i = 0; i < 10; i++)
    {
        tp.append(&task[i]);
    }

    getchar();
}