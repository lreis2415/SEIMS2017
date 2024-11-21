/*!
 * \file utils_array.h
 * \brief Template functions to initialize and release arrays.
 *
 * \remarks
 *   - 1. 2018-05-02 - lj - Make part of CCGL.
 *   - 2. 2021-07-20 - lj - Initialize 2D array in a succesive memory.
 *
 * \author Liangjun Zhu, zlj(at)lreis.ac.cn
 * \version 1.1
 */
#ifndef CCGL_UTILS_ARRAY_H
#define CCGL_UTILS_ARRAY_H

#include <new> // std::nothrow
#include <cstdarg> // variable arguments
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include "basic.h"

using std::vector;
using std::cout;
using std::endl;
using std::nothrow;

namespace ccgl {
/*!
 * \namespace ccgl::utils_array
 * \brief Array related functions include vector and pointer array.
 */
namespace utils_array {
/*!
 * \brief Initialize DT_Array1D data
 * \param[in] row
 * \param[in] data
 * \param[in] init_value
 * \return True if succeed, else false and the error message will print as well.
 */
template <typename T, typename INI_T>
bool Initialize1DArray(int row, T*& data, INI_T init_value);

/*!
 * \brief Initialize DT_Array1D data based on an existed array
 * \param[in] row
 * \param[in] data
 * \param[in] init_data
 * \return True if succeed, else false and the error message will print as well.
 */
template <typename T, typename INI_T>
bool Initialize1DArray(int row, T*& data, INI_T* init_data);

template <typename T, typename INI_T>
bool Initialize1DArray4ItpWeight(int row, T*& data, INI_T* init_data, int itp_weight_data_length);
/*!
 * \brief Initialize DT_Array2D data
 *
 * The 2D array are created in a successive memory.
 * 1. Create a 1D array of row data pointers with the length of row
 * 2. Create a 1D array of data pool with the length of row * col
 * 3. Iteratively point row pointers to appropriate positions in data pool
 *
 * Refers to https://stackoverflow.com/a/21944048/4837280
 *
 * \param[in] row
 * \param[in] col
 * \param[in] data
 * \param[in] init_value
 * \return True if succeed, else false and the error message will print as well.
 */
template <typename T, typename INI_T>
bool Initialize2DArray(int row, int col, T**& data, INI_T init_value);

/*!
 * \brief Initialize DT_Array2D data based on an existed array
 * The usage of `const T * const *` is refers to http://blog.csdn.net/pmt123456/article/details/50813564
 * \param[in] row
 * \param[in] col
 * \param[in] data
 * \param[in] init_data dimension MUST BE (row, col)
 * \return True if succeed, else false and the error message will print as well.
 */
template <typename T, typename INI_T>
bool Initialize2DArray(int row, int col, T**& data, INI_T** init_data);

/*!
 * \brief Initialize irregular DT_Array2D data based on an existed 1D array
 * \param[in] init_data Initial 1D array
 * \param[out] rows Rows count
 * \param[out] max_cols Maximum cols count
 * \param[out] data Irregular 2D array
 * \return True if succeed, else false and the error message will print as well.
 */
template <typename T1, typename T2>
bool Initialize2DArray(T1* init_data, int& rows, int& max_cols, T2**& data);

/*!
 * \brief Release DT_Array1D data
 * \param[in] data
 */
template <typename T>
void Release1DArray(T*& data);

/*!
 * \brief Release DT_Array2D data
 * \param[in] data
 */
template <typename T>
void Release2DArray(T**& data);

/*!
 * \brief Batch release of 1D array
 *        Variable arguments with the end of `nullptr`.
 *
 *        The input parameters are listed as `data`, `data2`, ... , `dataN`, and ended with `nullptr`.
 *
 * Example:
 * \code
 *   BatchRelease1DArray(array1, array2, array3, nullptr);
 * \endcode
 *
 * \warning After batch release, the variable will not be set to nullptr.
 *          So, do not use these variables any more.
 *          BTW, this function will not cause memory leak.
 *
 *          USE WITH ALL CAUTIONS CLEARLY AWARED.
 */
template <typename T>
void BatchRelease1DArray(T*& data, ...);

/*!
 * \brief Batch release of 2D array, \sa BatchRelease1DArray
 *        Variable arguments with the end of nullptr.
 *
 * Example:
 * \code
 *   BatchRelease2DArray(rows, array1, array2, array3, nullptr);
 * \endcode
 *
 * \param[in] nrows Rows
 * \param[in] data The input parameters are listed as `data`, `data2`, ... , `dataN`, and ended with `nullptr`.
 * \warning USE WITH ALL CAUTIONS CLEARLY AWARED.
 */
template <typename T>
void BatchRelease2DArray(int nrows, T**& data, ...);

/*!
 * \brief Write 1D array to a file
 * \sa Read1DArrayFromTxtFile(), Read2DArrayFromTxtFile(), Output2DArrayToTxtFile()
 * \param[in] n, data, filename
*/
void Output1DArrayToTxtFile(int n, const float* data, const char* filename);

/*!
 * \brief Write 2D array to a file
 * \sa Read1DArrayFromTxtFile(), Read2DArrayFromTxtFile(), Output1DArrayToTxtFile()
 * \param[in] rows, cols, data, filename
 */
void Output2DArrayToTxtFile(int rows, int cols, const float** data, const char* filename);

/*!
 * \brief Read 1D array from file
 *        The input file should follow the format:
 *           a 1D array sized rows * 1
 *
 *        The size of data is rows
 *
 * \sa Read2DArrayFromTxtFile(), Output1DArrayToTxtFile(), Output2DArrayToTxtFile()
 * \param[in] filename
 * \param[out] rows, data
 */
template <typename T>
void Read1DArrayFromTxtFile(const char* filename, int& rows, T*& data);

/*!
 * \brief Read 2D array from file
 *        The input file should follow the format:
 *            a 2D array sized rows * rows
 *
 *        The size of data is rows * (rows + 1), the first element of each row is the rows
 *
 * \sa Read1DArrayFromTxtFile(), Output1DArrayToTxtFile(), Output2DArrayToTxtFile()
 * \param[in] filename
 * \param[out] rows, data
 */
template <typename T>
void Read2DArrayFromTxtFile(const char* filename, int& rows, T**& data);


/*!
 * \brief Read csv.
 *
 * \param[in] filename
 * \param[in] header Only the columns in the header will be read and stored in data.
 *                      The order of the data is the same as the header.
 * \param[out] data
 */
template <typename T>
bool Read2DArrayFromCsvFile(const char* filename, const vector<string>* header, std::vector<std::vector<T>>& data);

/*!
 * \brief Read 2D array from string
 *        The input string should follow the format:
 *            float value, total number is rows * rows
 *
 *        The size of data is rows * (rows + 1), the first element of each row is the rows.
 *
 * \param[in] s
 * \param[out] rows, data
 */
template <typename T>
void Read2DArrayFromString(const char* s, int& rows, T**& data);

/*!
 * \brief If value in vector container
 * \param[in] val Value, e.g., a int, or float
 * \param[in] vec Vector container, data type is consistent with val
 * \return True if val is in vec, otherwise False
 */
template <typename T>
bool ValueInVector(T val, const vector<T>& vec);

/*!
 * \brief Remove value in vector container
 * \param[in] val Value to be removed, e.g., a int, or float
 * \param[in] vec Vector container, data type is consistent with val
 */
template <typename T>
void RemoveValueInVector(T val, vector<T>& vec);

/*!
 * \brief Rudimentary RAII class of 2D Array which occupy successive memory
 *
 * Currently not used in CCGL, but maybe in future!
 *
 * Refers to:
 *   origin implementation: https://stackoverflow.com/a/21944048/4837280 and
 *   memory leak fixed: https://stackoverflow.com/a/58309862/4837280
 */
template <typename T>
class Array2D {
    T** data_ptr;
    vuint32_t m_rows;
    vuint32_t m_cols;

