//
// Created by borisshapa on 15.06.19.
//

#ifndef VECTOR_VECTOR_H
#define VECTOR_VECTOR_H

#include <cstddef>
#include <memory.h>
#include <algorithm>

template<typename U>
void
destr(U *data, size_t size, typename std::enable_if<std::is_trivially_destructible<U>::value>::type * = nullptr) {}

template<typename U>
void destr(U *data, size_t size, typename std::enable_if<!std::is_trivially_destructible<U>::value>::type * = nullptr) {
    for (ptrdiff_t i = size - 1; i >= 0; i--)
        data[i].~U();
}

template<typename U>
void copy(U *new_data_, U const *data_, size_t size,
          typename std::enable_if<!std::is_trivially_copyable<U>::value>::type * = nullptr) {
    size_t i = 0;
    try {
        for (; i < size; ++i)
            new(new_data_ + i) U(data_[i]);
    } catch (...) {
        destr(new_data_, i);
        throw;
    }
}

template<typename U>
void copy(U *new_data_, U const *data_, size_t size,
          typename std::enable_if<std::is_trivially_copyable<U>::value>::type * = nullptr) {
    if (size != 0)
        memcpy(new_data_, data_, size * sizeof(U));
}


template<typename T>
struct vector {

    typedef T *iterator;
    typedef T const *const_iterator;
    typedef std::reverse_iterator<T *> reverse_iterator;
    typedef std::reverse_iterator<T const *> const_reverse_iterator;

    vector() noexcept;

    ~vector();

    vector(vector const &other); // strong

    template<typename InputIterator>
    vector(InputIterator first, InputIterator last);

    vector &operator=(vector const &other); // strong

    template<typename InputIterator>
    void assign(InputIterator first, InputIterator last);

    T &operator[](size_t index); // strong
    T const &operator[](size_t index) const noexcept;

    T &front() noexcept;

    T const &front() const; // noexcept

    T &back() noexcept;

    T const &back() const noexcept;

    void push_back(T const elem); // strong

    void pop_back() noexcept;

    T *data();

    T const *data() const;

    iterator begin();

    iterator end();

    const_iterator begin() const;

    const_iterator end() const;

    reverse_iterator rbegin();

    reverse_iterator rend();

    const_reverse_iterator rbegin() const;

    const_reverse_iterator rend() const;


    bool empty() const noexcept;

    size_t size() const noexcept;

    void reserve(size_t size_);

    size_t capacity() const;

    void shrink_to_fit();

    void resize(size_t size_, T const &elem);

    void clear();

    iterator insert(const_iterator pos, T const val); // basic
    iterator erase(const_iterator pos); // basic
    iterator erase(const_iterator first, const_iterator last); // basic

    template<typename U>
    friend void swap(vector<U> &lhs, vector<U> &rhs);

    template<typename U>
    friend bool operator==(vector<U> const &lhs, vector<U> const &rhs);

    template<typename U>
    friend bool operator!=(vector<U> const &lhs, vector<U> const &rhs);

    template<typename U>
    friend bool operator<(vector<U> const &lhs, vector<U> const &rhs);

    template<typename U>
    friend bool operator<=(vector<U> const &lhs, vector<U> const &rhs);

    template<typename U>
    friend bool operator>(vector<U> const &lhs, vector<U> const &rhs);

    template<typename U>
    friend bool operator>=(vector<U> const &lhs, vector<U> const &rhs);

public :
    // data_[0] size
    // data_[1] capacity
    // data_[2] count_of_reference
    union {
        void *data_;
        T small_obj_;
    };

    size_t &get_size();

    size_t &capacity();

    size_t &ref();

    bool is_small;

    void new_buffer(size_t new_capacity);

    void copy_buffer();

    size_t increase_capacity() const;
};


template<typename T>
vector<T>::vector() noexcept : data_(nullptr), is_small(false) {}

template<typename T>
vector<T>::vector(vector const &other) {
    if (other.is_small) {
        new(&small_obj_) T(other.small_obj_);
        is_small = true;
    } else {
        data_ = other.data_;
        is_small = false;
        if (data_ != nullptr)
            ref()++;
    }
}

