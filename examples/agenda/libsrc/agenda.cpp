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
// Implement a simple agenda-object, independent of kmodel.h
// --------------------------------------------
#include "agenda.h"

using std::cout;
using std::endl;
using std::flush;

namespace AgendaControl {

uint64_t fact(unsigned int n) {
  uint64_t f = 0;
  switch (n) {
  case 0:
  case 1:
    f = 1;
    break;
  case 2:
    f = 2;
    break;
  default:
    f = n * fact(n - 1);
    break;
  }
  return f;
}


uint64_t numSets(unsigned int n, unsigned int m) {
  assert(0 < n);
  assert(0 < m);
  uint64_t ns = fact(n) / (fact(m)*fact(n - m));
  return ns;
}

uint64_t numAgenda(unsigned int n) {
  // how many distinct agendas are there for n items?
  // Crucially, [x:y] == [y:x]
  assert(0 < n);
  uint64_t cn = 0;
  switch (n) {
  case 1:
    cn = 1;
    // only [a]
    break;
  case 2:
    cn = 1;
    // only [a:b]
    break;

  case 3:
    cn = 3;
    // [a:[b:c]]
    // [b:[a:c]]
    // [c:[a:b]]
    break;

  default: {
    // An agenda over n>3 items can only be a choice between sub-agendas.
    // Suppose the left has i items and the right has j=n-i items.
    // There are choose(n,i) ways to choose a unique, unordered set for the LHS,
    // and for each such choice there is only one possible RHS (all not in the LHS).
    // Because the LHS and RHS are distinct, any agendas addressing them are distinct,
    // so there are C(i) distinct ways to address the LHS, C(j) distinct ways to address the RHS,
    // and C(i)*C(j) distinct combinations.
    // We then sum those up for 0 < i < n. This does double count, as each (i,n-i)
    // division is matched by a (n-i, i) division, so the sum gets divided by 2.
    uint64_t cm = 0;
    for (unsigned int i = 1; i < n; i++) {
      unsigned int j = n - i;
      uint64_t splits = numSets(n, i);
      uint64_t lC = numAgenda(i);
      uint64_t rC = numAgenda(j);
      cm = cm + splits*lC*rC;
    }
    cn = cm / 2;
    assert(cm == (2 * cn));
  }
    break;
  }
  return cn;
}


// returns the list of all ways to choose an unordered set of m items from a set of n.
vector< vector <unsigned int> > chooseSet(const unsigned int n, const unsigned int m) {
  assert(0 < n);
  assert(0 < m);
  auto csnm = vector< vector <unsigned int> >();
  if (1 == m) {
    for (unsigned int i = 0; i < n; i++) {
      VUI ci = { i };
      csnm.push_back(ci);
    }
  }
  else {
    auto cLess = chooseSet(n, m - 1);
    for (auto lst : cLess) {
      auto len = ((const unsigned int)(lst.size()));
      assert(1 <= len);
      auto maxEl = ((const unsigned int)(lst[len - 1]));
      for (unsigned int j = maxEl + 1; j < n; j++) {
        VUI ls = lst;
        ls.push_back(j);
        csnm.push_back(ls);
      }
    }
  }
  return csnm;
}


tuple<VUI, VUI> indexedSet(const VUI xs,
                           const VUI is) {
  VUI rslt = {};
  for (auto i : is) {
    rslt.push_back(xs[i]);
  }
  VUI comp = {};
  for (unsigned int i = 0; i < xs.size() - rslt.size(); i++) {
    comp.push_back(0);
  }
  std::set_difference(xs.begin(), xs.end(),
                      rslt.begin(), rslt.end(),
                      comp.begin()
                      );
  auto pr = tuple<VUI, VUI>(rslt, comp);
  return pr;
}


vector<Agenda*> Agenda::agendaSet(PartitionRule pr, const VUI xs) {
  auto n = ((const unsigned int)(xs.size()));
  assert(0 < n);
  auto showA = [](const VUI &as) {
    printf("[");
    for (auto a : as) {
      printf(" %u ", a);
    }
    printf("]");
    return;
  };

  vector<Agenda*> as = {};

  switch (n) {
  case 1: {
    Agenda* a = new Terminal(xs[0]);
    as.push_back(a);
  }
    break;
  case 2: {
    Agenda* lsa = new Terminal(xs[0]);
    Agenda* rsa = new Terminal(xs[1]);
    Agenda* a = new Choice(lsa, rsa);
    as.push_back(a);
  }
    break;

  default: // n>2, odd or even
    for (unsigned int k = 1; k <= (n / 2); k++) {
      vector<VUI> leftIndices = {};

      leftIndices = chooseSet(n, k);
      // if n is odd, then when k= (n/2), we have k < n-k, so the
      // left-hand agenda is smaller than and distinct from the right-hand agenda.
      // However, when n is even, the latter half of the list must be discarded
      // because of symmetry.
      // Suppose n=4 for (a,b,c,d) and we choose 2.
      // ((a,b), (a,c), (a,d), (b,c), (b,d), (c,d)) is the usual set of 6 unique subsets of size 2.
      // However, if those are the left-hand agendas, then the right-hand are their complements:
      // ((c,d), (b,d), (b,c), (a,d), (a,c), (a,b)).
      // Thus, the complements of the second half are exactly the first half, so by symmetry
      // we discard the second half.
      if (n == (2 * k)) {
        auto m = ((const unsigned int)(leftIndices.size()));
        assert(m == 2 * (m / 2));
        vector<VUI> shortIndices = {};
        for (unsigned int i = 0; i < (m / 2); i++) {
          shortIndices.push_back(leftIndices[i]);
        }
        leftIndices = shortIndices;
      }

      for (auto lhi : leftIndices) {
        //showA(lhi);
        //cout << " -- ";
        auto lrPairs = indexedSet(xs, lhi);
        VUI lhs = std::get<0>(lrPairs);
        VUI rhs = std::get<1>(lrPairs);

        bool ok = Choice::balancedLR(pr, lhs.size(), rhs.size());
        if (ok) {
          auto lAgendas = agendaSet(pr, lhs);
          auto rAgendas = agendaSet(pr, rhs);
          for (auto la : lAgendas) {
            for (auto ra : rAgendas) {
              Agenda* a = new Choice(la, ra);
              as.push_back(a);
            }
          }
        }
      }
    }
    break;
  }


  if (PartitionRule::FreePR == pr) {
    assert(as.size() == numAgenda(n));
  }
  if ((2 <= n) && (PartitionRule::SeqPR == pr)) {
    // just a sorted list, of which there are n!,
    // except that the two orders of the last pair are equivalent
    auto fn = fact(n);
    assert(as.size() == (fn / 2));
  }
  return as;
}

// ------------------------------------------

vector<Agenda*> Agenda::enumerateAgendas(unsigned int n, PartitionRule pr) {
  VUI items = {};
  for (unsigned int i = 0; i < n; i++) {
    items.push_back(i);
  }


  vector<Agenda*> as1 = agendaSet(pr, items);

  vector<Agenda*> as2 = {};
  for (auto a : as1) {
    bool ok = a->balanced(pr);
    assert(ok); // should be true by construction
    if (ok) {
      as2.push_back(a);
    }
  }
  return as2;
}


unsigned int Agenda::minAgendaSize(PartitionRule pr, unsigned int n) {
  assert(0 < n); // could be 1, but not 0
  unsigned int minM = 0;
  switch (pr) {
  case PartitionRule::FullBalancedPR:
    minM = n / 2;
    break;
  case PartitionRule::ModBalancedPR:
    if (n <= 5) {
      minM = n / 2;
    }
    else { // 6 or more
      minM = n / 3;
    }
    break;

  case PartitionRule::FreePR:
  case PartitionRule::SeqPR:
    if (1 == n) {
      minM = 0;
    }
    else {
      minM = 1;
    }
    break;
  }

  return minM;
}

Agenda* Agenda::makeAgenda(vector<int> xs, PartitionRule pr, PRNG* rng) {
  auto n = ((const unsigned int)(xs.size()));
  const int minM = minAgendaSize(pr, n);


  Agenda* ap = nullptr;
  if (1 == n) {
    ap = new Terminal(xs[0]);
  }
  else {
    unsigned int m = 0;
    while ((m < minM) || (n - m < minM)) {
      m = 1 + rng->uniform() % (n - 1);
    }
    auto ys = vector<int>();
    auto zs = vector<int>();
    for (unsigned int i = 0; i < m; i++) {
      ys.push_back(xs[i]);
    }
    for (unsigned int i = m; i < n; i++) {
      zs.push_back(xs[i]);
    }
    bool ok = Agenda::balancedLR(pr, ys.size(), zs.size());
    assert(minM <= ys.size());
    assert(minM <= zs.size());
    auto a1 = makeAgenda(ys, pr, rng);
    auto a2 = makeAgenda(zs, pr, rng);
    ap = new Choice(a1, a2);
  }
  return ap;
}

// ------------------------------------------

double Choice::eval(const KMatrix& val, unsigned int i) {
  double valL = lhs->eval(val, i);
  double valR = rhs->eval(val, i);
  double valMin = (valL < valR) ? valL : valR;
  double valMax = (valL > valR) ? valL : valR;
  double ev = (4.0*valMin + 3.0*valMax) / 7.0;
  //cout << "Eval " << i << " of " << *this << " = " << ev << endl << flush;
  return ev;
};

bool Agenda::balancedLR(PartitionRule pr, unsigned int numL, unsigned int numR) {
  const unsigned int n = numL + numR;
  assert(2 <= n);

  unsigned int minN = 1;
  switch (pr) {
  case PartitionRule::FreePR:
  case PartitionRule::SeqPR:
    minN = 1;
    break;

  case PartitionRule::ModBalancedPR:
    if (n <= 5) {
      minN = n / 2;
    }
    else {
      minN = n / 3;
    }
    break;

  case PartitionRule::FullBalancedPR:
    minN = n / 2;
    break;
  };
  bool ok = (minN <= numL) && (minN <= numR);
  if (PartitionRule::SeqPR == pr) {
    ok = (1 == numL);
  }
  return ok;
}

bool Choice::balanced(PartitionRule pr) const {
  const unsigned int n = length();
  // every choice is between two items, and the shortest possible sub-agenda
  // is a Terminal of length 1, so this must be at least 2 long
  assert(2 <= n);
  unsigned int lN = lhs->length();
  unsigned int rN = rhs->length();
  bool okTop = balancedLR(pr, lN, rN);

  bool ok = okTop;
  if (okTop) {
    ok = (lhs->balanced(pr)) && (rhs->balanced(pr));
  }
  return ok;
}
// ------------------------------------------
double Terminal::eval(const KMatrix& val, unsigned int i) {
  double ev = val(i, item);
  //cout << "Eval " << i << " of " << *this << " = " << ev << endl << flush;
  return ev;
};

}; // end of namespace

// ------------------------------------------
// Copyright KAPSARC. Open Source MIT License
// ------------------------------------------
