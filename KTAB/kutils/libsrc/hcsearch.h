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
// --------------------------------------------


#ifndef KBASE_HCSEARCH_H
#define KBASE_HCSEARCH_H

#include <functional>   // function
#include <iostream>     // cout, etc.
#include <tuple>        // tuple, get, etc.
#include <vector>

#include "kutils.h"
#include "kmatrix.h"


// ----------------------------------------------
// Note that in VHCSearch and GHCSearch, I do not
// make much effort to clean up transient objects,
// like the various neighbors.
// Correctness first, then performance.
// ----------------------------------------------

namespace KBase {

using std::cout;
using std::endl;
using std::flush;
using std::tuple;

using KBase::ReportingLevel;

// Setup and manage maximization of scalar function of a column-vector
class  VHCSearch {
public:
    VHCSearch();
    virtual ~VHCSearch();
    tuple<double, KMatrix, unsigned int, unsigned int>

    // maximize scalar function of a column-vector
    run(KMatrix p0,
        unsigned int iMax, unsigned int sMax, double sTol,
        double s0, double shrink, double grow, double minStep,
        ReportingLevel rl
       );

    static vector<KMatrix> vn1(const KMatrix & m0, double s);
    static vector<KMatrix> vn2(const KMatrix & m0, double s);

    function <double(const KMatrix &)> eval = nullptr; // maximize this function
    function < vector<KMatrix>(const KMatrix &, double)> nghbrs = nullptr;
    function <void (const KMatrix &)> report = nullptr;

protected:
    
    // Note that these variables to control the search are
    // unique to this object, so it should be OK to run
    // several VHCSearch objects concurrently.
    std::mutex vhcEvalMtx; 
    double vhcBestVal;
    KMatrix vhcBestPoint;
    
private:
};


// Class to setup maximization of scalar function of arbitrary class
template <class HCP>
class GHCSearch {
public:
    GHCSearch();
    virtual ~GHCSearch();

    tuple<double, HCP, unsigned int, unsigned int>

    // maximize scalar function of arbitrary class
    run(HCP p0, ReportingLevel srl, unsigned int iMax, unsigned int sMax, double sTol);

    function <double(const HCP)> eval = nullptr;
    function <vector<HCP>(const HCP)> nghbrs = nullptr;
    function <void(const HCP)> show = nullptr;
};

template<class HCP>
GHCSearch<HCP>::GHCSearch() {
    eval = nullptr;
    nghbrs = nullptr;
    show = nullptr;
}

template<class HCP>
GHCSearch<HCP>::~GHCSearch() {
    eval = nullptr;
    nghbrs = nullptr;
    show = nullptr;
}

template<class HCP>
tuple<double, HCP, unsigned int, unsigned int>
GHCSearch<HCP>::run(HCP p0, ReportingLevel srl,
                    unsigned int iMax, unsigned int sMax, double sTol) {
    assert(eval != nullptr);
    assert(nghbrs != nullptr);
    unsigned int iter = 0;
    unsigned int sIter = 0;
    double v0 = eval(p0);

    while ((iter < iMax) && (sIter < sMax)) {
        double dv = 0;
        double vBest = v0;
        HCP pBest = p0;

        for (HCP pTmp : nghbrs(p0)) {
            double vTmp = eval(pTmp);
            if (vTmp > vBest) {
                vBest = vTmp;
                pBest = pTmp;
            }
        }

        if (vBest > v0 + sTol) {
            sIter = 0;
            dv = vBest - v0;
            v0 = vBest;
            p0 = pBest;
        }
        else {
            sIter++;
        }
        iter++;

        if (ReportingLevel::Low < srl) {
            printf("%u/%u iterations    %u/%u stable \n", iter, iMax, sIter, sMax);
            printf("newBest value: %+.4f up %+.4f \n", vBest, dv);
            printf("newBest point: ");
            show(p0);
            cout << endl << endl;
        }
    }
    if (ReportingLevel::Silent < srl) {
        printf("GHCSearch::run ended with %u/%u iterations    %u/%u stable \n", iter,
               iMax, sIter, sMax);
        printf("newBest value: %+.4f\n", v0);
        printf("newBest point: ");
        show(p0);
        cout << endl << endl;
    }

    auto rslt = tuple<double, HCP, unsigned int, unsigned int>(v0, p0, iter, sIter);
    return rslt;
}

// ----------------------------------------------


} // namespace KBase

// ----------------------------------------------
#endif
// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