template<typename T>
template<typename InputIterator>
vector<T>::vector(InputIterator first, InputIterator last) : data_(nullptr), is_small(false) {
    size_t sz = last - first;
    void *new_data_ = operator new(3 * sizeof(size_t) + sz * sizeof(T));
    size_t *ptr = reinterpret_cast<size_t *>(new_data_);
    ptr[0] = ptr[1] = sz;
    ptr[2] = 1;
    T *real_data_ = reinterpret_cast<T *>(ptr + 3);
    try {
        copy(real_data_, first, sz);
        data_ = new_data_;
    } catch (...) {
        operator delete(new_data_);
        throw;
    }
}

template<typename T>
vector<T>::~vector() {
    if (is_small) {
        small_obj_.~T();
    } else if (data_ != nullptr) {
        if (ref() > 1) {
            ref()--;
        } else {
            destr(data(), size());
            operator delete(data_);
        }
    }
    is_small = false;
    data_ = nullptr;
}

template<typename T>
vector<T> &vector<T>::operator=(vector const &other) {
    if (data() != other.data()) {
        if (data_ != nullptr) {
            destr(data(), size());
            operator delete(data_);
        }
        if (other.is_small) {
            try {
                new(&small_obj_) T(other.small_obj_);
                is_small = true;
            } catch (...) {
                data_ = nullptr;
                is_small = false;
            }
        } else {
            data_ = other.data_;
            if (data_ != nullptr)
                ref()++;
            is_small = false;
        }
    }
    return *this;
}


template<typename T>
void vector<T>::new_buffer(size_t new_capacity) {
    if (new_capacity != 0) {
        void *new_data_ = operator new(3 * sizeof(size_t) + new_capacity * sizeof(T));
        size_t *ptr = reinterpret_cast<size_t *>(new_data_);
        ptr[0] = (is_small) ? 1 : (data_ == nullptr) ? 0 : std::min(size(), new_capacity);
        ptr[1] = new_capacity;
        ptr[2] = 1;
        T *real_data_ = reinterpret_cast<T *>(ptr + 3);
        try {
            if (is_small) {
                new(real_data_) T(small_obj_);
            } else {
                copy(real_data_, data(), std::min(size(), new_capacity));
            }
            (*this).~vector();
            data_ = new_data_;
            is_small = false;
        } catch (...) {
            operator delete(new_data_);
            throw;
        }
    }
}


template<typename T>
void vector<T>::copy_buffer() {
    void *new_data_ = nullptr;
    try {
        new_data_ = operator new(3 * sizeof(size_t) + capacity() * sizeof(T));
        size_t *ptr = reinterpret_cast<size_t *>(new_data_);
        ptr[0] = size();
        ptr[1] = capacity();
        ptr[2] = 1;
        copy(reinterpret_cast<T *>(ptr + 3), data(), size());
        data_ = new_data_;
    } catch (...) {
        operator delete(new_data_);
        throw;
    }
}


template<typename T>
size_t &vector<T>::get_size() {
    size_t *ptr = reinterpret_cast<size_t *>(data_);
    return ptr[0];
}

template<typename T>
size_t vector<T>::size() const noexcept {
    if (is_small) {
        return 1;
    } else if (data_ == nullptr)
        return 0;
    else {
        size_t *ptr = reinterpret_cast<size_t *>(data_);
        return ptr[0];
    }
}

template<typename T>
size_t &vector<T>::capacity() {
    size_t *ptr = reinterpret_cast<size_t *>(data_);
    return ptr[1];
}

template<typename T>
size_t vector<T>::capacity() const {
    size_t *ptr = reinterpret_cast<size_t *>(data_);
    return ptr[1];
}

template<typename T>
size_t &vector<T>::ref() {
    size_t *ptr = reinterpret_cast<size_t *>(data_);
    return ptr[2];
}

template<typename T>
T *vector<T>::data() {
    if (is_small) {
        return &small_obj_;
    } else if (data_ == nullptr) {
        return nullptr;
    } else {
        if (ref() > 1) {
            ref()--;
            try {
                copy_buffer();
            } catch (...) {
                ref()++;
                throw;
            }
        }
        size_t *ptr = reinterpret_cast<size_t *>(data_);
        return reinterpret_cast<T *>(ptr + 3);
    }
}

