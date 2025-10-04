/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <memory>

namespace fraze {

template<class T, class D = std::default_delete<T>>
using uptr = std::unique_ptr<T, D>;

template<class T>
using sptr = std::shared_ptr<T>;

template<typename T, typename... Args>
inline sptr<T> spnew(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

template<class T>
class sptr_from_this : public std::enable_shared_from_this<T>
{
public:
    template<class Self>
    auto self(this Self& obj) {
        return std::shared_ptr<Self>(obj.shared_from_this(), &obj);
    }
};

} // fraze
