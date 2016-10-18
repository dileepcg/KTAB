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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <string.h>
#include <vector>

#include "prng.h"
#include "kmatrix.h"


namespace KBase {

using std::printf;
using std::cout;
using std::endl;
using std::flush;

KMatrix subMatrix(const KMatrix & m1,
                  unsigned int i1, unsigned int i2,
                  unsigned int j1, unsigned int j2) {

    assert(i1 <= i2);
    assert(i2 < m1.numR());
    const unsigned int nr = 1 + (i2 - i1);

    assert(j1 <= j2);
    assert(j2 < m1.numC());
    const unsigned int nc = 1 + (j2 - j1);

    KMatrix m2 = KMatrix(nr, nc);
    for (unsigned int i = 0; i < nr; i++) {
        for (unsigned int j = 0; j < nc; j++) {
            m2(i, j) = m1(i1 + i, j1 + j);
        }
    }
    return m2;
}

// return row-vector from row number i
KMatrix hSlice(const KMatrix & m1, unsigned int i) {
    auto m2 = subMatrix(m1, i, i, 0, m1.numC() - 1);
    return m2;
}

// return column-vector from column j
KMatrix vSlice(const KMatrix & m1, unsigned int j) {
    auto m2 = subMatrix(m1, 0, m1.numR() - 1, j, j);
    return m2;
}

KMatrix trans(const KMatrix & m1) {
    unsigned int nr2 = m1.numC();
    unsigned int nc2 = m1.numR();
    auto m2 = KMatrix(nr2, nc2);
    for (unsigned int i = 0; i < nr2; i++) {
        for (unsigned int j = 0; j < nc2; j++) {
            m2(i, j) = m1(j, i);
        }
    }
    return m2;
}


double norm(const KMatrix & m1) {
    double s = 0.0;
    for (auto x : m1) {
        s = s + (x*x);
    }
    return sqrt(s);
}


double sum(const KMatrix & m1) {
    double s = 0.0;
    for (auto x : m1) {
        s = s + x;
    }
    return s;
}


double mean(const KMatrix & m1) {
    return sum(m1) / (m1.numC()*m1.numR());
}


double stdv(const KMatrix & m1) {
    return norm(m1 - mean(m1)) / sqrt(m1.numR() * m1.numC());
}


double  maxAbs(const KMatrix & m) {
    double ma = 0;
    for (auto x : m) {
        double a = fabs(x);
        ma = (a > ma) ? a : ma;
    }
    return ma;
}

tuple<unsigned int, unsigned int>  ndxMaxAbs(const KMatrix & m) {
    const unsigned int nr = m.numR();
    const unsigned int nc = m.numC();

    // mark with obviously impossible values
    double ma = -1.0;
    unsigned int ndxI = 1+nr;
    unsigned int ndxJ = 1+nc;

    for (unsigned int i=0; i<nr; i++) {
        for (unsigned int j=0; j<nc; j++) {
            double a = fabs(m(i,j));
            if (ma < a) {
                ndxI = i;
                ndxJ = j;
                ma = a;
            }
        }
    }
    assert (0.0 <= ma);
    assert (ndxI < nr);
    assert (ndxJ < nc);
    auto ndx = tuple<unsigned int, unsigned int>(ndxI, ndxJ);
    return ndx;
}

// Notice that this is strictly, mathematically "linear" correlation,
// based on the model Y = aX.
// It is simple: a-est = (YX)/(XX), with correlation (YX)/sqrt((YY)(XX))
//
// When people say "linear correlation" and "linear model", they usually
// mean "affine", like Y = aX+b.
// If you want affine correlation, it is simply lCorr(y-mean(y), x-mean(x))
double  lCorr(const KMatrix & m1, const KMatrix & m2) {
    return dot(m1, m2) / sqrt(dot(m1, m1)*dot(m2, m2));
}


double  dot(const KMatrix & m1, const KMatrix & m2) {
    assert(sameShape(m1, m2));
    double s12 = 0;
    for (unsigned int i = 0; i < m1.numR(); i++) {
        for (unsigned int j = 0; j < m1.numC(); j++) {
            s12 = s12 + m1(i, j)*m2(i, j);
        }
    }
    return s12;
}


KMatrix::KMatrix() {
    vFillVec(0, 0, 0.0); // totally empty
}

KMatrix::~KMatrix() {
    vFillVec(0, 0, 0.0); // totally empty
}


KMatrix::KMatrix(unsigned int nr, unsigned int nc, double iv) {
    vFillVec(nr, nc, iv);
}

// if double mv[] = { 11, 12, 13, 21, 22, 23 }, then
// mArrayInit (mv, 2, 3) yields
// 11  12  13
// 21  22  23
// Trying to embed the mv declaration in the mArrayInit
// call does not seem to work easily.
//
// You have to make very sure that the mv[] really is of size nr*nc. If not, it just reads in random memory.
KMatrix KMatrix::arrayInit(const double mv[], const unsigned int & nr, const unsigned int & nc) {
    KMatrix m = KMatrix(nr, nc);
    for (unsigned int i = 0; i < nr; i++) {
        for (unsigned int j = 0; j < nc; j++) {
            unsigned int n = m.nFromRC(i, j);
            m(i, j) = mv[n];
        }
    }
    return m;
}


void KMatrix::mPrintf(string fs) const {
    const char * fc = fs.c_str();
    auto pf = [fc, this](unsigned int i, unsigned int j) {
        std::printf(fc, (*this)(i, j));
        if (j == (this->numC() - 1)) {
            std::printf("\n");
        }
        return;
    };
    mapV(pf, rows, clms);
    return;
}


void KMatrix::vFillVec(unsigned int nr, unsigned int nc, double iv) {
    rows = nr;
    clms = nc;
    const unsigned int n = nr*nc;
    vals = {}; // 0-length vector
    vals.resize(n);
    for (unsigned int i = 0; i < n; i++) {
        vals[i] = iv;
    }
    return;
}


double KMatrix::operator () (unsigned int i, unsigned int j) const {
    const unsigned int n = nFromRC(i, j);
    return vals[n]; // rvalue
}


double& KMatrix::operator() (unsigned int i, unsigned int j) {
    const unsigned int n = nFromRC(i, j);
    return vals[n]; // lvalue
}


unsigned int KMatrix::numR() const {
    return rows;
}
unsigned int KMatrix::numC() const {
    return clms;
}


unsigned int KMatrix::nFromRC(const unsigned int r, const unsigned int c) const {
    assert(r < rows);
    assert(c < clms);
    return (r*clms + c);
}


void KMatrix::rcFromN(const unsigned int n, unsigned int & r, unsigned int &c) const {
    assert(n < rows*clms);
    r = n / clms;
    c = n - (r*clms);
    assert(r < rows);
    assert(c < clms);
    return;
}


KMatrix operator+ (const KMatrix & m1, double x) {
    auto af = [x, &m1](unsigned int i, unsigned int j) {
        return m1(i, j) + x;
    };
    return KMatrix::map(af, m1.numR(), m1.numC());
}


KMatrix operator- (const KMatrix & m1, double x) {
    auto sf = [x, &m1](unsigned int i, unsigned int j) {
        return m1(i, j) - x;
    };
    return KMatrix::map(sf, m1.numR(), m1.numC());
}


bool sameShape(const KMatrix & m1, const KMatrix & m2) {
    const bool sameR = (m1.numR() == m2.numR());
    const bool sameC = (m1.numC() == m2.numC());
    return (sameR && sameC);
}


KMatrix operator+ (const KMatrix & m1, const KMatrix & m2) {
    assert(sameShape(m1, m2));
    auto af = [m1, m2](unsigned int i, unsigned int j) {
        return m1(i, j) + m2(i, j);
    };
    return KMatrix::map(af, m1.numR(), m1.numC());
}


KMatrix operator- (const KMatrix & m1, const KMatrix & m2) {
    assert(sameShape(m1, m2));
    auto sf = [&m1, &m2](unsigned int i, unsigned int j) {
        return m1(i, j) - m2(i, j);
    };
    return KMatrix::map(sf, m1.numR(), m1.numC());
}


KMatrix operator* (double x, const KMatrix & m1) {
    auto mf = [x, &m1](unsigned int i, unsigned int j) {
        return x*m1(i, j);
    };
    return KMatrix::map(mf, m1.numR(), m1.numC());
}


KMatrix operator* (const KMatrix & m1, double x) {
    auto mf = [x, &m1](unsigned int i, unsigned int j) {
        return x*m1(i, j);
    };
    return KMatrix::map(mf, m1.numR(), m1.numC());
}


KMatrix operator/ (const KMatrix & m1, double x) {
    auto df = [x, &m1](unsigned int i, unsigned int j) {
        return m1(i, j) / x;
    };
    return KMatrix::map(df, m1.numR(), m1.numC());
}


KMatrix operator* (const KMatrix & m1, const KMatrix & m2) {
    const unsigned int nr3 = m1.numR();
    const unsigned int nm3 = m1.numC();
    assert(nm3 == m2.numR());
    const unsigned int nc3 = m2.numC();
    auto f = [nm3, &m1, &m2](unsigned int i, unsigned int j) {
        double sij = 0.0;
        for (unsigned int k = 0; k < nm3; k++) {
            sij = sij + m1(i, k)*m2(k, j);
        }
        return sij;
    };
    return KMatrix::map(f, nr3, nc3);
}


KMatrix KMatrix::map(function<double(unsigned int i, unsigned int j)> f, unsigned int nr, unsigned int nc) {
    assert (f != nullptr);
    auto m = KMatrix(nr, nc);
    for (unsigned int i = 0; i < nr; i++) {
        for (unsigned int j = 0; j < nc; j++) {
            m(i, j) = f(i, j);
        }
    }
    return m;
}

KMatrix KMatrix::map(function<double(double mij, unsigned int i, unsigned int j)> f,
                     const KMatrix & mat) {
    assert (f != nullptr);
    const unsigned int nr = mat.numR();
    const unsigned int nc = mat.numC();
    auto m = KMatrix(nr,nc);
    for (unsigned int i = 0; i < nr; i++) {
        for (unsigned int j = 0; j < nc; j++) {
            m(i, j) = f(mat(i,j), i, j);
        }
    }
    return m;
}


KMatrix KMatrix::map(function<double(double x)> f,
		       const KMatrix & mat) {
    assert (f != nullptr);
    const unsigned int nr = mat.numR();
    const unsigned int nc = mat.numC();
    auto m = KMatrix(nr,nc);
    for (unsigned int i = 0; i < nr; i++) {
        for (unsigned int j = 0; j < nc; j++) {
            m(i, j) = f(mat(i,j));
        }
    }
    return m;
}
    

void KMatrix::mapV(function<void(unsigned int i, unsigned int j)> f, unsigned int nr, unsigned int nc) {
    assert (f != nullptr);
    for (unsigned int i = 0; i < nr; i++) {
        for (unsigned int j = 0; j < nc; j++) {
            f(i, j);
        }
    }
    return;
}


KMatrix KMatrix::uniform(PRNG* rng, unsigned int nr, unsigned int nc, double a, double b) {
    auto rf = [rng, a, b](unsigned int, unsigned int) {
        return rng->uniform(a, b);
    };
    return map(rf, nr, nc);
}


void KMatrix::pivot(unsigned int r, unsigned int c) {
    auto x = [this](unsigned int i, unsigned int j) {
        return vals[nFromRC(i, j)];
    };
    double xrc = x(r, c);
    double xrcAbs = fabs(xrc);
    const double minPivot = 1E-8;
    assert(xrcAbs > minPivot); // nearly-singular?

    // pivot the four blocks outside the main row and main column
    for (unsigned int i = 0; i < rows; i++) {
        for (unsigned int j = 0; j < clms; j++) {
            if ((r != i) && (c != j)) {
                double vij = x(i, j) - (x(i, c)*x(r, j)) / xrc;
                (*this)(i, j) = vij;
            }
        }
    }

    // pivot the main row
    for (unsigned int j = 0; j < clms; j++) {
        if (c != j) {
            double vrj = x(r, j) / xrc;
            (*this)(r, j) = vrj;
        }
    }

    // pivot the main column
    for (unsigned int i = 0; i < rows; i++) {
        if (r != i) {
            (*this)(i, c) = 0;
        }
    }

    // finally the pivot element itself
    (*this)(r, c) = 1;
    return;
}

// textbook algorithm
KMatrix inv(const KMatrix & m) {
    const unsigned int n = m.numR();
    assert(n == m.numC());

    KMatrix m2 = joinH(KMatrix(m), iMat(n));
    VBool ok = {};
    ok.resize(n);
    for (unsigned int i = 0; i < n; i++) {
        ok[i] = true; // at start, ok to pivot on any
    }

    for (unsigned int iter = 0; iter < n; iter++) {
        double maxD = -1;
        unsigned int pk = 0;
        for (unsigned int k = 0; k < n; k++) {
            if (ok[k]) {
                const double mk = m2(k,k);
                const double pe = fabs(mk);
                if (pe > maxD) {
                    maxD = pe;
                    pk = k;
                }
            }
        }
        assert(0 < maxD);
        m2.pivot(pk, pk);
        ok[pk] = false;
    }

    auto m3 = KMatrix(n, n);
    for (unsigned int i = 0; i < n; i++) {
        for (unsigned int j = 0; j < n; j++) {
            m3(i, j) = m2(i, n + j);
        }
    }
    return m3;
}


KMatrix clip(const KMatrix & m, double xMin, double xMax) {
    assert (xMin <= xMax);
    const unsigned int nr = m.numR();
    const unsigned int nc = m.numC();
    auto cfn = [m, xMin, xMax] (unsigned int i, unsigned int j) {
        double mij = m(i,j);
        mij = (mij < xMin) ? xMin : mij;
        mij = (xMax < mij) ? xMax : mij;
        return mij;
    };
    return KMatrix::map(cfn, nr, nc);
}


KMatrix iMat(unsigned int n) {
    auto idm = KMatrix(n, n); // all 0.0
    for (unsigned int i = 0; i < n; i++) {
        idm(i, i) = 1.0;
    }
    return idm;
}

bool iMatP(const KMatrix & m) {
    const unsigned int nr = m.numR();
    bool okP = (nr == m.numC());
    if (okP) {
        const double tol = 1E-10;
        // tolerable round-off error on a [0,1] range
        const auto im = iMat(nr);
        okP = (norm(m - im) < tol);
    }
    return okP;
}

// return y to min |y-x| s.t. perpendicular(y,p)
KMatrix makePerp(const KMatrix & x, const KMatrix & p) {
    double lambda = dot(x,p) / dot(p,p);
    auto y = x - lambda*p;
    return y;
}

KMatrix  joinH(const KMatrix & mL, const KMatrix & mR) {
    unsigned int nr3 = mL.numR();
    unsigned int nc1 = mL.numC();
    assert(nr3 == mR.numR());
    unsigned int nc2 = mR.numC();
    auto m3 = KMatrix(nr3, nc1 + nc2);
    for (unsigned int i = 0; i < nr3; i++) {
        for (unsigned int j = 0; j < nc1; j++) {
            m3(i, j) = mL(i, j);
        }
        for (unsigned int j = 0; j < nc2; j++) {
            m3(i, j + nc1) = mR(i, j);
        }
    }
    return m3;
}


KMatrix  joinV(const KMatrix & mT, const KMatrix & mB) {
    unsigned int nr1 = mT.numR();
    unsigned int nr2 = mB.numR();
    unsigned int nc3 = mT.numC();
    assert(nc3 == mB.numC());
    auto m3 = KMatrix(nr1 + nr2, nc3);
    for (unsigned int j = 0; j < nc3; j++) {
        for (unsigned int i = 0; i < nr1; i++) {
            m3(i, j) = mT(i, j);
        }
        for (unsigned int i = 0; i < nr2; i++) {
            m3(i + nr1, j) = mB(i, j);
        }
    }
    return m3;
}

KMatrix rescaleRows(const KMatrix& m1, const double vMin, const double vMax) {
    assert(vMin < vMax);
    const unsigned int nr = m1.numR();
    const unsigned int nc = m1.numC();
    KMatrix m2 = KMatrix(nr, nc);

    for (unsigned int i = 0; i < nr; i++) {
        double rowMin = m1(i, 0);
        double rowMax = m1(i, 0);
        for (unsigned int j = 0; j < nc; j++) {
            const double mij = m1(i, j);
            if (mij < rowMin) {
                rowMin = mij;
            }
            if (mij > rowMax) {
                rowMax = mij;
            }
        }
        const double rowRange = rowMax - rowMin;
        assert(0 < rowRange);

        for (unsigned int j = 0; j < nc; j++) {
            const double mij = m1(i, j);
            const double nij = (mij - rowMin) / rowRange; // normalize into [0, 1]
            const double rij = vMin + (vMax - vMin)*nij; // rescale into [vMin, vMax]
            m2(i, j) = rij;
        }
    }


    return m2;
}

// JAH 20160809 create a matrix by reshaping from a vector
KMatrix KMatrix::vecToKmat(vector<double> vec, unsigned int nr, unsigned int nc)
{
    // be sure the size of the desired matrix is consistent with the data
    assert(vec.size() == nr*nc);

    // init to the right size ...
    auto mat = KMatrix(nr,nc,0.0);
    int rowShift = 0; // rowshift is used to "block" chunks of data in the vector to create rows
    // ... then fill in the data from the vector
    for(unsigned int r = 0; r<nr; r++)
    {
        for(unsigned int c = 0; c<nc; c++)
        {
            mat(r,c) = vec[r+rowShift+c];
        }
        rowShift += (nc-1);
    }
    return mat;
}

KMatrix KMatrix::getRow(unsigned int nr)
{
    // be sure the requested row number is < numrows
    assert(nr < this->numR());
    auto thisrow = KMatrix(1,this->numC(),0.0);
    for (unsigned int c = 0; c<this->numC(); c++)
    {
        thisrow(0,c) = (*this)(nr,c);
    }
    return thisrow;
}

} // end of namespace

// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