template<typename T>
T const *vector<T>::data() const {
    if (is_small) {
        return &small_obj_;
    } else if (data_ == nullptr) {
        return nullptr;
    } else {
        size_t *ptr = reinterpret_cast<size_t *>(data_);
        return reinterpret_cast<T *>(ptr + 3);
    }
}


template<typename T>
T &vector<T>::operator[](size_t index) {
    return data()[index];
}

template<typename T>
T const &vector<T>::operator[](size_t index) const noexcept {
    return data()[index];
}

template<typename T>
bool vector<T>::empty() const noexcept {
    return !is_small && data_ == nullptr;
}

template<typename T>
T &vector<T>::front() noexcept {
    return *data();
}

template<typename T>
T const &vector<T>::front() const {
    return *data();
}

template<typename T>
T &vector<T>::back() noexcept {
    return (is_small) ? small_obj_ : data()[size() - 1];
}

template<typename T>
T const &vector<T>::back() const noexcept {
    return (is_small) ? small_obj_ : data()[size() - 1];
}

template<typename T>
void vector<T>::push_back(const T elem) {
    if (is_small) {
        new_buffer(8);
        push_back(elem);
    } else if (data_ == nullptr) {
        new(&small_obj_) T(elem);
        is_small = true;
    } else {
        if (ref() > 1) {
            ref()--;
            try {
                copy_buffer();
            } catch (...) {
                ref()++;
                throw;
            }
        }
        if (size() != capacity()) {
            new(data() + size()) T(elem);
            get_size()++;
        } else {
            new_buffer(2 * capacity());
            push_back(elem);
        }
    }
}

template<typename T>
size_t vector<T>::increase_capacity() const {
    if (capacity() == 0)
        return 4;
    else
        return capacity() * 2;
}

template<typename T>
void vector<T>::pop_back() noexcept {
    if (is_small) {
        small_obj_.~T();
        is_small = false;
        data_ = nullptr;
    } else if (data_ != nullptr) {
        if (ref() > 1) {
            size_t &refs_ = ref();
            copy_buffer();
            refs_--;
        }
        data()[size() - 1].~T();
        get_size()--;
        if (size() == 0) {
            destr(data(), size());
            operator delete(data_);
            is_small = false;
            data_ = nullptr;
        }
    }
}

template<typename T>
void vector<T>::reserve(size_t size_) {
    if (size_ > 1 && (is_small || data_ == nullptr || capacity() < size_))
        new_buffer(size_);
}

template<typename T>
void vector<T>::shrink_to_fit() {
    if (capacity() != size())
        new_buffer(size());
}

template<typename T>
void vector<T>::resize(size_t size_, T const &elem) {
    if (size_ < size()) {
        new_buffer(size_);
        return;
    }
    vector tmp;
    reserve(size_);
    for (size_t i = 0; i < size(); i++) {
        tmp.push_back(data()[i]);
    }
    while (tmp.size() < size_) {
        tmp.push_back(elem);
    }
    swap(*this, tmp);
}

template<typename T>
void vector<T>::clear() {
    destr(data(), size());
    get_size() = 0;
    capacity() = 0;
}

template<typename T>
template<typename InputIterator>
void vector<T>::assign(InputIterator first, InputIterator last) {
    vector<T> tmp(first, last);
    swap(*this, tmp);
}

template<typename T>
typename vector<T>::iterator vector<T>::begin() {
    if (is_small) {
        return &small_obj_;
    } else if (data_ == nullptr) {
        return nullptr;
    } else {
        if (ref() > 1) {
            ref()--;
            copy_buffer();
        }
        return data();
    }
}

template<typename T>
typename vector<T>::iterator vector<T>::end() {
    if (is_small) {
        iterator it = &small_obj_;
        return ++it;
    } else if (data_ == nullptr) {
        return nullptr;
    } else {
        if (ref() > 1) {
            ref()--;
            copy_buffer();
        }
        return data() + size();
    }
}

template<typename T>
typename vector<T>::const_iterator vector<T>::begin() const {
    if (is_small) {
        return &small_obj_;
    } else if (data_ == nullptr) {
        return nullptr;
    } else {
        return data();
    }
}

