#pragma once

#include <initializer_list>
#include <exception>
#include <cmath>
#include <string>
#include <cassert>
#include <iomanip>
#include <random>
#include <chrono>

#include "Buf.h"

namespace linal {

    constexpr double eps = 0.0000001;
    constexpr double tests_eps = 0.01; // acceptable eps


    template<typename T = double>
class Matrix final : private Buf<T> {

    using Buf<T>::data_;
    using Buf<T>::used_;
    using Buf<T>::size_;
    using Buf<T>::buf;

    public:


        Matrix();
        Matrix(size_t rows, size_t columns, T value = T{});
        Matrix(size_t rows, size_t columns, const std::initializer_list<T> &elems);
        Matrix(const std::initializer_list<std::initializer_list<T>> &elems);


        const T& at(size_t i, size_t j) const&;
        T& at(size_t i, size_t j) &;

        size_t GetRows() const noexcept { return rows_; };
        size_t GetColumns() const noexcept { return columns_; };
        std::pair<size_t, size_t> GetRowsAndColumns() const noexcept { return std::make_pair(rows_, columns_); };
        size_t GetSize() const noexcept {return size_;};
        size_t GetCapacity() const noexcept {return used_;};
        void Print() const;

        void resize(size_t rows, size_t columns);
        void clear();

        template <typename U> void Copy(const Matrix<U>& m);
        template <typename U> explicit Matrix(const Matrix<U> &m);
        Matrix(const Matrix& m) ;
        Matrix& operator=(const Matrix& m);

        Matrix(Matrix&& m) ;
        Matrix& operator= (Matrix&& m) ;


        Matrix& operator += (const Matrix& m)&;       
        Matrix& operator -= (const Matrix& m)&;
        Matrix& operator *= (const T& number)&;
        Matrix  operator - () const&;

        Matrix& multiply(const Matrix& m)&;

        template <typename U>
        bool operator == (const Matrix<U>& m) const;
        template <typename U>
        bool operator != (const Matrix<U>& m) const;

        void SwapRows(size_t r1, size_t r2);
        void SwapColumns(size_t c1, size_t c2);
        Matrix GetMinor(size_t row, size_t col) const;

        double determinant() const;
        double determinantGaus() const;


        T trace() const;
        Matrix& negate()&;
        Matrix& transpose()&;

        const T* data() const {return buf();};

        ~Matrix() = default;

    private:

        size_t rows_{}, columns_{};

    };

    void RandomFill(Matrix<int> &m, int d1, int d2);
    void RandomFill(Matrix<double> &m, int d1, int d2);

    template<typename T>
    std::istream& operator>>(std::istream& str, Matrix<T>& m) {

        int r;
        str>>r;
        m.resize(r,r);

        for (int i = 0; i < r; i++) {
            for (int j = 0; j < r; j++) {
                str >> m.at(i, j);
                if (!str.good())
                    throw std::invalid_argument("wrong argument");
            }
        }

        return str;
    }

    //................Constructor..........................

    template<typename T>
    Matrix<T>::Matrix(): Buf<T>(0) {};

    template<typename T>
    Matrix<T>::Matrix(size_t rows, size_t columns, T value) :
            Buf<T>(rows * columns), rows_(rows), columns_(columns) {

        if(rows_ < 0 || columns_ < 0)
            throw std::invalid_argument("the dimensions of the matrix must be positive");

        for (;used_ < size_; ++used_)
            new (data_ + used_) T (value);

    }

    template<typename T>
    Matrix<T>::Matrix(size_t rows, size_t columns, const std::initializer_list<T> &elems) :
            Buf<T>(rows*columns), rows_(rows), columns_(columns) {

        if(rows_ < 0 || columns_ < 0)
            throw std::invalid_argument("the dimensions of the matrix must be positive");

        const size_t size = elems.size();

        if(size_ < size)
            throw std::invalid_argument("number of elements is more than matrix' size");
        else {

            int i = 0;
            for (const auto& elem : elems) {
                new (data_ + i) T (elem);
                i++;
            }

            for (; i < size_; i++)
                new (data_ + i) T();
        }

    }

    template<typename T>
    Matrix<T>::Matrix(const std::initializer_list<std::initializer_list<T>> &elems) :
           Buf<T>(0) ,rows_(elems.size()) {

        size_t columns = 0;

        for (const auto& row : elems)
            columns = std::max(columns, row.size());

        columns_ = columns;

        Matrix<T> tmp(rows_, columns_);
        Buf<T>::swap(tmp);

        int i = 0;
        for (const auto& str : elems) {
            int j = 0;
            for (const auto& elem : str) {
                new(&at(i, j)) T(elem);
                j++;
            }

            for (; j < columns_; j++)
                new(&at(i, j)) T();

            i++;
        }

    }

