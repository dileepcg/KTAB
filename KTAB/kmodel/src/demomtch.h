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

#ifndef DEMO_MTCH_H
#define DEMO_MTCH_H

#include <assert.h>
#include <chrono>
#include <cstring>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <tuple>
#include <vector>

#include "kutils.h"
#include "prng.h"
#include "kmatrix.h"
#include "gaopt.h"
#include "hcsearch.h"
#include "kmodel.h"

namespace DemoMtch {
// namespace to which KBase has no access

using std::string;
using std::tuple;
using std::vector;

using KBase::KMatrix;
using KBase::PRNG;

using KBase::Actor;
using KBase::Position;
using KBase::State;
using KBase::Model;
using KBase::VotingRule;
using KBase::ReportingLevel;
 using KBase::VUI;

using KBase::MtchPstn;
using KBase::MtchGene;

class MtchActor;
class MtchState;
class MtchModel;

// -------------------------------------------------
void demoDivideSweets(uint64_t s);
void demoMaxSupport(uint64_t s);
void demoMtchSUSN(uint64_t s);
void multiMtchSUSN(uint64_t s);
bool oneMtchSUSN(uint64_t s);

void showMtchPstn(const MtchPstn & mp);
bool stableMtchState(unsigned int iter, const State* s);

// -------------------------------------------------
class MtchActor : public Actor {
public:
    enum class PropModel {
        ExpUtil, Probability, AgreeUtil
    };

    MtchActor(string n, string d);
    ~MtchActor();
    double vote(unsigned int est,unsigned int i, unsigned int j, const State* st) const;
    virtual double vote(const Position * ap1, const Position * ap2) const;
    double posUtil(const Position * ap1) const;

    static MtchPstn* rPos(unsigned int numI, unsigned int numA, PRNG * rng);
    static MtchActor* rAct(unsigned int numI, double minCap, double maxCap, PRNG* rng, unsigned int i);

    void randomize(PRNG* rng, double minCap, double maxCap, unsigned int id, unsigned int numI);

    tuple<double, MtchPstn> maxProbEUPstn(PropModel pm, const MtchState * mst) const;

    unsigned int idNum = 0;

    VotingRule vr =  VotingRule::Proportional;;
    PropModel pMod = PropModel::ExpUtil;

    // scalar capacity, positive
    double sCap = 0;

    // Similar to the LeonActor, this is a listing of how
    // much this actor values each item.
    // The values are all non-negative and sum to 1:
    // lowest utility, 0, to get nothing, and
    // highest utility, 1, to get everything.
    vector<double> vals = {};

protected:

private:

};

class MtchState : public State {
public:
    explicit MtchState(Model* mod);
    ~MtchState();

    KMatrix actrCaps() const;


    // use the parameters of your state to compute the relative probability of each actor's position.
    // persp = -1 means use everyone's separate perspectives (i.e. get actual probabilities, not one actor's beliefs)
    tuple <KMatrix, VUI>  pDist(int persp) const;


    MtchState * stepSUSN();

    MtchState * stepBCN();

protected:
    MtchState * doSUSN(ReportingLevel rl) const;
    MtchState * doBCN(ReportingLevel rl) const;

    bool equivNdx(unsigned int i, unsigned int j) const;
    // bool stableMtchState(unsigned int iter, const State* s);
    
    void setAllAUtil(ReportingLevel rl);

private:

};


class MtchModel : public Model {
public:
    // JAH 20160711 added rng seed JAH 20160730 JAH added sql flags
    explicit MtchModel(string d="", uint64_t s=KBase::dSeed, vector<bool> f={});
    virtual ~MtchModel();

    static MtchModel* randomMS(unsigned int numA, unsigned int numI, VotingRule vr, MtchActor::PropModel pMod, uint64_t seed);

    unsigned int numItm = 0;
    unsigned int numCat = 0;  // happens to equal numAct, in this demo

protected:

private:

};

}; // end of namespace

// -------------------------------------------------
#endif

// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
