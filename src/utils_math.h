/*!
 * \file utils_math.h
 * \brief Useful math equations in CCGL.
 *
 * \remarks
 *   - 1. 2018-05-02 - lj - Make part of CCGL.
 *   - 2. 2021-07-15 - lj - Integrate pal.math for fast pow, exp, and ln
 *
 * \author Liangjun Zhu, zlj(a)lreis.ac.cn
 * \version 1.1
 */
#ifndef CCGL_UTILS_MATH_H
#define CCGL_UTILS_MATH_H

#include <cmath>

#include "basic.h"
#include "utils_array.h"
#ifndef M_E
#define M_E        2.7182818284590452354     /* e */
#endif
#ifndef M_LOG2E
#define M_LOG2E    1.4426950408889634074     /* log 2e */
#endif
#ifndef M_LOG10E
#define M_LOG10E   0.43429448190325182765    /* log 10e */
#endif
#ifndef M_LN2 /* Avoid warning, newlib defines this as _M_LN2 */
#define M_LN2      0.69314718055994530942    /* log e2 */
#endif
#ifndef M_LN10
#define M_LN10     2.30258509299404568402    /* log e10 */
#endif
#ifndef M_PI
#define M_PI       3.14159265358979323846    /* pi */
#endif
#ifndef M_PI_2
#define M_PI_2     1.57079632679489661923    /* pi/2 */
#endif
#ifndef M_PI_4
#define M_PI_4     0.78539816339744830962    /* pi/4 */
#endif
#ifndef M_1_PI
#define M_1_PI     0.31830988618379067154    /* 1/pi */
#endif
#ifndef M_2_PI
#define M_2_PI     0.63661977236758134308    /* 2/pi */
#endif
#ifndef M_2_SQRTPI
#define M_2_SQRTPI 1.12837916709551257390    /* 2/sqrt(pi) */
#endif
#ifndef M_SQRT2
#define M_SQRT2    1.41421356237309504880    /* sqrt(2) */
#endif
#ifndef M_SQRT1_2
#define M_SQRT1_2  0.70710678118654752440    /* 1/sqrt(2) */
#endif
#ifndef M_2POW23
#define M_2POW23   8388608.0f                /* pow(2, 23) */
#endif

#ifndef HUGE
#define HUGE ((float)3.40282347e+38)      /*maximum value of float*/
#endif
#ifndef MAXFLOAT
#define MAXFLOAT ((float)3.40282347e+38)  /*maximum value of float*/
#endif
#ifndef MINFLOAT
#define MINFLOAT ((float)1.175494351e-38)  /*minimum value of float*/
#endif

/* This define is in newlib's math.h (but nowhere else) */
#ifndef M_SQRT3
#define M_SQRT3 1.732051f                  /* sqrt(3) */
#endif
/*ADDED MATH CONSTANTS*/
#define M_TC     0.63212055882855767840f    /* 1 - 1/e */
#define M_PI2    6.283185f                  /* pi*2 */
#define M_GOLDEN 1.618034f                  /* golden ratio */
#define M_1_PI2  0.15915494309189534561f    /* 1/(pi*2) */

/*ADDED RECIPROCAL CONSTANTS (AVOID DIVISION AT ALL COST)*/
/*IDEALLY THIS WOULD BE IN THE COMPILER? A BETTER WAY?*/
#define M_DIV3  0.3333333333333333333f  /* 1/3 */
#define M_DIV4  0.25f                   /* 1/4 */
#define M_DIV5  0.2f                    /* 1/5 */
#define M_DIV6  0.1666666666666666666f  /* 1/6 */
#define M_DIV7  0.142857143f            /* 1/7 */
#define M_DIV8  0.125f                  /* 1/8 */
#define M_DIV9  0.1111111111111111111f  /* 1/9 */
#define M_DIV10 0.1f                    /* 1/10 */
#define M_DIV11 0.090909091f            /* 1/11 */
#define M_DIV12 0.0833333333333333333f  /* 1/12 */
#define M_DIV13 0.076923077f            /* 1/13 */
#define M_DIV14 0.071428571f            /* 1/14 */
#define M_DIV15 0.0666666666666666666f  /* 1/15 */
#define M_DIV16 0.0625f                 /* 1/16 */
#define M_DIV_LN10 0.43429448190325182765f /* 1 / ln(10) */

