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
// This defines a very simple interface to some
// simple matrix operations. The basic implementation
// is straightforward and inefficient; feel free to subclass
// or modify as your problem requires (e.g. high performance
// parallel implementation for large, sparse matrices).
// "... rather antiquated, but adequate to our simple needs."
// -------------------------------------------------
#ifndef KMATRIX_H
#define KMATRIX_H

#include <cstdint>
#include <functional> 
#include <tuple>
#include <vector>

#include "kutils.h"

namespace KBase {

  using std::function;
  using std::tuple;

  class KMatrix;
  class PRNG;

  KMatrix subMatrix(const KMatrix & m1,
    unsigned int i1, unsigned int i2,  // requires i1 <= i2
    unsigned int j1, unsigned int j2); // requires j1 <= j2

  // return row-vector from row number i
  KMatrix hSlice(const KMatrix & m1, unsigned int i);

  // return column-vector from column j
  KMatrix vSlice(const KMatrix & m1, unsigned int j);

  KMatrix trans(const KMatrix & m);
  double  norm(const KMatrix & m);
  double  sum(const KMatrix & m);
  double  mean(const KMatrix & m);
  double  stdv(const KMatrix & m);
  double  maxAbs(const KMatrix & m);
  tuple<unsigned int, unsigned int>  ndxMaxAbs(const KMatrix & m);
  double  dot(const KMatrix & m1, const KMatrix & m2);
  double  lCorr(const KMatrix & m1, const KMatrix & m2);
  KMatrix  inv(const KMatrix & m);
  KMatrix  clip(const KMatrix & m, double xMin, double xMax);
  KMatrix iMat(unsigned int n);
  bool iMatP(const KMatrix & m);
  KMatrix makePerp(const KMatrix & x, const KMatrix & p);
  KMatrix  joinH(const KMatrix & mL, const KMatrix & mR);
  KMatrix  joinV(const KMatrix & mT, const KMatrix & mB);
  KMatrix operator+ (const KMatrix & m1, const KMatrix & m2);
  KMatrix operator+ (const KMatrix & m1, double x);
  KMatrix operator- (const KMatrix & m1, const KMatrix & m2);
  KMatrix operator- (const KMatrix & m1, double x);
  KMatrix operator* (double x, const KMatrix & m1);
  KMatrix operator* (const KMatrix & m1, double x);
  KMatrix operator/ (const KMatrix & m1, double x);
  bool sameShape(const KMatrix & m1, const KMatrix & m2);
  KMatrix operator* (const KMatrix & m1, const KMatrix & m2);

  KMatrix rescaleRows(const KMatrix& m1, const double vMin, const double vMax);

  // -------------------------------------------------

  class KMatrix {
    friend KMatrix  inv(const KMatrix & m);
  public:

    KMatrix();
    KMatrix(unsigned int nr, unsigned int nc, double iv = 0.0);

    // default copy constructor, copy assigment, etc. are sufficient 
    double operator() (unsigned int i, unsigned int j) const;  // readable rvalue
    double& operator() (unsigned int i, unsigned int j);       // assignable lvalue
    void mPrintf(string) const;
    unsigned int numR() const;
    unsigned int numC() const;
    static KMatrix uniform(PRNG* rng, unsigned int nr, unsigned int nc, double a, double b);

    // this builds a matrix by mapping a function over integer ranges,
    // setting each element to the returned value
    static KMatrix map(function<double(unsigned int i, unsigned int j)> f,
		       unsigned int nr, unsigned int nc);
    
    // this builds a matrix by mapping a function over a matrix and it
    // indices, setting each element to the returned value. Uses indices.
    static KMatrix map(function<double(double mij, unsigned int i, unsigned int j)> f,
		       const KMatrix & mat);
    
    // this builds a matrix by mapping a function over a matrix and it
    // indices, setting each element to the returned value. Ignores indices.
    static KMatrix map(function<double(double x)> f,
		       const KMatrix & mat);
    
    // this maps a function over integer ranges, performing the indicated operation on
    // each pair of indices. As the function must return 'void', it is invoked for side-affects only.
    static void mapV(function<void(unsigned int i, unsigned int j)> f,
		     unsigned int nr, unsigned int nc);

    static KMatrix arrayInit(const double mv[],
			     const unsigned int & rows, const unsigned int & clms);


    // JAH 20160809 return a matrix of specified dimensions populated with the data in the vector
    static KMatrix vecToKmat(vector<double> vec, unsigned int nr, unsigned int nc);

    // JAH 20160814 get a single row slice from a matrix
    KMatrix getRow(unsigned int nr);

    // For those rare cases when we do not need explicit indices inside the loop, 
    // the standard C++11 iterators are provided to support range-for
    vector<double>::iterator begin() { return vals.begin(); };
    vector<double>::iterator end() { return vals.end(); };
    vector<double>::const_iterator cbegin() { return vals.cbegin(); };
    vector<double>::const_iterator cend() { return vals.cend(); };
    vector<double>::const_iterator begin() const { return vals.begin(); };
    vector<double>::const_iterator end() const { return vals.end(); };

    virtual ~KMatrix();

  protected:
    unsigned int rows = 0;
    unsigned int clms = 0;
    vector<double> vals = vector<double>();

  private:
    void vFillVec(unsigned int nr, unsigned int nv, double iv);
    void pivot(unsigned int r, unsigned int c);
    inline unsigned int nFromRC(const unsigned int r, const unsigned int c) const;
    void rcFromN(const unsigned int n, unsigned int & r, unsigned int &c) const;
  };



};

// -------------------------------------------------
#endif
// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
