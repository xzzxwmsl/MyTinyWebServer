//
// Created by xzz on 8/10/22.
//

#ifndef MYTINYWEBSERVER_SINGLETON_H
#define MYTINYWEBSERVER_SINGLETON_H

#include "nocopyable.h"

// 通过模板实例化的方式来实现单例模式
template<typename T>
class Singleton : nocopyable {
private:
    Singleton() = default;

    ~Singleton() = default;

public:
    static T& getInstance() {
        static T instance;
        return instance;
    }
};

#endif //MYTINYWEBSERVER_SINGLETON_H