    //................Constructor_end......................

    template <typename T>
    T& Matrix<T>::at(size_t i, size_t j)&
    {
        return const_cast<T&>(static_cast<const Matrix<T>*>(this)->at(i, j));
    }


    template <typename T>
    const T& Matrix<T>::at(size_t i, size_t j) const&
    {
        if (i >= rows_ || j >= columns_)
            throw std::out_of_range("Out of range");
        else
            return data_[i * columns_ + j];
    }


    //................Copy.................................

    template <typename T>
    template <typename U>
    void Matrix<T>::Copy(const Matrix<U>& m)
    {
        const size_t min_row = std::min(rows_, m.GetRows());
        const size_t min_col = std::min(columns_, m.GetColumns());

        Matrix<T> tmp(rows_, columns_);


        for (size_t i = 0; i < min_row; i++)
        {
            for (size_t k = 0; k < min_col; k++) {
                new(&tmp.at(i, k)) T(m.at(i, k));
            }
        }

        for (size_t i = min_row; i < rows_; i++)
        {
            for (size_t k = min_col; k < columns_; k++)
                new (&tmp.at(i, k)) T ();
        }

        Buf<T>::swap(tmp);
    }

    template <typename T>
    template <typename U>
    Matrix<T>::Matrix(const Matrix<U> &m)
            : Matrix(m.GetRows(), m.GetColumns()) {

        Copy(m);
    }

    template <typename T>
    Matrix<T>::Matrix(const Matrix& m)
            : Matrix(m.rows_, m.columns_)
    {
        Copy(m);
    }

    template <typename T>
    Matrix<T>& Matrix<T>::operator = (const Matrix& m)
    {
        if (this == &m)
            return *this;

        resize(m.rows_, m.columns_);
        Copy(m);

        return *this;
    }


    //..................Copy_end........................

    template <typename T>
    Matrix<T>::Matrix(Matrix&& m)
            : Matrix()
    {
        rows_ = m.rows_;
        columns_ = m.columns_;
        Buf<T>::swap(m);
    }

    template <typename T>
    Matrix<T>& Matrix<T>::operator= (Matrix&& m)  {

        if (this == &m)
            return *this;

        rows_ = m.rows_;
        columns_ = m.columns_;
        Buf<T>::swap(m);
        return *this;
    }

    template <typename T>
    void Matrix<T>::resize(size_t rows, size_t columns) {
        if (rows < 0 || columns < 0)
            throw std::invalid_argument("the dimensions of the matrix must be positive");

        if (!rows || !columns) {
            clear();
            return;
        }

        if (!data_) {
            *this = std::move(Matrix<T>(rows, columns));
            return;
        }


        if (columns_ == columns && columns * rows <= used_) {

            for (size_t i = rows; i < rows_; i++) {
                for (size_t j = 0; j < columns_; j++)
                {
                    at(i, j).~T();
                    used_--;
                }
            }

            rows_ = rows;
        }
        else if (rows_ == rows && columns_ >= columns) {

            for (size_t i = 0; i < rows_; i++) {
                for (size_t j = columns; j < columns_; j++)
                {
                    at(i, j).~T();
                    used_--;
                }
            }
            columns_ = columns;
        }
        else {
            Matrix<T> tmp(rows, columns);
            tmp.Copy(*this);
            *this = std::move(tmp);
        }

    }


    // (+=, -=, *=, =, -, *).............................


    template <typename T>
    Matrix<T>& Matrix<T>::operator += (const Matrix& m)& {

        if (rows_ != m.rows_ || columns_ != m.columns_)
            throw std::invalid_argument("the dimensions of the matrices must be the same");

        for (size_t i = 0; i < m.rows_; i++)
        {
            for (size_t j = 0; j < m.columns_; j++)
                at(i, j) += m.at(i, j);
        }
        
        return *this;
    }

    template <typename T>
    Matrix<T>& Matrix<T>::operator -= (const Matrix<T>& m)& {

        if (rows_ != m.rows_ || columns_ != m.columns_)
            throw std::invalid_argument("the dimensions of the matrices must be the same");

        for (size_t i = 0; i < m.rows_; i++)
        {
            for (size_t j = 0; j < m.columns_; j++)
                at(i, j) -= m.at(i, j);
        }
        return  *this;
    }


