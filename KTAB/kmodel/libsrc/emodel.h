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
// Define the top-level class of enumerated KTAB models.
//
// Examples of explicitly enumerated policy-spaces include the following.
// 1-dim SMP, {0/100, 1/100, ..., 99/100, 100/100}, with 101 positions.
// 2-dim SMP, with 10K positions.
// Set of sequential agendas over N items, with (N!)/2 positions
// Set of actors to be included (or not) in a committee, with 2-dim SMP to follow.
// and many more.
//
// Initially, the reformpri and comsel actually have enumerated spaces,
// so the things which appear in both are clear candidates to get raised up
// into EModel, EState, etc.
//
// Some issue spaces are too large to enumerate, so they must be treated as "implicit".
// They include the following.
// Revenue-neutral tax/subsidy policy over 20 items
// 3-, 4- or 5-dim SMP.
//
// Obviously, the SMP can be done as enumerated or implicit, and they
// must agree (within round-off error) for 1- and 2-dim.
// The only way to ensure that is with code-sharing.
//
// -------------------------------------------------
#ifndef KTAB_EMODEL_H
#define KTAB_EMODEL_H



#include <sqlite3.h>

#include "kutils.h"
#include "kmatrix.h"
#include "prng.h"
#include "kmodel.h"

namespace KBase {
using std::shared_ptr;
using std::string;
using std::tuple;
using std::vector;

// -------------------------------------------------

template <class PT> class EState;

// -------------------------------------------------
// this model relies on an explicit enumeration of all possible
// outcomes/positions: a few tens of thousands of discrete choices.
// PT is the position-type, which is assumed to be small and simple
// so that copying is not a problem.

template <class PT>
class EModel : public Model {
  friend class EState<PT>;
public:
  // JAH 20160711 added rng seed 20160730 JAH added sql flags
  explicit EModel(string d = "", uint64_t s=KBase::dSeed, vector<bool> = {});
  virtual ~EModel();

  virtual void setOptions(); // just error-check before calling 'enumOptions'
  unsigned int numOptions() const;
  PT nthOption(unsigned int i) const;

  // number of similar policies to use in searches.
  // default is to use all known policies.
  unsigned int nSim = 0;

  // two states are equivalent if all actors have the same EPosition in each.
  // derived classes may need to extend this to take into account the
  // iternal state of actors in each EState.
  virtual bool equivStates(const EState<PT>* es1, const EState<PT>*  es2) const;

  // you have to provide these λ-fns

  // Enumerate theta, the set of options
  function <vector <PT> ()> enumOptions = nullptr;

  // the row-vector of actor's scalar capabilities
  KMatrix actorWeights() const;

protected:
  vector <PT> theta = {}; // the enumerated space of all possible positions/outcomes
  static const unsigned int minNumOptions = 3;

private:
};




// -------------------------------------------------
// Enumerated position is just handle for index into the set theta.
// The set Theta is small enough to be explicitly mostly-enumerated, eg 10K or less.
// They are assumed to be small simple objects, like string, integer, or
// small vector of integers.
template <class PT>
class EPosition : public Position {
public:
  EPosition(EModel<PT>* m, int n);
  virtual ~EPosition();
  int getIndex() const {
    return ndx;
  }
protected:

  // print the ndx to the output stream
  virtual void print(ostream& os) const;

  // index into Theta, which must be 0 or more:
  // a negative value marks uninitialized state.
  int ndx = -1;

private:
  const EModel<PT>* eMod = nullptr; // saves a lot of type-casting later
};

// -------------------------------------------------
template <class PT>
class EState : public State {
  friend class EModel<PT>;
public:
  explicit EState( EModel<PT>* mod);
  virtual ~EState();

  void show() const;

  EState<PT>* stepSUSN();
  EState<PT>* stepBCN();
  EState<PT>* stepMCN();

