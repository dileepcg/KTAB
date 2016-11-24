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
// Simple model of negotiating over the order of reform priorities
// -------------------------------------------------
#ifndef REF_PRI_LIB
#define REF_PRI_LIB

#include <algorithm>
#include <assert.h>
#include <iostream>
#include <set>
#include <string>
#include <tinyxml2.h>
#include <vector>

#include <kutils.h>
#include <kmatrix.h>
#include <prng.h>


#include "kmodel.h"

using namespace std;


namespace RfrmPri {
// namespace to hold everything related to the
// "priority of reforms" CDMP. Note that KBase has no access.

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

class RPActor;
class RPState;
class RPModel;

// -------------------------------------------------

const string appName = "rpdemo";
const string appVersion = "0.2";


// -------------------------------------------------
// return vector of neighboring 1- and 2-permutations
vector <MtchPstn>  nghbrPerms(const MtchPstn & mp0);

// -------------------------------------------------
// class declarations

class RPActor : public Actor {
public:
  enum class PropModel {
    ExpUtil, Probability, AgreeUtil
  };

  RPActor(string n, string d, const RPModel* rm);
  ~RPActor();
  double vote(unsigned int est, unsigned int p1, unsigned int p2, const State* st) const;
  virtual double vote(const Position * ap1, const Position * ap2) const;
  double posUtil(const Position * ap1) const;

  static MtchPstn* rPos(unsigned int numI, unsigned int numA, PRNG * rng); // make a random position
  static RPActor* rAct(unsigned int numI, double minCap,
                       double maxCap, PRNG* rng, unsigned int i); // make a random actor

  void randomize(PRNG* rng, double minCap, double maxCap, unsigned int id, unsigned int numI); // randomly alter this actor

  tuple<double, MtchPstn> maxProbEUPstn(PropModel pm, const RPState * rpst) const;

  unsigned int idNum = 0;

  PropModel pMod = PropModel::ExpUtil;

  VotingRule vr = VotingRule::PropBin; // fairly arbitrary default
  double sCap = 0.0; // scalar capacity, which must be positive

  // Similar to the LeonActor, this is a listing of how
  // much this actor values each reform item.
  // The values are all non-negative
  vector<double> riVals{};

  // given those riVals, we need to know the minimum and maximum values
  // of positions so as to normalize utilities to the [0,1] von Neumann scale.
  // Note that we start with min > max to indicate that they are not initialized.
  double posValMin = 1.0;
  double posValMax = 0.0;

  const RPModel *rpMod = nullptr;
  // these particular actors need model-parameters to compute utility.

protected:
private:
};


class RPModel : public Model {
public:
  // JAH 20160711 added rng seed JAH 20160802 added sqlflags
  explicit RPModel(string d = "", uint64_t s = KBase::dSeed, vector<bool> f = {});
  virtual ~RPModel();

  static RPModel* randomMS(unsigned int numA, unsigned int numI,
                           VotingRule vr, RPActor::PropModel pMod, PRNG * rng);

  double utilActorPos(unsigned int ai, const VUI &pstn) const;

  unsigned int govBudget = 0;
  KMatrix  govCost = KMatrix();
  double pDecline = 0.850;
  vector<double> prob = {};
  double obFactor = 0.1;
  unsigned int numItm = 0; // number of reform items
  vector<string> rpNames = {};

  // happens to equal numItm, in this demo, as the categories are 1, 2, ... numItm.
  unsigned int numCat = 0;

  void initScen(unsigned int ns);
  void readXML(string fileName);
  void showHist() const;

  static bool equivStates(const RPState * rs1, const RPState * rs2);

protected:
  void initScen0(); // random
  void initScen1(); // fixed, but dummy data
  void initScen2Avrg(unsigned int ns); // unfinished
  void initScen3Top4(unsigned int ns); // unfinished
  void configScen(unsigned int numA, const double aCap[], const KMatrix & utils);

private:
};


class RPState : public State {
public:
  explicit RPState(Model* mod);
  ~RPState();
  //KMatrix actrCaps() const;

  // use the parameters of your state to compute the relative probability of
  // each actor's position. persp = -1 means use everyone's separate perspectives
  //(i.e. get actual probabilities, not one actor's beliefs)
  tuple <KMatrix, VUI> pDist(int persp) const;
  RPState * stepSUSN();
  RPState * stepBCN();

  void show() const;



protected:
  virtual void setAllAUtil(ReportingLevel rl);
  void setOneAUtil(unsigned int perspH, ReportingLevel rl);

  RPState * doSUSN(ReportingLevel rl) const;
  RPState * doBCN(ReportingLevel rl) const;

  // Given the utility matrix, uMat, calculate the expected utility to each actor,
  // as a column-vector. Again, this is from the perspective of whoever developed uMat.
  KMatrix  expUtilMat(KBase::ReportingLevel rl, unsigned int numA, unsigned int numP, KBase::VPModel vpm, const KMatrix & uMat) const;

  const RPModel * rpMod = nullptr; // saves a lot of type-casting later


  // determine if the i-th position in this state is equivalent to the j-th position
  virtual bool equivNdx(unsigned int i, unsigned int j) const;

private:
};



} // end of namespace


#endif
// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