    T** create2DArray(vuint32_t nrows, vuint32_t ncols, const T& val = T()) {
        T** ptr = nullptr;
        T* pool = nullptr;
        try {
            ptr = new(nothrow) T*[nrows];  // allocate pointers (Do not throw here)
            pool = new(nothrow) T[nrows*ncols];  // allocate pool (Do not throw here)
            for (vuint32_t i = 0; i < nrows * ncols; i++) {
                pool[i] = val;
            }
            // now point the row pointers to the appropriate positions in the memory pool
            for (vuint32_t i = 0; i < nrows; ++i, pool += ncols) {
                ptr[i] = pool;
            }
            return ptr;
        } catch (std::bad_alloc& ex) {
            delete[] ptr; // either this is nullptr or it was allocated
            // throw ex;  // memory allocation error
        }
    }

public:
    typedef T value_type;
    T** data() {
        return data_ptr;
    }

    vuint32_t get_rows() const { return m_rows; }

    vuint32_t get_cols() const { return m_cols; }

    Array2D() : data_ptr(nullptr), m_rows(0), m_cols(0) {}
    Array2D(vuint32_t rows, vuint32_t cols, const T& val = T()) {
        if (rows <= 0)
            throw std::invalid_argument("number of rows is 0"); // TODO, DO not throw here
        if (cols <= 0)
            throw std::invalid_argument("number of columns is 0"); // TODO, DO not throw here
        data_ptr = create2DArray(rows, cols, val);
        m_rows = rows;
        m_cols = cols;
    }

