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


#include "kutils.h"
#include "hcsearch.h"

namespace KBase {
using std::tuple;
using std::get;
// --------------------------------------------


vector<KMatrix> VHCSearch::vn1(const KMatrix & m0, double s) {
  unsigned int n = m0.numR();
  auto nghbrs = vector<KMatrix>();
  double pms[] = { -1, +1 };
  for (unsigned int i = 0; i < n; i++) {
    for (double si : pms) {
      KMatrix m1 = m0;
      m1(i, 0) = m0(i, 0) + (si*s);
      nghbrs.push_back(m1);
    }
  }
  return nghbrs;
}

vector<KMatrix> VHCSearch::vn2(const KMatrix & m0, double s) {
  unsigned int n = m0.numR();
  assert(1 < n);
  auto nghbrs = vector<KMatrix>();
  double pms[] = { -1, +1 };
  for (unsigned int i = 0; i < n; i++) {
    for (unsigned int j = 0; j < i; j++) {
      for (double si : pms) {
        for (double sj : pms) {
          KMatrix m1 = m0;
          m1(i, 0) = m0(i, 0) + (si*s);
          m1(j, 0) = m0(j, 0) + (sj*s);
          nghbrs.push_back(m1);
        }
      }
    }
  }
  return nghbrs;
}

VHCSearch::VHCSearch() {
  eval = nullptr;
  nghbrs = nullptr;
  report = nullptr;
}

VHCSearch::~VHCSearch() {
  // nothing yet
}

tuple<double, KMatrix, unsigned int, unsigned int>
VHCSearch::run(KMatrix p0,
               unsigned int iMax, unsigned int sMax, double sTol,
               double s0, double shrink, double grow, double minStep,
               ReportingLevel rl) {
  using std::thread;

  assert(eval != nullptr);
  assert(nghbrs != nullptr);
  unsigned int iter = 0;
  unsigned int sIter = 0;
  double currStep = s0;
  double v0 = eval(p0);
  const double vInitial = v0;

  // set the variables in this objects
  vhcBestVal = v0;
  vhcBestPoint = p0;
  const bool parP = true;

  auto showFn = [this](string preface, const KMatrix & p, double v) {
    printf("%s point: \n", preface.c_str());
    trans(p).mPrintf(" %+0.4f ");
    printf("%s value: %+.6f \n", preface.c_str(), v);
    if (nullptr != report) {
      report(p);
    }
    cout << endl << flush;
    return;
  };

  if (ReportingLevel::Low <= rl) {
    showFn("Initial", p0, v0);
  }

  while ((iter < iMax) && (sIter < sMax) && (minStep < currStep)) {
    assert(vInitial <= v0);


    // TODO: change this to use groupThreads
    auto ts = vector<thread>();
    const auto nPnts = nghbrs(p0, currStep);
    const unsigned int numPnts = nPnts.size();
    for (unsigned int i = 0; i < numPnts; i++) {
      auto pTmp = nPnts[i];
      if (parP) {
        ts.push_back(thread([pTmp, i, this]() {
          // Notice that 'eval' is not in the critical section, so we could
          // have arbitrarily many 'eval' operations running concurrently,
          // interleaved arbitrarily with test&reset in the critical section.
          // So we may eval(p1), eval(p2), test(val2), eval(p3), test(val3), test(val1)
          // and it will still correctly get the best of three values, and
          // the matching point.
          double vTmp = eval(pTmp);
          vhcEvalMtx.lock();
          if (vTmp > vhcBestVal) {
            vhcBestVal = vTmp;
            vhcBestPoint = pTmp;
          }
          vhcEvalMtx.unlock();
          return;
        }));
      }
      else {
        // sequential execution, so no need for mutex.
        double vTmp = eval(pTmp);
        if (vTmp > vhcBestVal) {
          vhcBestVal = vTmp;
          vhcBestPoint = pTmp;
        }
      }
    }

    if (parP) {
      for (unsigned int i = 0; i < ts.size(); i++) {
        auto& t = ts[i];
        t.join();
      }
    }

    // lock is necessary if multi-threaded, harmless if single-threaded
    vhcEvalMtx.lock();
    if (vhcBestVal > v0 + sTol) {
      sIter = 0;
      currStep = grow*currStep;
      v0 = vhcBestVal;
      p0 = vhcBestPoint;
    }
    else {
      sIter++;
      currStep = shrink*currStep;
    }
    vhcEvalMtx.unlock();

    assert(vInitial <= v0);

    iter++;


    if (ReportingLevel::Medium <= rl) {
      if (parP) {
        printf("After multi-threaded VHC iteration %u \n", iter);
      }
      else {
        printf("After single-threaded VHC iteration %u \n", iter);
      }
      showFn("Best current", p0, v0);
      if (nullptr != report) {
        report(p0);
      }
    }
  }

  assert(vInitial <= v0); // either stay at orig point or improve it: never less
  tuple<double, KMatrix, unsigned int, unsigned int> rslt { v0, p0, iter, sIter };

  if (ReportingLevel::Low <= rl) {
    cout << endl << flush;
    showFn("Final", p0, v0);
  }
  return rslt;
}


} // namespace KBase

// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