    template <typename T>
    Matrix<T>& Matrix<T>::operator *= (const T& number)& {

        for (size_t i = 0; i < rows_; i++)
        {
            for (size_t j = 0; j < columns_; j++)
                at(i, j) *= number;
        }
    }


    template <typename T>
    Matrix<T> operator + (const Matrix<T>& l, const Matrix<T>& r)
    {
        Matrix<T> res(l);
        res += r;
        return res;
    }

    template <typename T>
    Matrix<T> operator + (const Matrix<T>& l, Matrix<T>&& r)
    {
        Matrix<T> res(std::move(r));
        r += l;
        return res;
    }
    template <typename T>
    Matrix<T> operator + (Matrix<T>&& l, const Matrix<T>& r)
    {
        Matrix<T> res(std::move(l));
        res += r;
        return res;
    }
    template <typename T>
    Matrix<T> operator + (Matrix<T>&& l, Matrix<T>&& r)
    {
        Matrix<T> res(std::move(l));
        res += r;
        return res;
    }



    template <typename T>
    Matrix<T> operator - (const Matrix<T>& l, const Matrix<T>& r)
    {
        Matrix<T> res(l);
        res -= r;
        return res;
    }
    template <typename T>
    Matrix<T> operator - (const Matrix<T>& l, Matrix<T>&& r)
    {
        Matrix<T> res(std::move(r));
        res -= l;
        return res;
    }
    template <typename T>
    Matrix<T> operator - (Matrix<T>&& l, const Matrix<T>& r)
    {
        Matrix<T> res(std::move(l));
        res -= r;
        return res;
    }
    template <typename T>
    Matrix<T> operator - (Matrix<T>&& l, Matrix<T>&& r)
    {
        Matrix<T> res(std::move(l));
        res -= r;
        return res;
    }


    template <typename T>
    Matrix<T> operator * (const Matrix<T>& m, const T& number)
    {
        Matrix<T> res(m);
        res.mul(number);
        return res;
    }
    template <typename T>
    Matrix<T> operator * (Matrix<T>&& m, const T& number)
    {
        Matrix<T> res(std::move(m));
        res.mul(number);
        return res;
    }
    template <typename T>
    Matrix<T> operator * (const T& number, const Matrix<T>& m)
    {
        Matrix<T> res(m);
        res.mul(number);
        return res;
    }
    template <typename T>
    Matrix<T> operator * (const T& number, Matrix<T>&& m)
    {
        Matrix<T> res(std::move(m));
        res.mul(number);
        return res;
    }


    template <typename T>
    Matrix<T>& Matrix<T>::multiply(const Matrix& m)&
    {

        if (columns_ != m.rows_)
            throw std::logic_error("matrix sizes are not valid for their composition");

        Matrix<T> res(rows_, m.columns_);

        for (int r = 0; r < rows_; r++)
        {
            for (int c = 0; c < m.columns_; c++)
            {
                T cur_res = T{};

                for (int j = 0; j < columns_; j++)
                    cur_res += at(r, j) * m.at(j, c);

                res.at(r,c) = cur_res;
            }
        }

        *this = std::move(res);

        return *this;
    }

    template <typename T>
    Matrix<T> operator * (const Matrix<T>& l, const Matrix<T>& r)
    {
        Matrix<T> res(l);
        res.multiply(r);
        return res;
    }
    template <typename T>
    Matrix<T> operator * (const Matrix<T>& l, Matrix<T>&& r)
    {
        Matrix<T> res(std::move(r));
        res.multiply(l);
        return res;
    }
    template <typename T>
    Matrix<T> operator * (Matrix<T>&& l, const Matrix<T>& r)
    {
        Matrix<T> res(std::move(l));
        res.multiply(r);
        return res;
    }
    template <typename T>
    Matrix<T> operator * (Matrix<T>&& l, Matrix<T>&& r)
    {
        Matrix<T> res(std::move(l));
        res.multiply(r);
        return res;
    }

    // (+=, -=, *=, =, -, *).......................end


    template<typename T>
    Matrix<T> Matrix<T>::operator -() const& {

        Matrix<T> res(*this);
        res.negate();

        return res;
    }

    template <typename T>
    template <typename U>
    bool Matrix<T>::operator == (const Matrix<U>& m) const {

        if (columns_ != m.GetColumns() || rows_ != m.GetRows())
            return false;

        for (size_t i = 0; i < rows_; i++) {
            for (size_t j = 0; j < columns_; j++) {

                if (at(i, j) != m.at(i, j))
                    return false;
            }
        }

        return true;
    }

    template <typename T>
    template <typename U>
    bool Matrix<T>::operator != (const Matrix<U>& m) const {
        return !(*this == m);
    }




