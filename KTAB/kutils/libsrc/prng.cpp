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

#include "prng.h"


namespace KBase {
  using std::cout;
  using std::endl;
  using std::flush;

  /*
  The q transformation is 1-to-1, mod 2^w, for any w>0,
  when a is arbitrary, n is positive even, and c is odd.

  Suppose there were two 0 <= j < i < 2^w where q(i) == q(j) mod 2^w.
  This leads to a contradiction, odd == even.
  First, q(i) = n*i^2 + (an+c)*i + ac.

  n*i^2 + (an+c)*i + ac  ==  n*j^2 + (an+c)*j + ac + k*s^w
  n*(i^2 - j^2) + (an+c)(i-j) = k*s^w
  [ n*(i+j) + (an+c)] * (i-j) = k*2^w
  As n is even, n(i+j) is even regardless of i and j.
  As n even, a arbitrary, and c odd, (an+c) is odd.
  Thus, the equation is [odd] * (i-j) = k * 2^w.
  If i-j is odd then this is impossible, as it would require odd * odd = even.

  So let's suppose i-j is even. group all factors of 2 so i-j = m*2^z.
  m must be odd, or else we could move another 2 into 2^z.
  Now we have
  [ odd ] * (odd * 2^z) = k*2^w
  [ odd ] * (odd)  = k * 2^(w-z).

  As 0 <= j < i < 2^w, we must have z <= w.
  If w > z, then the RHS is even, so again we have the contradiction odd * odd = even.
  If w = z, then i = j + m*2^w, so i == j mod 2^w, contrary to assumption that i != j mod 2^w.
--------------------
Fixed points.
Suppose a=0, n=2, c=1. We can easily find i s.t. q(i)==i mod 2^w.
i * (2i+1) = i + k*2^w
2i^2 + i = i + k*2^w
i^2 = k * 2^(w-1)

If w=64, i^2 = k*2*2^62, or i = sqrt(2k)*2^32. whenever k=2*j^2, i = 2j,
so there is fixed point for every even multiple, (2j)*2^33.

Suppose a is odd, n=2, c=1. Is there an i s.t. q(i) == i mod 2^w?
Suppose so:
(a+i)*(2i+1) = i + k * 2^w
2i(a+i) + (a+i) = i + k*2^w
2i(a+i) + a = k*2^w.
The first term on the LHS is even, and the second is odd, so the LHS is odd.
But the RHS is even, so no such fixed point can exist.

This generalizes: With odd a, positive even n, and odd c there are no fixed points.
Suppose i were such a fixed point.
(a+i)*(n*i + c) = i + k*2^w
ni(a+i) + ac + ic = i + k*2^w
ni(a+i) + ac + (c-1)i = k * 2^w.
The first LHS is even, the second is odd, and the third is even.
Again, the LHS is odd while the RHS is even,
so no i can be a fixed point.

The values of (a+i)(ni+c) increase quadratically in i.
q(i+1)-q(i) = n(2i+1) + (an+c).
For small values of i, this can be quite predictable.
Therefore, we pick an a-value large enough to 'wrap around'
quickly, while remaining 1-to-1 without fixed points.
  */

W64 qTrans(W64 i) {
    W64 n = 2; // even, != 0
    W64 c = 3; // odd
    W64 a = Q64C; // large and odd
    W64 r = (i + a) * ((n*i) + c);
    return r;
}

W64 rotl(const W64 x, unsigned int n) {
    W64 z = x;
    n = n % WordLength; // drop excessive loops
    if (0 != n) {
      z = (x << n) | (x >> (WordLength - n));
    }

    assert(z == (z & MASK64)); // just double-check
    assert(x == rotr(z, n)); // just double-check
    return z;
  }

  W64 rotr(const W64 x, unsigned int n) {
    W64 z = x;
    n = n % WordLength; // drop excessive loops
    if (0 != n) {
      z = (x >> n) | (x << (WordLength - n));
    }

    assert(z == (z & MASK64)); // just double-check
    return z;
  }


  PRNG::PRNG(uint64_t sd) { 
    setSeed(sd);
  }


  PRNG::~PRNG() { }


  uint64_t PRNG::setSeed(uint64_t s) {
    if (0 == s) {
      std::random_device rd;
      mt19937_64 mt1(rd());
      std::uniform_int_distribution<uint64_t> dist(0, 0xFFFFFFFFFFFFFFFF);
      s = dist(mt1);
    }
    mt.seed(s);
    return s;
  }


  double PRNG::uniform(double a, double b) {
    uint64_t n = uniform();
    double x = ((double)n) / ((double)0xFFFFFFFFFFFFFFFF);
    assert(0.0 <= x);
    assert(x <= 1.0);
    x = a + ((b - a)*x);
    return x;
  }

  unsigned int PRNG::probSel(const KMatrix & cv) {
    const unsigned int nr = cv.numR();
    assert(0 < nr);
    assert(1 == cv.numC());
    const double pTol = 1E-8;
    assert(fabs(KBase::sum(cv) - 1.0) < pTol);

    int iMax = -1;
    const double p = uniform(0.0, 1.0);
    double sum = 0.0;
    for (unsigned int i = 0; (i < nr) && (iMax < 0); i++) {
      sum = sum + cv(i, 0);
      if (p <= sum) {
        iMax = i;
      }
    }
    if (iMax < 0) { // round-off error
      iMax = nr - 1;
    }
    // obviously, now 0 <= iMax <= nr-1
    return ((unsigned int)iMax);
  };


  uint64_t PRNG::uniform() {
    const uint64_t max = 0xFFFFFFFFFFFFFFFF;
    std::uniform_int_distribution<uint64_t> dist(0, max);
    uint64_t n = dist(mt);
    return qTrans(n);
  }

  VBool PRNG::bits(unsigned int nb) {
    VBool bv = {};
    bv.resize(nb);
    uint64_t rNum = uniform(); // random bits
    const int nAvail = 64;
    int nUsed = 0;
    for (unsigned int i = 0; i < nb; i++) {
      bool b = (1 == (rNum & 0x1));
      rNum = rNum >> 1;
      nUsed = nUsed + 1;
      if (nAvail <= nUsed) {
        rNum = uniform();
        nUsed = 0;
      }
      bv[i] = b;
    }
    return bv;
  }

} // end of namespace

// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
