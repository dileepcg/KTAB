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
// Very simple interface to standard Mersenne Twister.
// -------------------------------------------------
#ifndef KTAB_PRNG_H
#define KTAB_PRNG_H

#include <cstdint>
#include <random>

#include "kutils.h"
#include "kmatrix.h"

namespace KBase {
using KBase::VBool;
using std::mt19937_64;


typedef uint64_t W64; // 64 bits
const unsigned int WordLength = 64;

const W64 MASK64 = 0xffffffffffffffff; // obviously 2^64 - 1
const W64 MASK32 = 0xffffffff; // obviously 2^32 - 1



// The constant e, left-shifted up to 62 bits.
// 2718281828459045235 == 0x25B946EBC0B36173
// Shifting one more decimal would be 65 bits.
const W64 Q64A = 0x25B946EBC0B36173;

// The constant pi, left shifted up to 62 bits (+1)
// 3141592653589793238 + 1 == 0x2B992DDFA23249D7
// Shifting one more decimal would be 65 bits.
const W64 Q64B = 0x2B992DDFA23249D7;

// Large odd constant to avoid fixed-points in qTrans
// phi = 1.618.. = (1+sqrt(5))/2
// 16180339887498948482 + 1 -- 64 bits
// We right-shifted the decimals and added 1 to make it odd, 64 bits
const W64 Q64C = 0xE08C1D668B756F83;

W64 qTrans(W64 s);
W64 rotl(const W64 x, unsigned int n);
W64 rotr(const W64 x, unsigned int n);

class PRNG {
public:
  explicit PRNG(uint64_t sd = KBase::dSeed);
  virtual ~PRNG();
  uint64_t uniform();
  double uniform(double a, double b);
  unsigned int probSel(const KMatrix & cv);
  VBool bits(unsigned int nb);
  uint64_t setSeed(uint64_t sd);
protected:
  mt19937_64 mt = mt19937_64();
};

};

// -------------------------------------------------
#endif
// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
