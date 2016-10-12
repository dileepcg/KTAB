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
// this model relies on an explicit enumeration of all possible
// outcomes/positions: a few tens of thousands of discrete choices.
// PT is the position-type

template <class PT>
class EModel : public Model {
public:
    // JAH 20160711 added rng seed 20160730 JAH added sql flags
    explicit EModel(string d = "", uint64_t s=KBase::dSeed, vector<bool> = {});
    virtual ~EModel();

    virtual void setOptions(); // just error-check before calling 'enumOptions'
    unsigned int numOptions() const;
    PT* nthOption(unsigned int i) const;


    // you have to provide these λ-fns

    // Enumerate theta, the set of options
    function <vector <PT*> ()> enumOptions = nullptr;

protected:
    vector <PT*> theta = {}; // the enumerated space of all possible positions/outcomes
    static const unsigned int minNumOptions = 3;

private:
};




// -------------------------------------------------
// enumerated positions are just handles that index into the set theta.
//
template <class PT>
class EPosition : public Position {
public:
    EPosition(EModel<PT>* m, int n);
    virtual ~EPosition();

protected:
    virtual void print(ostream& os) const;

    int ndx = -1;

private:
    const EModel<PT>* eMod = nullptr; // saves a lot of type-casting later

};

// -------------------------------------------------
template <class PT>
class EState : public State {
public:
    explicit EState( EModel<PT>* mod);
    virtual ~EState();
    void setValues();

    void show() const;

    EState<PT>* stepSUSN();

    // TODO: verify pDist
    // use the parameters of this state to compute the relative probability of
    // each actor's position. persp = -1 means use everyone's separate perspectives
    //(i.e. get actual probabilities, not one actor's beliefs)
    tuple <KMatrix, VUI> pDist(int persp) const;


protected:

    void setAllAUtil(ReportingLevel rl);

    // you have to provide these λ-fns.

    // probably using the EModel's raw-value matrix, actorVFn,
    // compute and set the aUtils vector of matrices,
    // where (aUtils[h])(i,j) = h's estimate of the utility to actor i of position held by actor j.
    function <vector<KMatrix>()> getAUtils = nullptr;

    // Calculate the values to the actors of the tj-th option, theta[j].
    // Note that this may depend on their current position, as in the SMP.
    // If the value does NOT depend on the current state (e.g. most CGE models),
    // then put it in EModel::baseUtils and just look it up here.
    //
    // Some models, like CGE, automatically produce the results for all actors at once,
    // given the policy, so it would be quite inefficient to run the model over
    // and over for each (actor, policy) pair.
    // Of course, you might do it that way, but you are not required to do so.
    // It might be more efficient to calculate a big data object the first time
    // any part of it is needed, and index into it when other parts are needed.
    //
    // This has to be lambda-bound to the relevant parameters.
    function <vector<double>(unsigned int tj, const EModel<PT>*)> actorVFn = nullptr;


    EState<PT>* doSUSN(ReportingLevel rl) const;

    // Given the utility matrix, uMat, calculate the expected utility to each actor,
    // as a column-vector. Again, this is from the perspective of whoever developed uMat.
    KMatrix  expUtilMat  (KBase::ReportingLevel rl, unsigned int numA, unsigned int numP,  KBase::VPModel vpm, const KMatrix & uMat) const;


    EModel<PT>*  eMod = nullptr; // saves a lot of type-casting later

    // TODO: finish equivNdx
    // determine if the i-th position in this state is equivalent to the j-th position
    virtual bool equivNdx(unsigned int i, unsigned int j) const;

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
