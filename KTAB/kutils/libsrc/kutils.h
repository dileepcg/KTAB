﻿// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2015 King Abdullah Petroleum Studies and Research Center
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software
// and associated documentation files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom
// the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
// BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// -------------------------------------------------
// Define a few generally useful functions.
//
// BTW, this is the header for which Findkutils.cmake will
// look to verify that is has located the right directory
// for the whole set of related header files.
// -------------------------------------------------
#ifndef KBASE_UTIL_H
#define KBASE_UTIL_H

#include <assert.h>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <functional>
#include <future>
#include <math.h>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

namespace KBase {
using std::function;
using std::string;
using std::tuple;
using std::vector;

typedef vector<unsigned int> VUI;

class PRNG;
class KMatrix;

enum class ReportingLevel {
    Silent, Low, Medium, High, Debugging
};


std::chrono::time_point<std::chrono::system_clock>  displayProgramStart();
void displayProgramEnd(std::chrono::time_point<std::chrono::system_clock> st);

// return a string of '0' chars
char* newChars(unsigned int len);

// linearly (actually, affine) rescale the [x0,x1] range into the [y0,y1] range
double rescale(double x, double x0, double x1, double y0, double y1);

// I find it irritating that the 'pop_xxx' operations of C++11 all return void,
// unlike the "pop" operations taught in CS courses which return the popped element.
// Note the use of a reference to avoid the problem of modifying only a copy.
template<typename T>
T popBack(vector<T> & v) {
    unsigned int n = v.size();
    assert(0 < n);
    auto el = v[n - 1];
    v.pop_back();
    return el;
}


// Return a pair of VUI: the unique indices and the equivalent indices.
// Suppose we have a vector of eleven items:
// [10, 11, 20, 12, 30,  9, 23, 29, 40, 22, 43]
// Their indices are (obviously) just integers in order:
// [ 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10]
// They are considered equivalent if they differ by 3 or less.
// The unique elements are 10,20,30,40, and the first appearance of each is at [0, 2, 4, 8].
// Also of use are indices of the unique positions to which each item is equivalent:
// [ 0,  0,  2,  0,  4,  0,  2,  4,  8,  2,  8] for the same list of items,
// [10, 11, 20, 12, 30,  9, 23, 29, 40, 22, 43].
// Clearly, every item in the eIndices list should appear in the uIndices list.
template <typename T>
tuple<VUI, VUI>
ueIndices(const vector<T> &xs, function<bool (const T &a, const T &b)> eqv) {
    VUI uns = {}; // unique indices
    VUI ens = {}; // equivalent indices
    const unsigned int n = xs.size();
    for (unsigned int i=0; i<n; i++) {
        bool found = false;
        for (unsigned int j=0; (!found)&&(j<uns.size()); j++) {
            unsigned int k = uns[j];
            if (eqv(xs[i], xs[k])) {
                found = true;
                ens.push_back(k);
            }
        }
        if (!found) {
            uns.push_back(i);
            ens.push_back(i);
        }
    }
    return tuple<VUI, VUI> (uns, ens);
}

// the unsigned ints in order from n1 to n2, inclusive.
VUI uiSeq(const unsigned int n1, const unsigned int n2, const unsigned int ns = 1);

class KException {  // just something to throw around
public:
    explicit KException(string m);
    virtual ~KException();
    string msg="";
};


double sqr(const double x); // second power
double qrtc(const double x); // fourth power

}; // end of namespace

// -------------------------------------------------
#endif
// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
