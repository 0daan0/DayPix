#pragma once

#ifndef ARX_RINGBUFFER_H
#define ARX_RINGBUFFER_H

#include "ArxContainer/has_include.h"
#include "ArxContainer/has_libstdcplusplus.h"

#ifdef ARDUINO
#include <Arduino.h>
#endif

#include "ArxContainer/replace_minmax_macros.h"
#include "ArxContainer/initializer_list.h"

#if ARX_HAVE_LIBSTDCPLUSPLUS >= 201103L  // Have libstdc++11

#include <vector>
#include <array>
#include <deque>
#include <map>

#else  // Do not have libstdc++11

#include <limits.h>

#ifndef ARX_VECTOR_DEFAULT_SIZE
#define ARX_VECTOR_DEFAULT_SIZE 16
#endif  // ARX_VECTOR_DEFAULT_SIZE

#ifndef ARX_DEQUE_DEFAULT_SIZE
#define ARX_DEQUE_DEFAULT_SIZE 16
#endif  // ARX_DEQUE_DEFAULT_SIZE

#ifndef ARX_MAP_DEFAULT_SIZE
#define ARX_MAP_DEFAULT_SIZE 16
#endif  // ARX_MAP_DEFAULT_SIZE

namespace arx {

namespace container {
    namespace detail {
        template <class T>
        T&& move(T& t) { return static_cast<T&&>(t); }
    }  // namespace detail
}  // namespace container

template <typename T, size_t N>
class RingBuffer {
    class ConstIterator {
        friend RingBuffer<T, N>;

        T* ptr {nullptr};  // pointer to the first element
        int pos {0};

        ConstIterator(T* ptr, int pos)
        : ptr(ptr), pos(pos) {}

    public:
        ConstIterator() {}
        ConstIterator(const ConstIterator& it) {
            this->ptr = it.ptr;
            this->pos = it.pos;
        }

        ConstIterator(ConstIterator&& it) {
            this->ptr = container::detail::move(it.ptr);
            this->pos = container::detail::move(it.pos);
        }

        int index() const {
            if (pos >= 0)
                return pos % N;
            else
                return N - (abs(pos) % (N + 1));
        }

        int index_with_offset(const int i) const {
            const int p = pos + i;
            if (p >= 0)
                return p % N;
            else
                return N - (abs(p) % (N + 1));
        }

        const T& operator*() const {
            return *(ptr + index());
        }
        const T* operator->() const {
            return ptr + index();
        }

        ConstIterator operator+(const ConstIterator& rhs) const {
            ConstIterator it(this->ptr, this->pos + rhs.pos);
            return it;
        }
        ConstIterator operator+(const int n) const {
            ConstIterator it(this->ptr, this->pos + n);
            return it;
        }
        ConstIterator operator-(const ConstIterator& rhs) const {
            ConstIterator it(this->ptr, this->pos - rhs.pos);
            return it;
        }
        ConstIterator operator-(const int n) const {
            ConstIterator it(this->ptr, this->pos - n);
            return it;
        }
        ConstIterator& operator+=(const ConstIterator& rhs) {
            this->pos += rhs.pos;
            return *this;
        }
        ConstIterator& operator+=(const int n) {
            this->pos += n;
            return *this;
        }
        ConstIterator& operator-=(const ConstIterator& rhs) {
            this->pos -= rhs.pos;
            return *this;
        }
        ConstIterator& operator-=(const int n) {
            this->pos -= n;
            return *this;
        }

        // prefix increment/decrement
        ConstIterator& operator++() {
            ++pos;
            return *this;
        }
        ConstIterator& operator--() {
            --pos;
            return *this;
        }
        // postfix increment/decrement
        ConstIterator operator++(int) {
            ConstIterator it = *this;
            ++pos;
            return it;
        }
        ConstIterator operator--(int) {
            ConstIterator it = *this;
            --pos;
            return it;
        }

        ConstIterator& operator=(const ConstIterator& rhs) {
            this->ptr = rhs.ptr;
            this->pos = rhs.pos;
            return *this;
        }
        ConstIterator& operator=(ConstIterator&& rhs) {
            this->ptr = container::detail::move(rhs.ptr);
            this->pos = container::detail::move(rhs.pos);
            return *this;
        }

