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
#ifndef KUTILS_DEMO_H
#define KUTILS_DEMO_H


#include "kutils.h"
#include "prng.h"
#include "kmatrix.h"
#include "gaopt.h"
#include "hcsearch.h"
#include "vimcp.h"

namespace UDemo {
// avoid namespace pollution by keeping all this demo stuff in its own namespace.
using std::tuple;
using std::vector;
using KBase::PRNG;
using KBase::KMatrix;
using KBase::VBool;

// -------------------------------------------------
double eNorm(const KMatrix & a, const KMatrix & x);
KMatrix eUnitize(const KMatrix & a, const KMatrix & x);
KMatrix projEllipse(const KMatrix & a, const KMatrix & w);
void demoEllipseLVI(PRNG* rng, unsigned int n);
void demoAntiLemke(PRNG* rng, unsigned int n);


// -------------------------------------------------
// define a simple class of boolean vectors that
// get evaluated by how close they are to a target
// Note: we re-define this behavior via lambda-fns later.

class TargetedBV {
public:
  TargetedBV();
  explicit TargetedBV(const VBool & b);
  virtual ~TargetedBV();
  static void setTarget(VBool trgt);
  static VBool getTarget();
  virtual void randomize(PRNG* rng);
  virtual TargetedBV * mutate(PRNG * rng) const;
  virtual tuple<TargetedBV*, TargetedBV*> cross(const TargetedBV * g2, PRNG * rng) const;
  virtual void show() const;
  virtual bool equiv(const TargetedBV * g2) const;
  static void showBits(VBool bv);
  static  VBool randomBV(PRNG* rng, unsigned int nb);
  double evaluate();
  double tblEval(double minD, vector<double> weights, vector<VBool> tbl) const;
  unsigned int hDist(VBool bv) const;
  VBool bits = VBool();
  static VBool target; // it is only a one-shot demo, so this can be static
};

}; // namespace

// -------------------------------------------------
#endif

// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
