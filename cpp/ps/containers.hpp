#pragma once
#include <queue>

namespace ps {
namespace containers {

template<typename _Tp,
         typename _Sequence = std::vector<_Tp>,
         typename _Compare = std::less<typename _Sequence::value_type> >
class fixed_priority_queue : public std::priority_queue<_Tp, _Sequence, _Compare> {
public:
    fixed_priority_queue(unsigned int size)
        : fixed_size(size)
        , min_element(this->c.end())
    {}

    fixed_priority_queue(unsigned int size, const _Compare& _comp)
        : std::priority_queue<_Tp, _Sequence, _Compare>(_comp)
        , fixed_size(size)
        , min_element(this->c.end())
    {}

    void push(const _Tp& x)
    {
        if (this->size() == fixed_size)
        {
            if (this->comp(x, *min_element))
                return;

            *min_element = x;
            std::make_heap(this->c.begin(), this->c.end(), this->comp);
        }
        else
            std::priority_queue<_Tp,_Sequence,_Compare>::push(x);

        min_element = std::min_element(this->c.begin(), this->c.end(), this->comp);
    }

    _Sequence& steal()
    {
        return this->c;
    }

    const _Tp& minimum() const
    {
        return *min_element;
    }

    bool full() const
    {
        return this->size() == fixed_size;
    }

private:
    fixed_priority_queue() {}
    const unsigned int fixed_size;
    typename _Sequence::iterator min_element;
};

template<typename _Tp,
         typename _Sequence = std::vector<_Tp>,
         typename _Compare = std::less<typename _Sequence::value_type> >
class reinsertable_priority_queue : public std::priority_queue<_Tp, _Sequence, _Compare> {
public:
    _Tp& top()
    {
        return this->c.front();
    }

    void reinsert()
    {
        std::pop_heap(this->c.begin(), this->c.end(), this->comp);
        std::push_heap(this->c.begin(), this->c.end(), this->comp);
    }
};


template<typename _Tp,
         typename _Sequence = std::vector<_Tp>,
         typename _Compare = std::less<typename _Sequence::value_type>,
         typename _Set = std::unordered_set<_Tp, _Compare> >
class unique_fixed_priority_queue : public std::priority_queue<_Tp, _Sequence, _Compare> {
public:
    unique_fixed_priority_queue(unsigned int size, _Tp _sentinel)
        : fixed_size(size)
        , sentinel(_sentinel)
        , min_element(this->c.end())
    {}

    unique_fixed_priority_queue(unsigned int size, _Tp _sentinel, const _Compare& _comp)
        : std::priority_queue<_Tp, _Sequence, _Compare>(_comp)
        , fixed_size(size)
        , sentinel(_sentinel)
        , min_element(this->c.end())
    {}

    void push(const _Tp& x)
    {
        try_push(x);
    }

    bool try_push(const _Tp& x)
    {
        // Sentinel does not count as insertion
        if (PS_UNLIKELY(sentinel == x))
            return true;

        if (PS_UNLIKELY(contained.find(x) != contained.end()))
            return true;

        if (this->size() == fixed_size)
        {
            if (this->comp(x, *min_element))
                return false;

            contained.erase(contained.find(*min_element));

            *min_element = x;
            std::make_heap(this->c.begin(), this->c.end(), this->comp);
        }
        else
            std::priority_queue<_Tp,_Sequence,_Compare>::push(x);

        contained.insert(x);
        min_element = std::min_element(this->c.begin(), this->c.end(), this->comp);
        return true;
    }

    _Sequence& steal()
    {
        return this->c;
    }

    _Tp get_sentinel() const
    {
        return sentinel;
    }

    const _Tp& minimum() const
    {
        return *min_element;
    }

    bool full() const
    {
        return this->size() == fixed_size;
    }

private:
    unique_fixed_priority_queue() {}
    const unsigned int fixed_size;
    _Tp sentinel;
    _Set contained;
    typename _Sequence::iterator min_element;
};

}
}