  // use the parameters of this state to compute the relative probability of
  // each actor's position. persp = -1 means use everyone's separate perspectives
  //(i.e. get actual probabilities, not one actor's beliefs)
  tuple <KMatrix, VUI> pDist(int persp) const;

  // get the matrix of utility for unique occupied positions,
  // as estimated by h (which might be -1).
  KMatrix uMatH(int h) const;

  // determine if the i-th position in this state is equivalent to the j-th position
  virtual bool equivNdx(unsigned int i, unsigned int j) const;

  // given an index into Theta, find the N policies most similar.
  // If |Theta|<N, then it just returns all of Theta.
  // If N=0, then it returns a domain-dependent number of "similar" options.
  // EState::powerWeightedSimilarity is an obvious candidate similarity-measure.
  virtual VUI similarPol(unsigned int ti, unsigned int numPol = 0) const = 0;

  // get the index into Theta from the i-th position of this state.
  unsigned int posNdx(const unsigned int i) const;

protected:
  // Notice that the makeNewState will have to use the 'model' of
  // that state which calls makeNewState
  virtual EState<PT>* makeNewEState() const = 0;
  virtual void setAllAUtil(ReportingLevel rl) = 0;

  // Calculate the values to the actors of the tj-th option, theta[j].
  // This needs to be done for any possible position, not just those
  // currently advocated.
  // Note that this may depend on their current position, as in the SMP.
  // If the value does NOT depend on the current state (e.g. most CGE models),
  // then put it in a const matrix (e.g. PMatrixModel::polUtilMat) then look it up.
  //
  // Some models, like CGE, automatically produce the results for all actors at once,
  // given the policy, so it would be quite inefficient to run the model over
  // and over for each (actor, policy) pair.
  // Of course, you might do it that way, but you are not required to do so.
  // It might be more efficient to calculate a big data object the first time
  // any part of it is needed, and index into it when other parts are needed.
  //
  //
  virtual vector<double> actorUtilVectFn( int h, int tj) const = 0;


  EState<PT>* doSUSN(ReportingLevel rl) const;
  EState<PT>* doBCN(ReportingLevel rl) const;
  EState<PT>* doMCN(ReportingLevel rl) const;

  // get the matrix of utility for unique occupied positions,
  // as estimated by h.
  KMatrix uMatH(unsigned int h) const;

  // Given the utility matrix, uMat, calculate the expected utility to each actor,
  // as a column-vector. Again, this is from the perspective of whoever developed uMat.
  KMatrix  expUtilMat  (KBase::ReportingLevel rl, unsigned int numA, unsigned int numP,  KBase::VPModel vpm, const KMatrix & uMat) const;

  KMatrix hypExpUtilMat () const;

  EModel<PT>*  eMod = nullptr; // saves a lot of type-casting later
  
  
  // This is an attempt to define a domain-independent measure of similarity,
  // by looking at the difference in outcomes to actors.
  // Notice that if we sort the columns by difference from #ti,
  // those with small differences in outcome might do it by very different
  // means, so that columns from all over the matrix are placed near ti.
  VUI powerWeightedSimilarity(const KMatrix& uMat, unsigned int ti, unsigned int nSim) const;

private:
};

// -------------------------------------------------
// enumerated positions are just handles that index into the set theta.
//
template <class PT>
class EActor : public Actor {
public:
  EActor(EModel<PT>* m, string n, string d);
  virtual ~EActor();

  //default voting rule is proportional
  KBase::VotingRule vr = KBase::VotingRule::Proportional;

  // simple scalar capability
  double sCap = 0.0;


  // Vote between positions occupied by two different actors in the same
  // state, just looking up stored information.
  // Note well: it cannot be assumed that the vote between two
  // options can be determined simply by looking at the difference
  // in stored utilities.
  virtual double vote(unsigned int est,unsigned int p1, unsigned int p2, const State* st) const;

protected:
  const EModel<PT>* eMod = nullptr; // saves a lot of type-casting later


private:

};


}; // end of namespace



// -------------------------------------------------
#endif
// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