    ~Array2D() {
        if (data_ptr) {
            delete[] data_ptr[0];  // remove the pool
            delete[] data_ptr;     // remove the pointers
        }
    }

    Array2D(const Array2D& rhs) : m_rows(rhs.m_rows), m_cols(rhs.m_cols) {
        data_ptr = create2DArray(m_rows, m_cols);
        std::copy(&rhs.data_ptr[0][0], &rhs.data_ptr[m_rows - 1][m_cols], &data_ptr[0][0]);
    }

    Array2D(Array2D&& rhs) NOEXCEPT {
        data_ptr = rhs.data_ptr;
        m_rows = rhs.m_rows;
        m_cols = rhs.m_cols;
        rhs.data_ptr = nullptr;
    }

    Array2D& operator=(Array2D&& rhs) NOEXCEPT {
        if (&rhs != this) {
            swap(rhs, *this);
        }
        return *this;
    }

    void swap(Array2D& left, Array2D& right) {
        std::swap(left.data_ptr, right.data_ptr);
        std::swap(left.m_cols, right.m_cols);
        std::swap(left.m_rows, right.m_rows);
    }

    Array2D& operator = (const Array2D& rhs) {
        if (&rhs != this) {
            Array2D temp(rhs);
            swap(*this, temp);
        }
        return *this;
    }

    T* operator[](vuint32_t row) {
        return data_ptr[row];
    }

    const T* operator[](vuint32_t row) const {
        return data_ptr[row];
    }

    void create(vuint32_t rows, vuint32_t cols, const T& val = T()) {
        *this = Array2D(rows, cols, val);
    }
};


/************ Implementation of template functions ******************/
template <typename T, typename INI_T>
bool Initialize1DArray(const int row, T*& data, const INI_T init_value) {
    if (nullptr != data) {
        //Should allow an array to re-enter this function then just return? --wyj
        //cout << "The input 1D array pointer is not nullptr. No initialization performed!" << endl;
        return false;
    }
    if (row <= 0) {
        cout << "The data length MUST be greater than 0!" << endl;
        data = nullptr;
        return false;
    }
    data = new(nothrow)T[row];
    if (nullptr == data) {
        delete[] data;
        cout << "Bad memory allocated during 1D array initialization!" << endl;
        data = nullptr;
        return false;
    }
    T init = static_cast<T>(init_value);
#pragma omp parallel for
    for (int i = 0; i < row; i++) {
        data[i] = init;
    }
    return true;
}

template <typename T, typename INI_T>
bool Initialize1DArray(const int row, T*& data, INI_T* const init_data) {
    if (nullptr != data) {
        // cout << "The input 1D array pointer is not nullptr. No initialization performed!" << endl;
        return false;
    }
    data = new(nothrow) T[row];
    if (nullptr == data) {
        delete[] data;
        cout << "Bad memory allocated during 1D array initialization!" << endl;
        return false;
    }
    if (nullptr == init_data) {
        cout << "The input parameter init_data MUST NOT be nullptr!" << endl;
        return false;
    }
#pragma omp parallel for
    for (int i = 0; i < row; i++) {
        data[i] = static_cast<T>(init_data[i]);
    }
    return true;
}

template <typename T, typename INI_T>
bool Initialize2DArray(const int row, const int col, T**& data, const INI_T init_value) {
    if (row <= 0 || col <= 0) {
        cout << "The row and col should not be less or equal to ZERO!" << endl;
        return false;
    }
    if (nullptr != data) {
        // cout << "The input 2D array pointer is not nullptr. No initialization performed!" << endl;
        return false;
    }
    data = new(nothrow) T*[row];
    if (nullptr == data) {
        delete[] data;
        cout << "Bad memory allocated during initialize rows of the 2D array!" << endl;
        return false;
    }
    T* pool = nullptr;
    pool = new(nothrow) T[row * col];
    if (nullptr == pool) {
        delete[] pool;
        cout << "Bad memory allocated during initialize data pool of the 2D array!" << endl;
        return false;
    }
    // Initialize the data pool
    T init = static_cast<T>(init_value);
#pragma omp parallel for
    for (int i = 0; i < row * col; i++) {
        pool[i] = init;
    }
    // Now point the row pointers to the appropriate positions in the data pool
    for (int i = 0; i < row; ++i, pool += col) {
        data[i] = pool;
    }
    return true;
}

template <typename T, typename INI_T>
bool Initialize2DArray(const int row, const int col, T**& data,
                       INI_T** const init_data) {
    bool flag = Initialize2DArray(row, col, data, init_data[0][0]);
    if (!flag) { return false; }
#pragma omp parallel for
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < col; j++) {
            data[i][j] = static_cast<T>(init_data[i][j]);
        }
    }
    return true;
}

