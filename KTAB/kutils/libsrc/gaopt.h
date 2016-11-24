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
// Define a few generally useful optimizers.
// -------------------------------------------------
#ifndef GAOPT_H
#define GAOPT_H

#include <assert.h>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

#include "prng.h"
#include "kutils.h"

namespace KBase {
using std::cout;
using std::endl;
using std::flush;
using std::function;
using std::get;
using std::tuple;
using std::vector;

// -------------------------------------------------

unsigned int crossSite(PRNG* rng, unsigned int nc);

// -------------------------------------------------

template <class GAP>
class GAOpt {
public:
  explicit GAOpt(unsigned int s);
  virtual ~GAOpt();

  void init(vector < GAP* > ipop);
  void fill(PRNG* rng);
  void run(PRNG* rng, double c, double m,
           unsigned int maxI, double sTh, unsigned int maxS,
           ReportingLevel srl,
           unsigned int & iter, unsigned int &sIter);
  tuple<double, GAP* > getNth(unsigned int n);
  void show();

  // lambda functions that must be supplied to define your particular problem
  function <tuple<GAP*, GAP*>(const GAP* g1, const GAP* g2, PRNG* rng)> cross = nullptr;
  function <GAP* (const GAP* g1, PRNG* rng)> mutate = nullptr;
  function <double(const GAP* g1)> eval = nullptr;
  function <void(const GAP*)> showGene = nullptr;
  function <GAP* (PRNG* rng)> makeGene = nullptr;
  function <bool(const GAP* g1, const GAP* g2)> equiv = nullptr;

  // If you provide the appropriate methods in a GAP class,
  // the lambdas can be quite simple:
  // cross = [](const GAP* g1, const GAP* g2, PRNG* rng) { return g1->cross(g2, rng); };
  // mutate = [](const GAP* g1, PRNG* rng)               { return (g1->mutate(rng));  };
  // equiv = [](const GAP* g1, const GAP* g2)            { return g1->equiv(g2);      };
  // showGene = [](const GAP* g1)                        { g1->show(); return;        };
  // makeGene = [](const GAP* g1, PRNG* rng)             { return (GAP::random(rng)); };

