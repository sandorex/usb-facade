#include <stdio.h>
#include <string.h>
#include <libusb.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <functional>
#include <bitset>
#include <optional>
#include <format>

/// @brief simple RAII class that calls callback on destructor
class RAII {
private:
    std::function<void()> _fun;

public:
    RAII(std::function<void()> fun)
        : _fun(fun) {}

    ~RAII() {
        this->_fun();
    }
};

#define CONCAT_(prefix, suffix) prefix##suffix
#define CONCAT(prefix, suffix) CONCAT_(prefix, suffix)
#define DEFER(x) RAII CONCAT(__raii_, __LINE__) (x)