template <typename T1, typename T2>
bool Initialize2DArray(T1* init_data, int& rows, int& max_cols, T2**& data) {
    int idx = 0;
    rows = CVT_INT(init_data[idx++]);
    data = new(nothrow) T2* [rows];
    if (nullptr == data) {
        delete[] data;
        cout << "Bad memory allocated during initialize rows of the 2D array!" << endl;
        return false;
    }
    T2* pool = nullptr;
    // Get actual data length of init_data, excluding the first element which is 'rows'
    int* cols = new int[rows];
    max_cols = -1;
    for (int i = 0; i < rows; i++) {
        cols[i] = CVT_INT(init_data[idx]);
        idx += cols[i] + 1;
        if (cols[i] > max_cols) { max_cols = cols[i]; }
    }
    int length = idx - 1;
    // New a 1d array to store data
    Initialize1DArray(length, pool, init_data + 1);
    // Now point the row pointers to the appropriate positions in the data pool
    int pos = 0;
    for (int i = 0; i < rows; ++i) {
        data[i] = pool + pos;
        pos += cols[i] + 1;
    }
    delete[] cols;
    return true;
}

template <typename T>
void Release1DArray(T*& data) {
    if (nullptr != data) {
        delete[] data;
        data = nullptr;
    }
}

template <typename T>
void Release2DArray(T**& data) {
    if (nullptr == data) {
        return;
    }
    delete[] data[0]; // delete the memory pool
    delete[] data; // delete row pointers
    data = nullptr;
}

template <typename T>
void BatchRelease1DArray(T*& data, ...) {
    va_list arg_ptr;
    va_start(arg_ptr, data);
    Release1DArray(data);
    T* arg_value = va_arg(arg_ptr, T*);
    while (nullptr != arg_value) {
        Release1DArray(arg_value);
        arg_value = va_arg(arg_ptr, T*);
    }
    va_end(arg_ptr);
}

template <typename T>
void BatchRelease2DArray(const int nrows, T**& data, ...) {
    va_list arg_ptr;
    va_start(arg_ptr, data);
    Release2DArray(nrows, data);
    T** arg_value = va_arg(arg_ptr, T**);
    while (nullptr != arg_value) {
        Release2DArray(nrows, arg_value);
        arg_value = va_arg(arg_ptr, T**);
    }
    va_end(arg_ptr);
}

template <typename T>
bool ValueInVector(const T val, const vector<T>& vec) {
    if (vec.empty()) {
        return false;
    }
    if (find(vec.begin(), vec.end(), val) == vec.end()) {
        return false;
    }
    return true;
}

template <typename T>
void RemoveValueInVector(const T val, vector<T>& vec) {
    for (auto iter = vec.begin(); iter != vec.end();) {
        if (*iter == val) {
            iter = vec.erase(iter);
        } else {
            ++iter;
        }
    }
}

template <typename T>
bool Read2DArrayFromCsvFile(const char* filename, const vector<string>* header, std::vector<std::vector<T>>& data) {
    std::ifstream ifs(filename);
    std::string line;
    if (!ifs.is_open()) {
        std::cerr << "Error: Open file " << filename << " failed!" << std::endl;
        return false;
    }
    vector<int> col_idx;
    if (header != nullptr) {
        int i = 0;
        std::getline(ifs, line);
        std::stringstream ss(line);
        std::string value;
        while (getline(ss, value, ',')) {
            if (ValueInVector(value, *header)) {
                col_idx.push_back(i);
            }
            i++;
        }
    }
    while (std::getline(ifs, line)) {
        int i = 0;
        std::vector<T> row;
        std::stringstream ss(line);
        std::string value;
        while (getline(ss, value, ',')) {
            if (ValueInVector(i, col_idx)) {
                row.push_back(static_cast<T>(std::stod(value)));
            }
            i++;
        }
        data.push_back(row);
    }
    ifs.close();
    return true;
}

} /* utils_array */
} /* namespace: ccgl */

#endif /* CCGL_UTILS_ARRAY_H */
