/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <memory>
#include <type_traits>

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
class sptr_view
{
    T* ptr;
public:
    sptr_view() = delete;
    sptr_view(T* ptr) : ptr(ptr) {}
    
    template<class U> requires std::is_convertible_v<U*, T*>
    sptr_view(const sptr<U>& sp) : ptr(sp.get()) {}

    template<class U> requires std::is_convertible_v<U*, T*>
    sptr_view(sptr<U>&&) = delete;

    T& operator*() const { return *ptr; }
    T* operator->() const { return ptr; }
    operator T*() const { return ptr; }
    operator bool() const { return !!ptr; }

    template<class U> requires std::is_convertible_v<U*, T*>
    bool operator==(const sptr_view<U>& other) const { return ptr == other.ptr; }
};

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
