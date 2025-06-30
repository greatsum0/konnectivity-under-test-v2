#pragma once
#include <array>

namespace kut::data::ring_queue {
    template<class T, std::size_t N>
    class ring_queue {
    public:
        template<class U>
        bool push(U &&m) {
            if (size_ == N) return false; // queue full → drop or signal
            buf_[(head_ + size_) % N] = std::forward<U>(m);;
            ++size_;
            return true;
        }

        T &operator[](std::size_t i) {
            return buf_[(head_ + i) % N];
        }

        [[nodiscard]] std::size_t size() const {
            return size_;
        }

        [[nodiscard]] bool empty() const {
            return size_ == 0;
        }

        void pop() // remove front element
        {
            head_ = (head_ + 1) % N;
            --size_;
        }

    private:
        std::array<T, N> buf_{};
        std::size_t head_{0};
        std::size_t size_{0};
    };
}
