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

#include <iostream>

#include "gaopt.h"
#include "kmodel.h"


bool operator==(const KBase::MtchPstn& mp1, const KBase::MtchPstn& mp2) {
  bool eqv = (mp1.numItm == mp2.numItm) && (mp1.numCat == mp2.numCat);
  for (unsigned int i = 0; i < mp1.numItm; i++) {
    eqv = eqv && (mp1.match[i] == mp2.match[i]);
  }
  return eqv;
}

namespace KBase {
using KBase::PRNG;
using KBase::KMatrix;


// --------------------------------------------
Position::Position() {}
Position::~Position() {}



// --------------------------------------------
VctrPstn::VctrPstn() : Position(), KMatrix() {}
VctrPstn::VctrPstn(unsigned int nr, unsigned int nc) : Position(), KMatrix(nr, nc) {}
VctrPstn::VctrPstn(const KMatrix & m) : KMatrix(m) {} // copy constructor
VctrPstn::~VctrPstn() {}

void VctrPstn::print(ostream& os) const {  
  // better formatting is available through KMatrix::printf
  os << "[VectrPstn ";
  for (auto v : vals) {
    os << v << " ";
  }
  os << "]";
  return;
}

// --------------------------------------------
MtchPstn::MtchPstn() : Position()  {
  numCat = 0;
  numItm = 0;
}

MtchPstn::~MtchPstn() {}

void MtchPstn::print(ostream& os) const {
  assert(numItm == match.size());
  os << "[MtchPstn ";
  for (auto m : match) {
    os << m << " ";
  }
  os << "]";
  return;
}

vector< MtchPstn > MtchPstn::neighbors(unsigned int nVar) const {
  assert(0 < nVar);
  auto nghbrs = vector<MtchPstn>();


  // vary one assignment, O(I*(A-1))
  if (1 <= nVar) {
    for (unsigned int n = 0; n < numItm; n++) {
      for (unsigned int an = 0; an < numCat; an++) {
        if (an != match[n]) {
          auto nbr = MtchPstn(*this);
          nbr.match[n] = an;
          nghbrs.push_back(nbr);
        }
      }
    }
  }


  // vary two different assignments, O(I*(A-1))^2
  if (2 <= nVar) {
    for (unsigned int n = 0; n < numItm; n++) {
      for (unsigned int m = 0; m < n; m++) {
        for (unsigned int an = 0; an < numCat; an++) {
          for (unsigned int am = 0; am < numCat; am++) {
            if ((an != match[n]) && (am != match[m])) {
              auto nbr = MtchPstn(*this);
              nbr.match[n] = an;
              nbr.match[m] = am;
              nghbrs.push_back(nbr);
            }
          }
        }
      }
    }
  }


  // vary three different assignments, O(I*(A-1))^3
  if (3 <= nVar) {
    for (unsigned int i = 0; i < numItm; i++) {
      for (unsigned int j = 0; j < i; j++) {
        for (unsigned int k = 0; k < j; k++) {
          for (unsigned int ai = 0; ai < numCat; ai++) {
            for (unsigned int aj = 0; aj < numCat; aj++) {
              for (unsigned int ak = 0; ak < numCat; ak++) {
                if ((ai != match[i]) && (aj != match[j]) && (ak != match[k])) {
                  auto nbr = MtchPstn(*this);
                  nbr.match[i] = ai;
                  nbr.match[j] = aj;
                  nbr.match[k] = ak;
                  nghbrs.push_back(nbr);
                }
              }
            }
          }
        }
      }
    }
  }


  return nghbrs;
}

// --------------------------------------------
// MtchGene inherits these data members:
// actrs: vector of the Actor* in this state (really TActor3*)
// pstns: vector of the MtchPstn of those actors.
// numA: the number of actors ( == actrs.size())
// numI: the number of items (sweets)
// match: this particular gene, from {0 ... numCat-1}^numItm
MtchGene::MtchGene() : MtchPstn() {
  assert(0 == numCat);
  assert(0 == numItm);
  actrs = vector<Actor*>();
  pstns = vector<MtchPstn*>();
  match = VUI();
}

MtchGene::~MtchGene() {};


void MtchGene::print(ostream& os) const {
  assert(numItm == match.size());
  os << "[MtchGene ";
  for (auto m : match) {
    os << m << " ";
  }
  os << "]";
  return;
}

void MtchGene::randomize(PRNG* rng) {
  assert(0 < numCat);
  assert(0 < numItm);
  match = VUI();
  for (unsigned int i = 0; i < numItm; i++) {
    unsigned int aID = ((unsigned int)(rng->uniform() % numCat));
    match.push_back(aID);
  }
  return;
}

void  MtchGene::copySelf(MtchGene* mg2) const {
  assert(0 < numCat);
  assert(0 < numItm);
  assert(numItm == match.size());

  mg2->numCat = numCat;
  mg2->numItm = numItm;
  mg2->actrs = actrs;
  mg2->pstns = pstns;
  mg2->match = match;

  return;
}

MtchGene * MtchGene::mutate(PRNG * rng) const {
  // because quid-pro-quo can be expected, we mutate two chromosomes
  auto mg2 = new MtchGene();
  copySelf(mg2);

  unsigned int n1 = ((unsigned int)(rng->uniform() % numItm));
  unsigned int a1 = ((unsigned int)(rng->uniform() % numCat));
  mg2->match[n1] = a1;

  unsigned int n2 = ((unsigned int)(rng->uniform() % numItm));
  unsigned int a2 = ((unsigned int)(rng->uniform() % numCat));
  mg2->match[n2] = a2;

  return mg2;
}

tuple<MtchGene*, MtchGene*>  MtchGene::cross(const MtchGene * mg2, PRNG * rng) const {
  auto gA = new MtchGene();
  auto gB = new MtchGene();
  copySelf(gA);
  mg2->copySelf(gB);
  unsigned int nc = crossSite(rng, numItm);
  for (unsigned int i = 0; i < numItm; i++) {
    unsigned int c1i = match[i];
    unsigned int c2i = mg2->match[i];
    if (i < nc) {
      gA->match[i] = c1i;
      gB->match[i] = c2i;
    }
    else {
      gA->match[i] = c2i;
      gB->match[i] = c1i;
    }
  }
  return  tuple<MtchGene*, MtchGene*>(gA, gB);
}


bool MtchGene::equiv(const MtchGene * mg2) const {
  assert((numItm == mg2->numItm) && (numCat == mg2->numCat));
  bool e = (*this == *mg2);
  return e;
}


void MtchGene::setState(vector<Actor*> as, vector<MtchPstn*> ps)  {
  actrs = as;
  pstns = ps;
}


} // end of namespace

// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