template<typename T>
typename vector<T>::const_iterator vector<T>::end() const {
    if (is_small) {
        return (&small_obj_) + 1;
    }
    return (data_ == nullptr) ? nullptr : data() + size();
}

template<typename T>
typename vector<T>::reverse_iterator vector<T>::rbegin() {
    return vector::reverse_iterator(end());
}

template<typename T>
typename vector<T>::reverse_iterator vector<T>::rend() {
    return vector::reverse_iterator(begin());
}

template<typename T>
typename vector<T>::const_reverse_iterator vector<T>::rbegin() const {
    return vector::const_reverse_iterator(end());
}

template<typename T>
typename vector<T>::const_reverse_iterator vector<T>::rend() const {
    return vector::const_reverse_iterator(begin());
}

template<typename T>
typename vector<T>::iterator vector<T>::insert(vector::const_iterator pos, const T val) {
    size_t sz = pos - begin();
    if (pos == end()) {
        push_back(val);
        return begin();
    }
    push_back(val);
    iterator it = (end() - 1);
    for (size_t i = 0; i < size() - sz - 1; i++, it--) {
        std::swap(*it, *(it - 1));
    }
    return it;
}

template<typename T>
typename vector<T>::iterator vector<T>::erase(vector::const_iterator first, vector::const_iterator last) {
    iterator first_ = begin(), last_ = begin();
    for (; first_ != first; first_++) {}
    for (; last_ != last; last_++) {}

    iterator ans = first_;
    while (last_ != end()) {
        std::swap(*first_, *last_);
        ++first_;
        ++last_;
    }

    for (size_t i = 0; i < size_t(last_ - first_); i++) {
        pop_back();
    }
    return ans;
}

template<typename T>
typename vector<T>::iterator vector<T>::erase(vector::const_iterator pos) {
    return erase(pos, pos + 1);
}

template<typename T>
void swap(vector<T> &lhs, vector<T> &rhs) {
    if (lhs.is_small && rhs.is_small) {
        std::swap(lhs.small_obj_, rhs.small_obj_);
    } else if (!lhs.is_small && !rhs.is_small) {
        std::swap(lhs.data_, rhs.data_);
    } else {
        if (lhs.is_small) {
            vector<T> tmp(rhs);
            try {
                new(&rhs.small_obj_) T(lhs.small_obj_);
            } catch (...) {
                rhs.data_ = tmp.data_;
                throw;
            }
            lhs.small_obj_.~T();
            rhs.is_small = true;
            lhs.data_ = tmp.data_;
            lhs.is_small = false;
        } else {
            vector<T> tmp(lhs);
            try {
                new(&lhs.small_obj_) T(rhs.small_obj_);
            } catch (...) {
                lhs.data_ = tmp.data_;
                throw;
            }
            rhs.small_obj_.~T();
            lhs.is_small = true;
            rhs.data_ = tmp.data_;
            rhs.is_small = false;
        }
    }
}

template<typename T>
bool operator==(vector<T> const &lhs, vector<T> const &rhs) {
    if (lhs.is_small && rhs.is_small) {
        return lhs.small_obj_ == rhs.small_obj_;
    }
    if (lhs.is_small != rhs.is_small || lhs.size() != rhs.size())
        return false;
    for (size_t i = 0; i < lhs.size(); i++) {
        if (lhs[i] != rhs[i])
            return false;
    }
    return true;
}

template<typename T>
bool operator<(vector<T> const &lhs, vector<T> const &rhs) {
    for (size_t i = 0; i < std::min(lhs.size(), rhs.size()); i++) {
        if (lhs[i] < rhs[i])
            return true;
        if (lhs[i] > rhs[i])
            return false;
    }
    return lhs.size() < rhs.size();
}

template<typename T>
bool operator!=(vector<T> const &lhs, vector<T> const &rhs) {
    return !(lhs == rhs);
}

template<typename T>
bool operator>(vector<T> const &lhs, vector<T> const &rhs) {
    return rhs < lhs;
}

template<typename T>
bool operator<=(vector<T> const &lhs, vector<T> const &rhs) {
    return !(lhs > rhs);
}

template<typename T>
bool operator>=(vector<T> const &lhs, vector<T> const &rhs) {
    return !(lhs < rhs);
}

#endif //VECTOR_VECTOR_H