        bool operator==(const ConstIterator& rhs) const {
            return (rhs.ptr == ptr) && (rhs.pos == pos);
        }
        bool operator!=(const ConstIterator& rhs) const {
            return !(*this == rhs);
        }
        bool operator<(const ConstIterator& rhs) const {
            return pos < rhs.pos;
        }
        bool operator<=(const ConstIterator& rhs) const {
            return pos <= rhs.pos;
        }
        bool operator>(const ConstIterator& rhs) const {
            return pos > rhs.pos;
        }
        bool operator>=(const ConstIterator& rhs) const {
            return pos >= rhs.pos;
        }

    private:
        int raw_pos() const {
            return pos;
        }

        void set(const int i) {
            pos = i;
        }

        void reset() {
            pos = 0;
        }
    };

    class Iterator : public ConstIterator {
    public:
        using ConstIterator::ConstIterator;

        Iterator(const Iterator&) = default;
        Iterator& operator=(const Iterator&) = default;
        Iterator(Iterator&&) = default;
        Iterator() = default;
        Iterator& operator=(Iterator&&) = default;

        Iterator(const ConstIterator& cit) {
            this->ptr = cit.ptr;
            this->pos = cit.pos;
        }

        Iterator& operator=(const ConstIterator& rhs) {
            ConstIterator::operator=(rhs);
            return *this;
        }
        Iterator& operator=(ConstIterator&& rhs) noexcept {
            ConstIterator::operator=(rhs);
            return *this;
        }

        T& operator*() const {
            return *(ptr + ConstIterator::index());
        }
        T* operator->() const {
            return ptr + ConstIterator::index();
        }
    };

protected:
    friend class Iterator;
    friend class ConstIterator;

    T queue_[N];
    Iterator head_;
    Iterator tail_;

public:
    using iterator = Iterator;
    using const_iterator = ConstIterator;

    RingBuffer()
    : queue_()
    , head_(queue_, 0)
    , tail_(queue_, 0) {
    }

    RingBuffer(std::initializer_list<T> lst)
    : queue_()
    , head_(queue_, 0)
    , tail_(queue_, 0) {
        for (auto it = lst.begin(); it != lst.end(); ++it) {
            push_back(*it);
        }
    }

    // copy
    explicit RingBuffer(const RingBuffer& r)
    : queue_()
    , head_(queue_, r.head_.raw_pos())
    , tail_(queue_, r.tail_.raw_pos()) {
        for (size_t i = 0; i < r.size(); ++i)
            queue_[i] = r.queue_[i];
    }
    RingBuffer& operator=(const RingBuffer& r) {
        head_.set(r.head_.raw_pos());
        tail_.set(r.tail_.raw_pos());
        for (size_t i = 0; i < r.size(); ++i)
            queue_[i] = r.queue_[i];
        return *this;
    }

    // move
    RingBuffer(RingBuffer&& r) {
        head_ = container::detail::move(r.head_);
        tail_ = container::detail::move(r.tail_);
        for (size_t i = 0; i < r.size(); ++i)
            queue_[i] = container::detail::move(r.queue_[i]);
    }

    RingBuffer& operator=(RingBuffer&& r) {
        head_ = container::detail::move(r.head_);
        tail_ = container::detail::move(r.tail_);
        for (size_t i = 0; i < r.size(); ++i)
            queue_[i] = container::detail::move(r.queue_[i]);
        return *this;
    }

    virtual ~RingBuffer() {}

    size_t capacity() const { return N; };
    size_t size() const { return abs(tail_.raw_pos() - head_.raw_pos()); }
    inline const T* data() const { return &(get(head_)); }
    T* data() { return &(get(head_)); }
    bool empty() const { return tail_ == head_; }
    void clear() {
        head_.reset();
        tail_.reset();
    }

    void pop() {
        pop_front();
    }
    void pop_front() {
        if (size() == 0) return;
        if (size() == 1)
            clear();
        else
            increment_head();
    }
    void pop_back() {
        if (size() == 0) return;
        if (size() == 1)
            clear();
        else
            decrement_tail();
    }

