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
// Very basic variational inequalities and mixed complementarity.
// An excellent reference (e.g. to the Basic Gradient algorithms) is the two-volume set
// "Finite-Dimensional Variational Inequalities and Complementarity Problems" by Facchinei and Pang
// -------------------------------------------------
#ifndef VIMCP_H
#define VIMCP_H
// -------------------------------------------------

#include <assert.h>
#include <functional>
#include <iostream>
#include <tuple>

#include "kutils.h"
#include "kmatrix.h"
#include "prng.h"

namespace KBase {

using std::function;
using std::tuple;

tuple<KMatrix, KMatrix, KMatrix, KMatrix> antiLemke(unsigned int n);

KMatrix projPos(const KMatrix & w);
KMatrix projBox(const KMatrix & lb, const KMatrix & ub, const KMatrix & w);

tuple<KMatrix, unsigned int, KMatrix> viABG(const KMatrix & xInit,
                                            function<KMatrix(const KMatrix & x)> F,
                                            function<KMatrix(const KMatrix & x)> P,
                                            double beta, double thresh, unsigned int iMax,
                                            bool extra);

tuple<KMatrix, unsigned int, KMatrix> viBSHe96(const KMatrix & M, const KMatrix & q,
                                               function<KMatrix(const KMatrix &)> pK,
                                               KMatrix u0, const double eps, const unsigned int iMax);

}; // namespace

// -------------------------------------------------
#endif
// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
