//
// Created by xzz on 8/10/22.
//

#ifndef MYTINYWEBSERVER_NOCOPYABLE_H
#define MYTINYWEBSERVER_NOCOPYABLE_H

class nocopyable {
private:
    nocopyable(const nocopyable &) = delete;

    nocopyable &operator=(const nocopyable &) = delete;

    nocopyable(const nocopyable &&) = delete;

    nocopyable &operator=(const nocopyable &&) = delete;

public:
    nocopyable() = default;

    ~nocopyable() = default;
};

#endif //MYTINYWEBSERVER_NOCOPYABLE_H
