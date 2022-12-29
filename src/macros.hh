// Copyright 2022 Aleksandar Radivojević
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <functional>

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
