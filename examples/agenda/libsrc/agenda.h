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
// Declare a simple agenda-object, independent of kmodel.h
// --------------------------------------------
#ifndef AGENDA_H
#define AGENDA_H

#include <iostream>
#include <algorithm>
#include <vector>
#include <iterator>

#include "kutils.h"
#include "kmatrix.h"
#include "prng.h" 

#include "kmodel.h"

// ------------------------------------------
namespace AgendaControl {
using std::function;
using std::ostream;
using std::vector;
using std::tuple;
using KBase::KMatrix;
using KBase::PRNG;
using KBase::VUI;

using KBase::Position;

class Agenda;
class Choice;
class Terminal;

uint64_t fact(unsigned int n);
uint64_t numSets(unsigned int n, unsigned int m);
uint64_t numAgenda(unsigned int n);

// returns the list of all lists that have m integers out of the first n integers,  {0, 1, 2, ... n-1}
vector< vector <unsigned int> > chooseSet(const unsigned int n, const unsigned int m);

// pick out the indicated subset
tuple<VUI, VUI> indexedSet(const VUI xs, const VUI is);


class Agenda : public Position {
public:
  enum class PartitionRule { FullBalancedPR, ModBalancedPR, FreePR, SeqPR};
  Agenda() {};
  virtual  ~Agenda() {};

  // estimate the value of this agenda to actor number i
  virtual double eval(const KMatrix& val, unsigned int i)  = 0;


  static vector<Agenda*> enumerateAgendas(unsigned int n, PartitionRule pr);
  virtual unsigned int length() const = 0;
  virtual bool balanced(PartitionRule pr) const = 0;
  static bool balancedLR(PartitionRule pr, unsigned int numL, unsigned int numR);

  // list all agendas of the given type, over the given items
  static vector<Agenda*> agendaSet(PartitionRule pr, const VUI xs);

protected:
private:
  static Agenda* makeAgenda(vector<int> xs, PartitionRule pr, PRNG* rng);
  static unsigned int minAgendaSize(PartitionRule pr, unsigned int n);
};


class Choice : public Agenda {
public:
  Choice( Agenda* la,  Agenda* ra) : Agenda() {
    lhs = la;
    rhs = ra;
  };
  virtual  ~Choice() { }; // delete lhs; delete rhs; };
  virtual double eval(const KMatrix& val, unsigned int i);
  virtual unsigned int length() const { return (lhs->length() + rhs->length()); }
  bool balanced(PartitionRule pr) const;

protected:
  virtual void print(ostream& os) const {
    os << "[" << *lhs << ":" << *rhs << "]";
    return;
  };
  Agenda* lhs = nullptr;
  Agenda* rhs = nullptr;
};


class Terminal : public Agenda {
public:
  explicit Terminal(unsigned int v = 0) : Agenda() { item = v;  };
  virtual  ~Terminal() {};
  virtual double eval(const KMatrix& val, unsigned int i);
  virtual unsigned int length() const { return 1; }
  bool balanced(PartitionRule pr) const { return true; }

protected:
  virtual void print(ostream& os) const {
    os << item;
    return;
  };
  unsigned int item=0;
};


}; // end of namespace


// ------------------------------------------
#endif
// ------------------------------------------
// Copyright KAPSARC. Open Source MIT License
// ------------------------------------------