    void push(const T& data) {
        push_back(data);
    }
    void push(T&& data) {
        push_back(data);
    }
    void push_back(const T& data) {
        get(tail_) = data;
        increment_tail();
    };
    void push_back(T&& data) {
        get(tail_) = data;
        increment_tail();
    };
    void push_front(const T& data) {
        get(head_) = data;
        decrement_head();
    };
    void push_front(T&& data) {
        get(head_) = data;
        decrement_head();
    };
    void emplace(const T& data) { push(data); }
    void emplace(T&& data) { push(data); }
    void emplace_back(const T& data) { push_back(data); }
    void emplace_back(T&& data) { push_back(data); }

    const T& front() const { return get(head_); }
    T& front() { return get(head_); }

    const T& back() const { return get(size() - 1); }
    T& back() { return get(size() - 1); }

    const T& operator[](size_t index) const { return get((int)index); }
    T& operator[](size_t index) { return get((int)index); }

    iterator begin() { return empty() ? Iterator() : head_; }
    iterator end() { return empty() ? Iterator() : tail_; }
    const_iterator begin() const { return empty() ? ConstIterator() : static_cast<ConstIterator>(head_); }
    const_iterator end() const { return empty() ? ConstIterator() : static_cast<ConstIterator>(tail_); }

    iterator erase(const iterator& p) {
        if (!is_valid(p)) return end();

        iterator it_last = begin() + size() - 1;
        for (iterator it = p; it != it_last; ++it)
            *it = *(it + 1);
        *it_last = T();
        decrement_tail();
        return empty() ? end() : p;
    }

    void resize(size_t sz) {
        size_t s = size();
        if (sz > size()) {
            for (size_t i = 0; i < sz - s; ++i) push(T());
        } else if (sz < size()) {
            for (size_t i = 0; i < s - sz; ++i) pop();
        }
    }

    void assign(const_iterator first, const_iterator end) {
        clear();
        while (first != end) push(*(first++));
    }

    void assign(const T* first, const T* end) {
        clear();
        // T* it = first;
        while (first != end) push(*(first++));
    }

    void shrink_to_fit() {
        // dummy
    }

    void reserve(size_t n) {
        (void)n;
        // dummy
    }

    void fill(const T& v) {
        iterator it = begin();
        while (it != end()) {
            *it = v;
            ++it;
        }
    }

    void insert(const_iterator pos, const_iterator first, const_iterator last) {
        if (pos != end()) {
            size_t sz = 0;
            {
                for (iterator it = first; it != last; ++it) ++sz;
            }
            iterator it = end() + sz - 1;
            for (int i = sz; i > 0; --i, --it)
                *it = *(it - sz);
            it = pos;
            for (size_t i = 0; i < sz; ++i)
                *it = *(first + i);
        } else {
            for (iterator it = first; it != last; ++it)
                push_back(*it);
        }
    }

    void insert(const_iterator pos, const T* first, const T* last) {
        if (pos != end()) {
            size_t sz = 0;
            {
                for (const T* it = first; it != last; ++it) ++sz;
            }
            iterator it = end() + sz - 1;
            for (int i = sz; i > 0; --i, --it)
                *it = *(it - sz);
            it = pos;
            for (size_t i = 0; i < sz; ++i)
                *it = *(first + i);
        } else {
            for (const T* it = first; it != last; ++it)
                push_back(*it);
        }
    }

private:
    T& get(const Iterator& it) {
        return queue_[it.index()];
    }
    const T& get(const Iterator& it) const {
        return queue_[it.index()];
    }
    T& get(const int index) {
        return queue_[head_.index_with_offset(index)];
    }
    const T& get(const int index) const {
        return queue_[head_.index_with_offset(index)];
    }

    T* ptr(const Iterator& it) {
        return (T*)(queue_ + it.index());
    }
    const T* ptr(const Iterator& it) const {
        return (T*)(queue_ + it.index());
    }
    T* ptr(const int index) {
        return (T*)(queue_ + head_.index_with_offset(index));
    }
    const T* ptr(const int index) const {
        return (T*)(queue_ + head_.index_with_offset(index));
    }

    void increment_head() {
        ++head_;
        resolve_overflow();
    }
    void increment_tail() {
        ++tail_;
        resolve_overflow();
        if (size() > N)
            increment_head();
    }
    void decrement_head() {
        --head_;
        resolve_overflow();
        if (size() > N)
            decrement_tail();
    }
    void decrement_tail() {
        --tail_;
        resolve_overflow();
    }

