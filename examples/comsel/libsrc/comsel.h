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

#ifndef COMSEL_LIB_H
#define COMSEL_LIB_H

#include <algorithm>
//#include "csv_parser.hpp"
#include "sqlite3.h"
#include "kutils.h"
#include "prng.h"
#include "kmatrix.h"
#include "gaopt.h"
#include "kmodel.h"

#include "smp.h"

namespace ComSelLib {
  // namespace to which KBase has no access
  //  using std::function;
  using std::shared_ptr;
  using std::string;
  using std::tuple;
  using std::vector;
  using KBase::newChars;
  using KBase::KMatrix;
  using KBase::PRNG;
  using KBase::VUI;
  using KBase::Actor;
  using KBase::Position;
  using KBase::State;
  using KBase::Model;
  using KBase::VotingRule;
  using KBase::ReportingLevel;
  using KBase::MtchPstn;

  using KBase::Actor;
  using KBase::Model;
  using KBase::Position;
  using KBase::State;
  using KBase::VctrPstn;

  class CSActor;
  class CSState;
  class CSModel;

  // -------------------------------------------------

  const string appName = "csdemo";
  const string appVersion = "0.3";

  // -------------------------------------------------
  // function declarations

  VUI intToVB(unsigned int x, unsigned int n);
  unsigned int vbToInt(const VUI & vb);

  // -------------------------------------------------
  // class declarations

  class CSModel : public Model {
  public:
    // JAH 20160711 added rng seed JAH 20160802 added sql flags
    explicit CSModel(unsigned int nd, PRNG* r, string d = "", uint64_t s=0, vector<bool> f={});
    virtual ~CSModel();


    static bool equivStates(const CSState * rs1, const CSState * rs2);

    // the positions are matchings.
    // the number of items is the number of actors (e.g. political parties)
    // the number of categories is always 2: out or in, respectively
    unsigned int numItm = 0;
    unsigned int numCat = 2;

    double getActorCSPstnUtil(unsigned int ai, unsigned int pj); // get [0,1] normalized utility to each actor of each CSposition
    
  protected:
    unsigned int numDims = 0;
    KMatrix * actorCSPstnUtil = nullptr; // normalized [0,1] utility to each actor (row) of each CS position (column)
    void setActorCSPstnUtil();
    
    // return the clm-vector of actors' expected utility for this particular committee
    KMatrix oneCSPstnUtil(const VUI& vb) const;
    
    KMatrix * actorSpPstnUtil = nullptr; // normalized [0,1] utility to each actor (row) of each spatial position (column)
    void setActorSpPstnUtil();
    double oneSpPstnUtil(unsigned int ai, unsigned int pj) const;
    
    // when an actor is not on the committee, its influence is divided by this factor
    double nonCommDivisor = 20.0;
    
  private:
  };


  class CSActor : public Actor {
  public:
    explicit CSActor(string n, string d,  CSModel* csm);
    virtual ~CSActor();

    VotingRule vr = VotingRule::PropBin; // fairly arbitrary default
    double sCap = 0.0; // scalar capacity, which must be positive

    double posUtil(const Position * ap1) const;

    double vote(unsigned int est,unsigned int i, unsigned int j, const State* st) const;
    double vote(const Position* ap1, const Position* ap2) const;
    
    // the CSActor has a standard vector position, with vector saliences.
    // This does not change over time as they bargain over committees.
    VctrPstn vPos = VctrPstn();
    KMatrix vSal = KMatrix();
    
    void randomize(PRNG* rng, unsigned int nDim);
  
  protected:
    CSModel* csMod = nullptr;
    
  private:
  };


  class CSState : public State {
  public:
    explicit CSState(CSModel* mod);
    virtual ~CSState();

    // use the parameters of your state to compute the relative probability of each unique position.
    // persp = -1 means use everyone's separate perspectives (i.e. get actual probabilities, not one actor's beliefs).
    // Because the voting mechanisms may differ, so v_k(i:j) could differ widely from sub-class to sub-class,
    // it is tricky to make a single function to do this.
    virtual tuple< KMatrix, VUI> pDist(int persp) const;

    CSState * stepSUSN();
    CSState * stepBCN();

    void show() const;

  protected:
    virtual void setAllAUtil(ReportingLevel rl);

    CSState * doSUSN(ReportingLevel rl) const;
    CSState * doBCN(ReportingLevel rl) const;
    
    // Given the utility matrix, uMat, calculate the expected utility to each actor,
    // as a column-vector. Again, this is from the perspective of whoever developed uMat.
    KMatrix expUtilMat (KBase::ReportingLevel rl, unsigned int numA, unsigned int numP, const KMatrix & uMat) const; 
     
    virtual bool equivNdx(unsigned int i, unsigned int j) const;

  private:
  };

};// end of namespace


// --------------------------------------------
#endif
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