    template <typename T>
    void Matrix<T>::SwapRows(size_t r1, size_t r2) {

        for (size_t j = 0; j < columns_; j++)
            std::swap(at(r1, j), at(r2, j));
    }


    template <typename T>
    void Matrix<T>::SwapColumns(size_t c1, size_t c2) {

        for (size_t i = 0; i < rows_; i++)
            std::swap(at(i, c1), at(i, c2));
    }

    template <typename T>
    T Matrix<T>::trace() const {

        if (rows_ != columns_)
            throw std::logic_error("Only the square matrix has a trace");

        if (!rows_)
            return T{};

        T res = at(0, 0);

        for (int i = 1; i < rows_; i++)
            res *= at(i, i);

        return res;
    }

    template <typename T>
    Matrix<T>& Matrix<T>::transpose()& {

        if (!rows_ || !columns_)
            return *this;

        size_t r = rows_;
        size_t c = columns_;

        if (r >= c)
            resize(r, r);
        else
            resize(c, c);

        for (int i = 0; i < rows_; i++) {
            for (int j = i + 1; j < columns_; j++)
                std::swap(at(i, j), at(j,i));
        }

        if (r >= c)
            resize(c, r);
        else
            resize(r, c);

        return *this;
    }


    template <typename T>
    Matrix<T> Matrix<T>::GetMinor(size_t row, size_t col) const {

        if (!size_)
            throw std::logic_error("the null matrix has no submatrix");


        Matrix<T> res(columns_ - 1, rows_ - 1);

        for (int i = 0, i_res = 0; i < rows_; i++) {

            if (i == row)
                continue;

            for (int j = 0, j_res = 0; j < columns_; j++) {

                if (j == col)
                    continue;

                res.at(i_res, j_res) = at(i, j);
                j_res++;
            }
            i_res++;
        }

        return res;
    }

    template <typename T>
    Matrix<T>& Matrix<T>::negate()& {

        for (int i = 0; i < size_; i++)
            data_[i] = -data_[i];

        return *this;
    }


    template <typename T>
    double Matrix<T>::determinant() const {

        double res = 0.0;

        if (rows_ != columns_)
            throw std::logic_error("Only the square matrix has a determinant");

        if (rows_ == 1)
            return at(0, 0);

        for (int j = 0; j < rows_; j++) {

            const int k = (j % 2) ? -1 : 1;
            res += at(0, j) * (GetMinor(0, j).determinant()) * k;
        }

        return res;
    }


    template <typename T>
    double Matrix<T>::determinantGaus() const {

        if (rows_ != columns_)
            throw std::logic_error("Only the square matrix has a determinant");

        Matrix<double> copy_m(*this);

        bool sign = false;
        for (int i = 0; i < rows_; i++) {

            for (int ii = i; ii < rows_; ii++) {
                if (std::abs(copy_m.at(ii, i)) > eps) {
                    if (ii != i) {
                        copy_m.SwapRows(i, ii);
                        sign = !sign;
                    }
                    break;
                }
            }

            if (std::abs(copy_m.at(i, i)) < eps)
                return 0.0;

            auto elem = static_cast<double>(copy_m.at(i, i));
            for (int r = i + 1; r < rows_; r++) {

                const double k = copy_m.at(r, i) / elem;
                for (int j = 0; j < rows_; j++)
                    copy_m.at(r, j) -= copy_m.at(i, j) * k;
            }
        }
        const int res_sign = (sign) ? -1 : 1;

        return copy_m.trace() * res_sign;
    }

    template<typename T>
    void Matrix<T>::clear() {
        *this = Matrix<T>();
    }

    template<typename T>
    void Matrix<T>::Print() const {

        std::cout<<"________________________________________\n";
        std::cout<<"rows: "<<rows_<<" columns: "<<columns_<<"\n";
        std::cout<<"size: "<<size_<<" used: "<<used_<<"\n\n";

        for (int i = 0; i < rows_; i++) {
            for (int j = 0; j < columns_; j++) {
                std::cout<<at(i,j)<<" ";
            }
            std::cout<<std::endl;
        }
        std::cout<<std::endl;
        std::cout<<"________________________________________\n";
    }

    template<typename T>
    std::ostream& operator<<(std::ostream& os, const Matrix<T>& m) {

        os << m.GetRows()<<" "<<m.GetColumns()<<"\n";

        for (int i = 0; i < m.GetRows(); i++) {
            for (int j = 0; j < m.GetColumns(); j++) {
                os<<m.at(i,j)<<" ";
            }
            os<<"\n";
        }
        return os;
    }


}