    void resolve_overflow() {
        if (empty())
            clear();
        else if (head_.raw_pos() > tail_.raw_pos()) {
            // the same value will be obtained regardless of which of the head tail overflows
            int len = (INT_MAX - head_.raw_pos()) + (tail_.raw_pos() - INT_MIN);
            clear();
            tail_.set(len);
        }
    }

    bool is_valid(const iterator& it) const {
        return (it.raw_pos() >= head_.raw_pos()) && (it.raw_pos() < tail_.raw_pos());
    }
};

template <typename T, size_t N>
bool operator==(const RingBuffer<T, N>& x, const RingBuffer<T, N>& y) {
    if (x.size() != y.size()) return false;
    for (size_t i = 0; i < x.size(); ++i)
        if (x[i] != y[i]) return false;
    return true;
}

template <typename T, size_t N>
bool operator!=(const RingBuffer<T, N>& x, const RingBuffer<T, N>& y) {
    return !(x == y);
}

template <typename T, size_t N = ARX_VECTOR_DEFAULT_SIZE>
struct vector : public RingBuffer<T, N> {
    using iterator = typename RingBuffer<T, N>::iterator;
    using const_iterator = typename RingBuffer<T, N>::const_iterator;

    vector()
    : RingBuffer<T, N>() {}
    vector(std::initializer_list<T> lst)
    : RingBuffer<T, N>(lst) {}

    // copy
    vector(const vector& r)
    : RingBuffer<T, N>(r) {}

    vector& operator=(const vector& r) {
        RingBuffer<T, N>::operator=(r);
        return *this;
    }

    // move
    vector(vector&& r)
    : RingBuffer<T, N>(r) {}

    vector& operator=(vector&& r) {
        RingBuffer<T, N>::operator=(r);
        return *this;
    }

    virtual ~vector() {}

private:
    using RingBuffer<T, N>::pop;
    using RingBuffer<T, N>::pop_front;
    using RingBuffer<T, N>::push;
    using RingBuffer<T, N>::push_front;
    using RingBuffer<T, N>::emplace;
    using RingBuffer<T, N>::fill;
};

template <typename T, size_t N>
struct array : public RingBuffer<T, N> {
    using iterator = typename RingBuffer<T, N>::iterator;
    using const_iterator = typename RingBuffer<T, N>::const_iterator;

    array()
    : RingBuffer<T, N>() {}
    array(std::initializer_list<T> lst)
    : RingBuffer<T, N>(lst) {}

    // copy
    array(const array& r)
    : RingBuffer<T, N>(r) {}

    array& operator=(const array& r) {
        RingBuffer<T, N>::operator=(r);
        return *this;
    }

    // move
    array(array&& r)
    : RingBuffer<T, N>(r) {}

    array& operator=(array&& r) {
        RingBuffer<T, N>::operator=(r);
        return *this;
    }

    virtual ~array() {}

private:
    using RingBuffer<T, N>::pop;
    using RingBuffer<T, N>::pop_front;
    using RingBuffer<T, N>::push;
    using RingBuffer<T, N>::push_front;
    using RingBuffer<T, N>::emplace;
};

template <typename T, size_t N = ARX_DEQUE_DEFAULT_SIZE>
struct deque : public RingBuffer<T, N> {
    using iterator = typename RingBuffer<T, N>::iterator;
    using const_iterator = typename RingBuffer<T, N>::const_iterator;

    deque()
    : RingBuffer<T, N>() {}
    deque(std::initializer_list<T> lst)
    : RingBuffer<T, N>(lst) {}

    // copy
    deque(const deque& r)
    : RingBuffer<T, N>(r) {}

    deque& operator=(const deque& r) {
        RingBuffer<T, N>::operator=(r);
        return *this;
    }

    // move
    deque(deque&& r)
    : RingBuffer<T, N>(r) {}

    deque& operator=(deque&& r) {
        RingBuffer<T, N>::operator=(r);
        return *this;
    }

