#ifndef UTIL_HPP_
#define UTIL_HPP_

#include <chrono>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <limits>
#include <new>
#include <vector>

using std::vector;

namespace Util {
    void startTimer();
    double endTimer();
    double average(const vector<double> &v);
    double stddev(const vector<double> &v);

    const bool is_power2(const unsigned long int x);
    void attach_debugger(bool condition);

    /** Zip through two iterators and call a function */
    template<class Iter1, class Iter2, typename F>
        void zip(
            Iter1 a, Iter1 a_end,
            Iter2 b, Iter2 b_end,
            F f) {
        while(a != a_end && b != b_end) {
            f(*a, *b);
            a++; b++;
        }

    }

    template<class T>
    struct AlignedAllocator
    {
        typedef T value_type;

        // default constructor
        AlignedAllocator () =default;

        // copy constructor
        template <class U> constexpr AlignedAllocator (const AlignedAllocator<U>&) noexcept {}

        T* allocate(std::size_t n) {
            if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
                throw std::bad_array_new_length();

            if (auto p = static_cast<T*>(std::aligned_alloc(32, n * sizeof(T)))) {
                return p;
            }

            throw std::bad_alloc();
        }

        void deallocate(T* p, std::size_t n) noexcept {
            std::free(p);
        }
    };
}

#endif
