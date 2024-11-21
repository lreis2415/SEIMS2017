/*!
 * \file utils_string.h
 * \brief Handling string related issues in CCGL.
 *
 * \remarks
 *   - 1. 2018-05-02 - lj - Make part of CCGL.
 *   - 2. 2018-11-12 - lj - Add check and conversion between string and number (int, float, double)
 *
 * \author Liangjun Zhu, zlj(at)lreis.ac.cn
 * \version 1.1
 */
#ifndef CCGL_UTILS_STRING_H
#define CCGL_UTILS_STRING_H

#include <sstream>
#include <vector>

#include "basic.h"

using std::string;
using std::wstring;
using std::vector;

namespace ccgl {
/*!
 * \namespace ccgl::utils_string
 * \brief String related functions
 */
namespace utils_string {
/*!
 * \brief Get Uppercase of given string
 * \param[in] str
 * \return Uppercase string
 */
string GetUpper(const string& str);

/*!
 * \brief Match \a char ignore cases
 * \param[in] a, b \a char*
 * \return true or false
 * \sa StringMatch()
 */
bool StringMatch(const char* a, const char* b);

/*!
 * \brief Match Strings in UPPERCASE manner
 * \param[in] text1, text2
 * \return true or false
 */
bool StringMatch(const string& text1, const string& text2);

/*!
 * \brief Trim Both leading and trailing spaces
 * \sa Trim
 * \param[in] str \a string
 */
void TrimSpaces(string& str);

/*!
 * \brief Trim given string's heading and tailing by "<space>,\n,\t,\r"
 * \sa TrimSpaces
 * \param[in] s \a string information
 * \return Trimmed string
 */
string& Trim(string& s);

/*!
 * \brief Splits the given string by spaces
 * \param[in] item \a string information
 * \return The split strings vector
 */
vector<string> SplitString(const string& item);

/*!
 * \brief Splits the given string based on the given delimiter
 * \param[in] item \a string information
 * \param[in] delimiter \a char
 * \return The split strings vector
 */
vector<string> SplitString(const string& item, char delimiter);

/*!
 * \brief Convert value to string
 * \param[in] val value, e.g., a int, or float
 * \return converted string
 */
template <typename T>
string ValueToString(const T& val) {
    std::ostringstream oss;
    oss << val;
    return oss.str();
}

/*!
 * \brief Copy string map
 */
void CopyStringMap(const STRING_MAP& in_opts, STRING_MAP& out_opts);

/*!
 * \brief Add or modify element in a string map
 */
void UpdateStringMap(STRING_MAP& opts, const string& key, const string& value);
void UpdateStringMapIfNotExist(STRING_MAP& opts, const string& key, const string& value);

#if defined(CPP_GCC) || defined(CPP_ICC)
extern void _itoa_s(vint32_t value, char* buffer, size_t size, vint radix);
extern void _itow_s(vint32_t value, wchar_t* buffer, size_t size, vint radix);
extern void _i64toa_s(vint64_t value, char* buffer, size_t size, vint radix);
extern void _i64tow_s(vint64_t value, wchar_t* buffer, size_t size, vint radix);
extern void _uitoa_s(vuint32_t value, char* buffer, size_t size, vint radix);
extern void _uitow_s(vuint32_t value, wchar_t* buffer, size_t size, vint radix);
extern void _ui64toa_s(vuint64_t value, char* buffer, size_t size, vint radix);
extern void _ui64tow_s(vuint64_t value, wchar_t* buffer, size_t size, vint radix);
extern void _gcvt_s(char* buffer, size_t size, double value, vint numberOfDigits);
#endif

/*!
 * \brief Convert a signed integer to a string
 * \param[in] number The number to convert
 * \return The converted string
 */
string itoa(vint number);

/*!
 * \brief Convert a signed integer to an unicode string
 * \param[in] number The number to convert
 * \return The converted unicode string
 */
wstring itow(vint number);

/*!
 * \brief Convert a 64-bits signed integer to a string
 * \param[in] number The number to convert
 * \return The converted string
 */
string i64toa(vint64_t number);

/*!
 * \brief Convert a 64-bits signed integer to an unicode string
 * \param[in] number The number to convert
 * \return The converted unicode string
 */
wstring i64tow(vint64_t number);

/*!
 * \brief Convert an unsigned integer to a string
 * \param[in] number The number to convert
 * \return The converted string
 */
string utoa(vuint number);

/*!
 * \brief Convert an unsigned integer to an unicode string
 * \param[in] number The number to convert
 * \return The converted unicode string
 */
wstring utow(vuint number);

/*!
 * \brief Convert a 64-bits unsigned integer to a string
 * \param[in] number The number to convert
 * \return The converted string
 */
string u64toa(vuint64_t number);

/*!
* \brief Convert a 64-bits unsigned integer to an unicode string
* \param[in] number The number to convert
* \return The converted unicode string
*/
wstring u64tow(vuint64_t number);

/*!
 * \brief Convert a 64-bits floating pointer number to a string
 * \param[in] number The number to convert
 * \return The converted string
 */
string ftoa(double number);

/*!
* \brief Convert a 64-bits floating pointer number to an unicode string
* \param[in] number The number to convert
* \return The converted unicode string
*/
wstring ftow(double number);

/*!
 * \brief Convert an unicode string to an Ansi string
 * \param[in] wstr The unicode string to convert
 * \return The converted ansi string
 */
string wtoa(const wstring& wstr);

vint _wtoa(const wchar_t* w, char* a, vint chars);

/*!
 * \brief Convert an Ansi string to an unicode string
 * \param[in] astr The Ansi string to convert
 * \return The converted unicode string
 */
wstring atow(const string& astr);
vint _atow(const char* a, wchar_t* w, vint chars);

/*!
 * \brief Get numeric values by splitting the given string based on the given delimiter
 */
template <typename T>
bool SplitStringForValues(const string& items, const char delimiter, vector<T>& values);

/*!
 * \brief Check if a string is an signed integer, if ture, return the converted integer
 * \param[in] num_str The string to convert
 * \param[out] success Return true if succeed
 * \return The converted number if succeed, otherwise the result is undefined.
 */
vint IsInt(const string& num_str, bool& success);

/*!
 * \brief Check if an unicode string is an signed integer
 * \param[in] num_str The string to convert
 * \param[out] success Return true if succeed
 * \return The converted number if succeed, otherwise the result is undefined.
 */
vint IsInt(const wstring& num_str, bool& success);

/*!
 * \brief Convert a string to an signed 64-bits integer
 * \param[in] num_str The string to convert
 * \param[out] success Return true if succeed
 * \return The converted number if succeed, otherwise the result is undefined.
 */
vint64_t IsInt64(const string& num_str, bool& success);

/*!
 * \brief Convert an unicode string to an signed 64-bits integer
 * \param[in] num_str The string to convert
 * \param[out] success Return true if succeed
 * \return The converted number if succeed, otherwise the result is undefined.
 */
vint64_t IsInt64(const wstring& num_str, bool& success);

/*!
 * \brief Convert an Ansi string to an unsigned integer
 * \param[in] num_str The string to convert
 * \param[out] success Return true if succeed
 * \return The converted number if succeed, otherwise the result is undefined.
 */
vuint IsUInt(const string& num_str, bool& success);

/*!
 * \brief Convert an Unicode string to an unsigned integer
 * \param[in] num_str The string to convert
 * \param[out] success Return true if succeed
 * \return The converted number if succeed, otherwise the result is undefined.
 */
vuint IsUInt(const wstring& num_str, bool& success);

/*!
 * \brief Convert an Ansi string to a 64-bits unsigned integer
 * \param[in] num_str The string to convert
 * \param[out] success Return true if succeed
 * \return The converted number if succeed, otherwise the result is undefined.
 */
vuint64_t IsUInt64(const string& num_str, bool& success);

/*!
 * \brief Convert an Unicode string to a 64-bits unsigned integer
 * \param[in] num_str The string to convert
 * \param[out] success Return true if succeed
 * \return The converted number if succeed, otherwise the result is undefined.
 */
vuint64_t IsUInt64(const wstring& num_str, bool& success);

/*!
 * \brief Convert an Ansi string to 64-bits floating point number
 * \param[in] num_str The string to convert
 * \param[out] success Return true if succeed
 * \return The converted number if succeed, otherwise the result is undefined.
 */
double IsDouble(const string& num_str, bool& success);

/*!
 * \brief Convert an Ansi string to 64-bits floating point number
 * \param[in] num_str The string to convert
 * \param[out] success Return true if succeed
 * \return The converted number if succeed, otherwise the result is undefined.
 */
double IsDouble(const wstring& num_str, bool& success);


/*!
 * \brief Check if a string is a number (integer or float)
 */
template <typename STRING_T>
bool IsNumber(const STRING_T& num_str);

/*!
 * \brief Convert an Ansi or Unicode string to an integer
 */
template <typename STRING_T>
vint ToInt(const STRING_T& num_str);

/*!
 * \brief Convert an Ansi or Unicode string to an signed 64-bits integer
 */
template <typename STRING_T>
vint64_t ToInt64(const STRING_T& num_str);

/*!
 * \brief Convert an Ansi or Unicode string to an unsigned integer
 */
template <typename STRING_T>
vuint ToUInt(const STRING_T& num_str);

/*!
 * \brief Convert an Ansi or Unicode string to a 64-bits unsigned integer
 */
template <typename STRING_T>
vuint64_t ToUInt64(const STRING_T& num_str);

/*!
 * \brief Convert an Ansi or Unicode string to a 64-bits floating point number
 */
template <typename STRING_T>
double ToDouble(const STRING_T& num_str);


template <typename STRING_T>
vint ToInt(const STRING_T& num_str) {
    bool success = false;
    return IsInt(num_str, success);
}

template <typename STRING_T>
vint64_t ToInt64(const STRING_T& num_str) {
    bool success = false;
    return IsInt64(num_str, success);
}

template <typename STRING_T>
vuint ToUInt(const STRING_T& num_str) {
    bool success = false;
    return IsUInt(num_str, success);
}

template <typename STRING_T>
vuint64_t ToUInt64(const STRING_T& num_str) {
    bool success = false;
    return IsUInt64(num_str, success);
}

template <typename STRING_T>
double ToDouble(const STRING_T& num_str) {
    bool success = false;
    return IsDouble(num_str, success);
}

/************ Implementation of template functions ******************/
template <typename T>
bool SplitStringForValues(const string& items, const char delimiter, vector<T>& values) {
    vector<string> value_strs = SplitString(items, delimiter);
    if (value_strs.empty()) { return false; }
    values.clear();
    char* end = nullptr;
    for (auto it = value_strs.begin(); it != value_strs.end(); ++it) {
        if ((*it).find_first_of("0123456789") == string::npos) {
            continue;
        }
        values.emplace_back(static_cast<T>(strtod((*it).c_str(), &end)));
    }
    vector<T>(values).swap(values);
    return value_strs.size() == values.size();
}

template <typename STRING_T>
bool IsNumber(const STRING_T& num_str) {
    bool is_double = false;
    IsDouble(num_str, is_double);
    if (is_double) return true;
    return false;
}
} /* namespace: utils_string */
} /* namespace: ccgl */

#endif /* CCGL_UTILS_STRING_H */