    virtual ~deque() {}

private:
    using RingBuffer<T, N>::capacity;
    using RingBuffer<T, N>::pop;
    using RingBuffer<T, N>::push;
    using RingBuffer<T, N>::emplace;
    using RingBuffer<T, N>::assign;
    using RingBuffer<T, N>::begin;
    using RingBuffer<T, N>::end;
    using RingBuffer<T, N>::fill;
};

template <class T1, class T2>
struct pair {
    T1 first;
    T2 second;
};

template <class T1, class T2>
pair<T1, T2> make_pair(const T1& t1, const T2& t2) {
    return {t1, t2};
};

template <typename T1, typename T2>
bool operator==(const pair<T1, T2>& x, const pair<T1, T2>& y) {
    return (x.first == y.first) && (x.second == y.second);
}

template <typename T1, typename T2>
bool operator!=(const pair<T1, T2>& x, const pair<T1, T2>& y) {
    return !(x == y);
}

template <class Key, class T, size_t N = ARX_MAP_DEFAULT_SIZE>
struct map : public RingBuffer<pair<Key, T>, N> {
    using iterator = typename RingBuffer<pair<Key, T>, N>::iterator;
    using const_iterator = typename RingBuffer<pair<Key, T>, N>::const_iterator;
    using base = RingBuffer<pair<Key, T>, N>;

    map()
    : base() {}
    map(std::initializer_list<pair<Key, T> > lst)
    : base(lst) {}

    // copy
    map(const map& r)
    : base(r) {}

    map& operator=(const map& r) {
        base::operator=(r);
        return *this;
    }

    // move
    map(map&& r)
    : base(r) {}

    map& operator=(map&& r) {
        base::operator=(r);
        return *this;
    }

    virtual ~map() {}

    const_iterator find(const Key& key) const {
        for (size_t i = 0; i < this->size(); ++i) {
            const_iterator it = this->begin() + i;
            if (key == it->first)
                return it;
        }
        return this->end();
    }

    iterator find(const Key& key) {
        for (size_t i = 0; i < this->size(); ++i) {
            iterator it = this->begin() + i;
            if (key == it->first)
                return it;
        }
        return this->end();
    }

    pair<iterator, bool> insert(const Key& key, const T& t) {
        bool b {false};
        iterator it = find(key);
        if (it == this->end()) {
            this->push(make_pair(key, t));
            b = true;
            it = this->begin() + this->size() - 1;
        }
        return {it, b};
    }

    pair<iterator, bool> insert(const pair<Key, T>& p) {
        bool b {false};
        iterator it = find(p.first);
        if (it == this->end()) {
            this->push(p);
            b = true;
            it = this->begin() + this->size() - 1;
        }
        return {it, b};
    }

    pair<iterator, bool> emplace(const Key& key, const T& t) {
        return insert(key, t);
    }

    pair<iterator, bool> emplace(const pair<Key, T>& p) {
        return insert(p);
    }

    const T& at(const Key& key) const {
        // iterator it = find(key);
        // if (it != this->end()) return it->second;
        // return T();
        return find(key)->second;
    }

    T& at(const Key& key) {
        // iterator it = find(key);
        // if (it != this->end()) return it->second;
        // return T();
        return find(key)->second;
    }

    iterator erase(const iterator& it) {
        iterator i = (iterator)find(it->first);
        return base::erase(i);
    }

    iterator erase(const Key& key) {
        iterator i = find(key);
        return base::erase(i);
    }

    iterator erase(const size_t index) {
        if (index < this->size()) {
            iterator it = this->begin() + index;
            erase(it);
        }
        return this->end();
    }

    T& operator[](const Key& key) {
        iterator it = find(key);
        if (it != this->end()) return it->second;

        insert(::arx::make_pair(key, T()));
        return this->back().second;
    }

private:
    using base::assign;
    using base::back;
    using base::capacity;
    using base::data;
    using base::emplace_back;
    using base::front;
    using base::pop;
    using base::pop_back;
    using base::pop_front;
    using base::push;
    using base::push_back;
    using base::push_front;
    using base::resize;
    using base::shrink_to_fit;
    using base::fill;
};

}  // namespace arx

template <typename T, size_t N>
using ArxRingBuffer = arx::RingBuffer<T, N>;

#endif  // Do not have libstdc++11
#endif  // ARX_RINGBUFFER_H
