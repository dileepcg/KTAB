// --------------------------------------------
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

#include <inttypes.h> // especially uint64_t
#include <algorithm>
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
using std::ostream;  


typedef vector<unsigned int> VUI;
typedef tuple<double, unsigned int> TDI;
typedef vector<bool> VBool;

void printVUI(const VUI& p);

enum class ReportingLevel : uint8_t { Silent = 0, Low, Medium, High, Debugging };

// arbitrary default PRNG seed value, which just
// happens to be one of my favorite integers
const uint64_t dSeed = 0xD67CC16FE69C185C;

double sqr(const double& x);  // second power
double qrtc(const double& x); // fourth power

// basic formula for quadratic risk aversion
double quadUfromV(double v, double bigR) ;


// basic Nash Product for bargaining
double nProd(double x, double y);

double trim(double x, double minX, double maxX, bool strict = false);

// This launches a number of threads, but no more than numPar at a time.
// The function is given unsigned ints in a range, like [0, n-1] inclusive.
// If no value is given for numPar, it will guess from the number of cores.
void groupThreads(function<void(unsigned int)> tfn,
                  unsigned int numLow, unsigned int numHigh, unsigned int numPar=0);

// ----------------------------------------------

std::chrono::time_point<std::chrono::system_clock>  displayProgramStart(string appName = "", string appVersion = "");
void displayProgramEnd(std::chrono::time_point<std::chrono::system_clock> st);

// return a string of '0' chars
char* newChars(unsigned int len);

// linearly (actually, affine) rescale the [x0,x1] range into the [y0,y1] range
double rescale(double x, double x0, double x1, double y0, double y1);

// Pop the front of the vector, returning the popped item.
template<typename T>
T popBack(vector<T> & v) {
  // I find it irritating that the 'pop_xxx' operations of C++11 all return void,
  // unlike the "pop" operations taught in CS courses which return the popped element.
  // Note the use of a reference to avoid the problem of modifying only a copy.
  auto n = ((const unsigned int)(v.size()));
  assert(0 < n);
  auto el = v[n - 1];
  v.pop_back();
  return el;
}


// Return a pair of VUI: the unique indices and the equivalent indices.
template <typename T>
tuple<VUI, VUI>
ueIndices(const vector<T> &xs, function<bool(const T &a, const T &b)> eqv) {
  // Suppose we have a vector of eleven items:
  // [10, 11, 20, 12, 30,  9, 23, 29, 40, 22, 43]
  // Their indices are (obviously) just integers in order:
  // [ 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10]
  // They are considered equivalent if they differ by 3 or less.
  // The unique elements are 10,20,30,40, and the first appearance of each is at [0, 2, 4, 8].
  //
  // There are two ways to compute the equivalent indices: index into the original list, or index into uniques.
  // I chose the latter, for the following reason.
  // Suppose we have N actors, with N positions. Of those, M<=N are unique.
  // We compute pUtil(i,k) = utility to actor i of k-th unique position, 0<=i<N, 0<=k<M
  // We also want aUtil(i,j) = utility to actor i of position of actor j.
  // The easiest way is aUtil(i,j) = pUtil(i, eNdx(j)), so 0 <= eNdx(j) < M.
  // Thus, eNdx(j) is the index of the unique position held by actor j, 0<=j<N.
  //
  // Here are indices of the unique positions to which each item is equivalent:
  // [ 0,  0,  1,  0,  2,  0,  1,  2,  3,  1,  3] for the same list of items,
  // [10, 11, 20, 12, 30,  9, 23, 29, 40, 22, 43].
  // Clearly, every index in the eIndices list should appear in the uIndices list, and vice versa.
  VUI uns = {}; // unique indices
  VUI ens = {}; // equivalent indices
  auto n = ((const unsigned int)(xs.size()));
  for (unsigned int i = 0; i < n; i++) {
    bool found = false;
    for (unsigned int j = 0; (!found) && (j < uns.size()); j++) {
      unsigned int k = uns[j];
      if (eqv(xs[i], xs[k])) {
        found = true;
        ens.push_back(j);
      }
    }
    if (!found) {
      uns.push_back(i); // now uns.size is at least 1
      auto en = ((unsigned int)(uns.size()));
      assert(1 <= en);
      ens.push_back(en - 1);
    }
  }
  return tuple<VUI, VUI>(uns, ens);
}

// the unsigned ints in order from n1 to n2, inclusive.
VUI uiSeq(const unsigned int n1, const unsigned int n2, const unsigned int ns = 1);

class KException {
public:
  explicit KException(string m);
  virtual ~KException();
  string msg = "";
};

/*
class EnumType {
public:
  explicit EnumType(int i);
  explicit EnumType(const char* n);
  explicit EnumType(const string& s);

  virtual ~EnumType();

 // ostream& operator<< (ostream& os, const EnumType& et);

protected:
  unsigned int ndx = 0;
  unsigned int addName(string etn);
  vector<string> *names = nullptr; // pointer to shared data structure
  string name(const EnumType& et);

private:
};
*/

}; // end of namespace

// -------------------------------------------------
#endif
// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