  void sortPop();

protected:
  void step();
  void mutatePop();
  void crossPop();
  void dropDups();
  void selectPop();
  vector < tuple<double, GAP* >> gpool = {};
  unsigned int pSize = 0;
  double cFrac = 1.0;
  double mFrac = 0.5;
  GAP* mutateOne(const GAP* g1, PRNG* rng);
  tuple<GAP*, GAP*> crossPair(const GAP* g1, const GAP* g2, PRNG* rng);
  void cyclicApply(function <void(unsigned int i)> fn, double f);
  PRNG* rng = nullptr;
  std::mutex cycAppMtx;

private:
  // nothing yet
};

template<class GAP>
GAOpt<GAP>::GAOpt(unsigned int s) {
  assert(1 < s); // long enough to do a crossover
  pSize = s;
  cFrac = 0;
  mFrac = 0;

  gpool = vector< tuple<double, GAP*>>();
  gpool.resize(pSize);

  for (unsigned int i = 0; i < pSize; i++) {
    gpool[i] = tuple <double, GAP*>(0, nullptr);
  }

  cross = nullptr;
  mutate = nullptr;
  eval = nullptr;
  showGene = nullptr;
  makeGene = nullptr;
  equiv = nullptr;
}

template<class GAP>
GAOpt<GAP>::~GAOpt() {
  for (auto pr : gpool) {
    delete get<1>(pr);
  }
}


template<class GAP>
void GAOpt<GAP>::run(PRNG* rng, double c, double m,
                     unsigned int maxI, double sTh, unsigned int maxS,
                     ReportingLevel srl,
                     unsigned int & iter, unsigned int &sIter) {
  assert(cross != nullptr);
  assert(mutate != nullptr);
  assert(eval != nullptr);
  assert(showGene != nullptr);
  assert(makeGene != nullptr);
  assert(equiv != nullptr);
  iter = 0;
  sIter = 0;
  //double oldBest = 0.0;
  //double newBest = 0.0;
  bool runP = true;

  assert((0 <= c) && (0 <= m) && (0 < c + m));
  cFrac = c;
  mFrac = m;
  assert(0 < maxS);
  assert(0 < sTh);
  assert(maxS < maxI);

  auto vn = [this](unsigned int i) {
    auto pri = getNth(i);
    double vi = get<0>(pri);
    return vi;
  };
  sortPop();
  while (runP) {
    auto pri = getNth(0);
    double oldBest = get<0>(pri);
    step();
    pri = getNth(0);
    double newBest = get<0>(pri);
    double dv = newBest - oldBest;
    assert(0.0 <= dv);

    sIter = (sTh < dv) ? 0 : sIter + 1;
    iter++;
    runP = (iter < maxI) && (sIter < maxS);

    if (ReportingLevel::Low < srl) {
      printf("%u/%u iterations    %u/%u stable \n", iter, maxI, sIter, maxS);
      printf("newBest value: %+.4f up %+.4f  \n", newBest, dv);
      cout << "newBest gene: ";
      showGene(get<1>(pri));
      cout << endl;
      if (ReportingLevel::Medium < srl) {
        show();
      }
      cout << endl << endl << flush;
    }
  }
  if (ReportingLevel::Silent < srl) {
    auto pri = getNth(0);
    printf("Search completed after %u/%u iterations    %u/%u stable \n", iter, maxI, sIter, maxS);
    printf("best value: %+.4f  \n", get<0>(pri));
    cout << "best gene: ";
    showGene(get<1>(pri));
    cout << endl << endl << flush;
  }
  return;
}


template<class GAP>
tuple<double, GAP* > GAOpt<GAP>::getNth(unsigned int n) {
  assert(n < gpool.size()); // check here
  return gpool[n];
}

template<class GAP>
void GAOpt<GAP>::sortPop() {
  auto prBefore = [this] (tuple<double, GAP*> pri, tuple<double, GAP*> prj) {
    double vi = get<0>(pri);
    double vj = get<0>(prj);
    bool pb = (vi > vj);
    return pb;
  };

  /*
    auto ps = ((const unsigned int)(gpool.size()));
    for (unsigned int i = 0; i < ps; i++) {
        for (unsigned int j = i + 1; j < ps; j++) {
            auto pri = getNth(i);
            double vi = get<0>(pri);
            auto prj = getNth(j);
            double vj = get<0>(prj);
            if (vi < vj) {
                gpool[i] = prj;
                gpool[j] = pri;
            }
        }
    }
    */

  std::sort(gpool.begin(), gpool.end(), prBefore);

  return;
}

template<class GAP>
void GAOpt<GAP>::dropDups() {
  auto cSize = ((const unsigned int)(gpool.size()));
  VBool unique = {};
  unique.resize(cSize);
  for (unsigned int i = 0; i < cSize; i++) {
    unique[i] = true;
  }
  for (unsigned int i = 0; i < cSize; i++) {
    GAP* gi = get<1>(getNth(i));
    for (unsigned int j = 0; j < i; j++) {
      GAP* gj = get<1>(getNth(j));
      if (equiv(gi, gj)) {
        unique[i] = false;
      }
    }
  }
  auto newGP = vector<tuple<double, GAP*>>();
  for (unsigned int i = 0; i < cSize; i++) {
    auto pri = getNth(i);
    if (unique[i]) {
      newGP.push_back(pri);
    }
    else {
      get<0>(pri) = 0.0;
      GAP* gi = get<1>(pri);
      delete gi;
      get<1>(pri) = nullptr;
    }
  }
  gpool.clear();
  while (0 < newGP.size()) {
    auto pr = popBack(newGP);
    assert(nullptr != get<1>(pr));
    gpool.push_back(pr);
  }
  return;
}

template <class GAP>
void GAOpt<GAP>::selectPop() {
  sortPop();
  while (pSize < gpool.size()) {
    auto pr = KBase::popBack(gpool);
    GAP * g = get<1>(pr);
    assert(nullptr != g);
    delete g;
  }
  return;
}




template <class GAP>
void GAOpt<GAP>::cyclicApply(function <void(unsigned int i)> fn, double f) {
  while (1 <= f) {
    groupThreads(fn, 0, pSize-1, 0);
    // for (unsigned int i = 0; i < pSize; i++) { fn(i); }
    f = f - 1.0;
  }

  if (f <= 0.0) {
    return;
  }

  // now (0 < f < 1)
  const unsigned int n = ((unsigned int)(0.5 + (f * pSize)));

  const function <void(unsigned int i)> gn = [this, fn] (unsigned int) {
    unsigned int j = rng->uniform() % pSize; // 'existing' pool, not unevaluated additions
    fn(j);
    return;
  };

  groupThreads(gn, 0, n-1, 0);
  /*
    for (unsigned int i = 0; i < n; i++) {
        unsigned int j = rng->uniform() % pSize; // 'existing' pool, not unevaluated additions
        fn(j);
    }
    */

  return;
}


template <class GAP>
void GAOpt<GAP>::crossPop() {

  auto bundle = [this](GAP* g) {
    double v = eval(g);
    auto pr = tuple<double, GAP*>(v, g);
    return pr;
  };

  auto cFn = [this, bundle](unsigned int i) {
    assert (i <pSize);
    unsigned int j = rng->uniform() % pSize; // 'existing' pool, not unevaluated additions
    GAP* gi = get<1>(getNth(i));
    GAP* gj = get<1>(getNth(j));
    auto pr = cross(gi, gj, rng);
    auto pr0 = bundle(get<0>(pr));
    auto pr1 = bundle(get<1>(pr));
    cycAppMtx.lock();
    const unsigned int s1 = gpool.size();
    gpool.push_back(pr0);
    gpool.push_back(pr1);
    const unsigned int s2 = gpool.size();
    assert (s2 == 2 + s1); // correct locking?
    cycAppMtx.unlock();
    return;
  };

  cyclicApply(cFn, cFrac);
  return;
}


template <class GAP>
void GAOpt<GAP>::mutatePop() {
  auto mFn = [this](unsigned int i) {
    GAP* gi = get<1>(getNth(i));
    GAP* mg = mutate(gi, rng);
    double mgv = eval(mg);
    auto mpr = tuple<double, GAP*>(mgv, mg);
    cycAppMtx.lock();
    const unsigned int s1 = gpool.size();
    gpool.push_back(mpr);
    const unsigned int s2 = gpool.size();
    assert (s2 == 1 + s1);
    cycAppMtx.unlock();
    return;
  };
  cyclicApply(mFn, mFrac);
  return;
}

template <class GAP>
void GAOpt<GAP>::show() {
  for (unsigned int i = 0; i < gpool.size(); i++) {
    auto pri = gpool[i];
    auto vi = get<0>(pri);
    auto gi = get<1>(pri);
    printf("%4u  %8.3f   ", i, vi);
    cout << flush;
    assert(nullptr != gi);
    showGene(gi);
    cout << endl << flush;
  }
  return;
}

template <class GAP>
void GAOpt<GAP>::step() {
  assert(pSize == gpool.size());
  mutatePop();
  crossPop();
  dropDups();
  assert(pSize <= gpool.size());
  selectPop();
  assert(pSize == gpool.size());
  return;
}


template <class GAP>
void GAOpt<GAP>::fill(PRNG* r) {
  assert(eval != nullptr);
  assert(makeGene != nullptr);
  assert(nullptr != r);
  rng = r;
  //const unsigned int ps = gpool.size();
  for (unsigned int i = 0; i < gpool.size(); i++) {
    auto pri = gpool[i];
    if (nullptr == get<1>(pri)) {
      GAP* gi = makeGene(rng);
      double vi = eval(gi);
      gpool[i] = tuple<double, GAP*>(vi, gi);
    }
  }
  return;
}


template <class GAP>
void GAOpt<GAP>::init(vector < GAP* > ipop) {
  assert(eval != nullptr);
  assert(ipop.size() <= pSize);
  for (unsigned int i = 0; i < ipop.size(); i++) {
    auto pri = getNth(i);
    auto tgi = get<1>(pri);
    assert(nullptr == tgi);
    auto gi = ipop[i];
    assert(nullptr != gi);
    double vi = eval(gi);
    auto pvi = tuple<double, GAP*>(vi, gi);
    gpool[i] = pvi;
  }
  return;
}


}; // namespace

// -------------------------------------------------
#endif
// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------

