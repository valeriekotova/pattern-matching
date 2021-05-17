#pragma once
#include <iostream>

namespace linal {

    template<typename T>
    class Buf {

    protected:

        Buf(size_t size) {

            if (size < 0)
                throw std::invalid_argument("Bad size (size < 0)");

            if (size)
                data_ = static_cast<T *> (::operator new[](sizeof_ * size));
            else
                data_ = nullptr;

            size_ = size;
            used_ = 0;
        }

        Buf(Buf &&buf) noexcept: Buf(0) {
            swap(buf);
        }

        Buf &operator=(Buf &&buf) noexcept {
            swap(std::move(buf));
        }

        ~Buf() {
            for (int i = 0; i < used_; ++i)
                (data_ + i)->~T();

            operator delete[](data_);
        }

        const T* buf() const {return data_;}

        void swap(Buf &buf) {
            std::swap(data_, buf.data_);
            std::swap(size_, buf.size_);
            std::swap(used_, buf.used_);
            std::swap(sizeof_, buf.sizeof_);
        }

        T *data_ = nullptr;
        size_t size_ = 0, used_ = 0;
        size_t sizeof_ = sizeof(T);

        size_t GetSize() noexcept {return size_;};
        size_t GetUsed() noexcept {return used_;};

    public:

        Buf() = delete;
        Buf(const Buf& buf) = delete;
        Buf& operator=(const Buf& buf) = delete;;

    };

}