/*ADDED PHYSICAL CONSTANTS (ADD YOUR FAVORITES:-)*/
#define PH_C    ((float)299792458)             /*speed of light (m/s)*/
#define PH_M0   ((float)1.2566370614359172950) /*mag permeability (mH/m)*/
#define PH_H    ((float)6.62606957e-34)        /*planck constant (J/Hz)*/
#define PH_HBAR ((float)1.05457172e-34)        /*diract constant* (J.s/rad)*/
#define PH_K    ((float)1.3806488e-23)         /*boltzmann constant (J/K)*/
#define PH_ME   ((float)9.10938291e-31)        /*mass of electron (kg)*/
#define PH_MP   ((float)1.672614e-27)          /*mass of proton (kg)*/
#define PH_MN   ((float)1.674920e-27)          /*mass of neutron (kg)*/
#define PH_EC   ((float)1.6021917e-19)         /*charge of electron (C)*/
#define PH_F    ((float)9.648670e4)            /*faraday constant (C/mol)*/
#define PH_G    ((float)6.6732e-11)            /*gravitational constant (N*m^2/kg^2)*/
#define PH_AVO  ((float)6.022169e23)           /*avogadro constant*/


namespace ccgl {
/*!
 * \namespace ccgl::utils_math
 * \brief Basic mathematics related functions
 */
namespace utils_math {
/*! Return maximum value */
#ifndef Max
#define Max(a, b) ((a) >= (b) ? (a) : (b))
#endif
/*! Return minimum value */
#ifndef Min
#define Min(a, b) ((a) >= (b) ? (b) : (a))
#endif
/*! Return absolute value */
#ifndef Abs
#define Abs(x) ((x) >= 0 ? (x) : -(x))
#endif
/*! Return non-negative value */
# ifndef NonNeg
# define NonNeg(x) ((x) >= 0 ? (x) : 0)
# endif

/*!
 * \brief Whether v1 is equal to v2
 * \param[in]  v1 Numeric value of data type 1
 * \param[in]  v2 Numeric value of data type 2
 * \return true or false
 */
template <typename T1, typename T2>
bool FloatEqual(T1 v1, T2 v2) {
    return Abs(CVT_DBL(v1) - CVT_DBL(v2)) < 1.e-32;
}

/*!
 * \brief Check the argument against upper and lower boundary values prior to doing Exponential function
 */
float Expo(float xx, float upper = 20.f, float lower = -20.f);

/*!
 *\brief deal with positive and negative float numbers
 */
float Power(float a, float n);

/*!
 * \brief Get maximum value in a numeric array with size n.
 * \param[in] a, n
 * \return max value
 */
template <typename T>
T MaxInArray(const T* a, int n);

/*!
* \brief Get minimum value in a numeric array with size n.
* \param[in] a, n
* \return min value
*/
template <typename T>
T MinInArray(const T* a, int n);


template <typename T>
bool IsVectorAllSame(const vector<T>* a, T value);

template <typename T>
bool IsValueInVector(const vector<T>* a, T value);

/*!
 * \brief Sum of a numeric array
 * Get sum value of a double array with size row.
 * \param[in] row
 * \param[in] data
 * \return sum
 */
template <typename T>
T Sum(int row, const T* data);

/*!
 * \brief Sum of a numeric array
 * Get sum value of a double array with size row and real index idx.
 * \param[in] row
 * \param[in] idx
 * \param[in] data
 * \return sum
 */
template <typename T>
T Sum(int row, int*& idx, const T* data);

/*!
 * \brief calculate basic statistics at one time_funcs
 * \param[in] values data array
 * \param[in] num data length
 * \param[out] derivedvalues \a double array, value number, mean, max, min, std, range
 * \param[in] exclude optional, excluded value, e.g. NoDATA, the default is -9999
 */
template <typename T>
void BasicStatistics(const T* values, int num, double** derivedvalues,
                     T exclude = static_cast<T>(NODATA_VALUE));

/*!
 * \brief calculate basic statistics at one time_funcs for 2D raster data
 * \param[in] values data array
 * \param[in] num data length
 * \param[in] lyrs layer number
 * \param[out] derivedvalues \a double array, value number, mean, max, min, std, range
 * \param[in] exclude optional, excluded value, e.g. NoDATA, the default is -9999
 */
template <typename T>
void BasicStatistics(const T*const * values, int num, int lyrs,
                     double*** derivedvalues, T exclude = static_cast<T>(NODATA_VALUE));

/*!
 * \brief approximate sqrt
 *
 * This uses a method to approximate sqrt which only applies to IEEE 754 floating point numbers,
 * described in [1]. The optimized magic constant is from Chris Lomont[2]
 *
 * References:
 * 1: http://en.wikipedia.org/wiki/Fast_inverse_square_root
 * 2: http://www.lomont.org/Math/Papers/2003/InvSqrt.pdf
 *
 * \param[in] z float or double value
 * \return approximation of sqrt
 */
float ApprSqrt(float z);
double ApprSqrt(double z);

template<typename T>
T CalSqrt(T val) {
#if defined(USE_APPR_PAL_MATH)
    return ApprSqrt(val);
#else
    return sqrt(val);
#endif
}

/*!
 * \brief Approximate e^x by Taylor Series expansion when 0<=x<=ln2
 *
 * e^x = 1./e^-x
 * e^-x = 1 + a1 * x + a2 * x^2 + ... + a4 * x^4 + e(x), where |e(x)| <= 3 * 10^-5
 *
 */
template <typename T>
static inline T __p_exp_ln2(const T x) {
    const T a1 = static_cast<T>(-0.9998684);
    const T a2 = static_cast<T>(0.4982926);
    const T a3 = static_cast<T>(-0.1595332);
    const T a4 = static_cast<T>(0.0293641);
    T exp_x = static_cast<T>(1.0) +
        a1 * x +
        a2 * x * x +
        a3 * x * x * x +
        a4 * x * x * x * x;
    return static_cast<T>(1.0) / exp_x;
}

/*!
 * \brief Approximate e^x when x >= 0
 *
 * exp x = exp(x' + k ln 2) = (exp x') * 2^k
 */
template <typename T>
static inline T __p_exp_pos(const T x) {
    long int k, twok;
    static const T ln2 = static_cast<T>(M_LN2);
    T x_;
    k = x / ln2;
    twok = 1ULL << k;
    x_ = x - static_cast<T>(k) * ln2;
    return static_cast<T>(twok) * __p_exp_ln2(x_);
}

template <typename T>
static inline T ApprExp(const T x) {
    if (x >= static_cast<T>(0.0))
        return __p_exp_pos(x);
    else
        return static_cast<T>(1.0) / __p_exp_pos(-x);
}

template<typename T>
T CalExp(T val) {
#if defined(USE_APPR_PAL_MATH)
    return ApprExp(val);
#else
    return exp(val);
#endif
}

/*!
 * \brief Approximates the natural logarithm, (where the base is 'e'=2.71828)
 *
 */
float ApprLn(float z);
double ApprLn(double z);

template<typename T>
T CalLn(T val) {
#if defined(USE_APPR_PAL_MATH)
    return ApprLn(val);
#else
    return log(val);
#endif
}

/*!
  * \brief lookup for pow(a, b) function
  * \param[in] exp power to raise radix to (exponent), i.e., b in pow(a, b)
  * \param[in] log_base one over log, to required radix, of two (ln(base))
  */
float pow_lookup(const float exp, const float log_base);

/*!
 * \brief Approximates pow(a, b) based on the work of Harrison Ainsworth.
 *
 * Refers to http://www.hxa.name/articles/content/fast-pow-adjustable_hxa7241_2007.html
 * Copyright (c) 2007, Harrison Ainsworth / HXA7241.
 */
float inline ApprPow(float a, float b) {
    // pow(base, exponent) = pow_lookup(exponent, ln(base))
    return pow_lookup(b, ApprLn(a));
};

template<typename T1, typename T2>
double CalPow(T1 a, T2 b) {
#if defined(USE_APPR_PAL_MATH)
    return CVT_DBL(ApprPow(CVT_FLT(a), CVT_FLT(b)));
#else
    return pow(CVT_DBL(a),CVT_DBL(b));
#endif
}

/************ Implementation of template functions ******************/
template <typename T>
T MaxInArray(const T* a, const int n) {
    T m = a[0];
    for (int i = 1; i < n; i++) {
        if (a[i] > m) {
            m = a[i];
        }
    }
    return m;
}

template <typename T>
T MinInArray(const T* a, const int n) {
    T m = a[0];
    for (int i = 1; i < n; i++) {
        if (a[i] < m) {
            m = a[i];
        }
    }
    return m;
}

template <typename T>
bool IsVectorAllSame(const vector<T>* a, T value){
    for (int i = 0; i < a->size(); i++) {
        if (a->at(i) != value) {
            return false;
        }
    }
    return true;
}

template <typename T>
bool IsValueInVector(const vector<T>* a, T value){
    return std::find(a->begin(), a->end(), value) != a->end();
}
template <typename T>
bool IsValueInVectorVector(const vector< vector<T>>* a, T value) {
    for (int i = 0; i < a->size(); i++) {
        if (std::find(a->at(i).begin(), a->at(i).end(), value) != a->at(i).end()) {
            return true;
        }
    }
    return false;
}
template <typename T>
T Sum(const int row, const T* data) {
    T tmp = 0;
#pragma omp parallel for reduction(+:tmp)
    for (int i = 0; i < row; i++) {
        tmp += data[i];
    }
    return tmp;
}

template <typename T>
T Sum(const int row, int*& idx, const T* data) {
    T tmp = 0;
#pragma omp parallel for reduction(+:tmp)
    for (int i = 0; i < row; i++) {
        int j = idx[i];
        tmp += data[j];
    }
    return tmp;
}

template <typename T>
void BasicStatistics(const T* values, const int num, double** derivedvalues,
                     T exclude /* = CVT_TYP(NODATA_VALUE) */) {
    double* tmpstats = new double[6];
    double maxv = MISSINGFLOAT;
    double minv = MAXIMUMFLOAT;
    int validnum = 0;
    double sumv = 0.;
    double std = 0.;
    for (int i = 0; i < num; i++) {
        if (FloatEqual(values[i], exclude)) continue;
        if (maxv < values[i]) maxv = values[i];
        if (minv > values[i]) minv = values[i];
        validnum += 1;
        sumv += values[i];
    }
    tmpstats[0] = CVT_DBL(validnum);
    double mean = sumv / tmpstats[0];
#pragma omp parallel for reduction(+:std)
    for (int i = 0; i < num; i++) {
        if (!FloatEqual(values[i], exclude)) {
            std += (values[i] - mean) * (values[i] - mean);
        }
    }
    std = sqrt(std / tmpstats[0]);
    tmpstats[1] = mean;
    tmpstats[2] = maxv;
    tmpstats[3] = minv;
    tmpstats[4] = std;
    tmpstats[5] = maxv - minv;
    *derivedvalues = tmpstats;
}

template <typename T>
void BasicStatistics(const T*const * values, const int num, const int lyrs,
                     double*** derivedvalues, T exclude /* = CVT_TYP(NODATA_VALUE) */) {
    double** tmpstats = new double *[6];
    for (int i = 0; i < 6; i++) {
        tmpstats[i] = new double[lyrs];
    }
    for (int j = 0; j < lyrs; j++) {
        tmpstats[0][j] = 0.;                    /// valid number
        tmpstats[1][j] = 0.;                    /// mean
        tmpstats[2][j] = CVT_DBL(MISSINGFLOAT); /// maximum
        tmpstats[3][j] = CVT_DBL(MAXIMUMFLOAT); /// minimum
        tmpstats[4][j] = 0.;                    /// std
        tmpstats[5][j] = 0.;                    /// range
    }
    double* sumv = nullptr;
    utils_array::Initialize1DArray(lyrs, sumv, 0.);
    for (int i = 0; i < num; i++) {
        for (int j = 0; j < lyrs; j++) {
            if (FloatEqual(values[i][j], exclude)) continue;
            if (tmpstats[2][j] < values[i][j]) tmpstats[2][j] = values[i][j];
            if (tmpstats[3][j] > values[i][j]) tmpstats[3][j] = values[i][j];
            tmpstats[0][j] += 1;
            sumv[j] += values[i][j];
        }
    }

    for (int j = 0; j < lyrs; j++) {
        tmpstats[5][j] = tmpstats[2][j] - tmpstats[3][j];
        tmpstats[1][j] = sumv[j] / tmpstats[0][j];
    }
    for (int j = 0; j < lyrs; j++) {
        double tmpstd = 0;
#pragma omp parallel for reduction(+:tmpstd)
        for (int i = 0; i < num; i++) {
            if (!FloatEqual(values[i][j], exclude)) {
                tmpstd += (values[i][j] - tmpstats[1][j]) * (values[i][j] - tmpstats[1][j]);
            }
        }
        tmpstats[4][j] = tmpstd;
    }
    for (int j = 0; j < lyrs; j++) {
        tmpstats[4][j] = sqrt(tmpstats[4][j] / tmpstats[0][j]);
    }
    utils_array::Release1DArray(sumv);
    *derivedvalues = tmpstats;
}

} /* namespace: utils_math */
} /* namespace: ccgl */

#endif /* CCGL_UTILS_MATH_H */
