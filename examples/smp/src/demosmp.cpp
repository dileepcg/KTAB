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
//
// Demonstrate a very basic, but highly parameterized, Spatial Model of Politics.
//
// --------------------------------------------

#include "smp.h"
#include "demosmp.h"
#include <functional>



using KBase::PRNG;
using KBase::KMatrix;
using KBase::Actor;
using KBase::Model;
using KBase::Position;
using KBase::State;
using KBase::VotingRule;
using KBase::VPModel;


namespace DemoSMP {

using std::cout;
using std::endl;
using std::flush;
using std::function;
using std::get;
using std::string;
using std::vector;

using KBase::ReportingLevel;

using SMPLib::SMPModel;
using SMPLib::SMPActor;
using SMPLib::SMPState;

// -------------------------------------------------

void demoEUSpatial(unsigned int numA, unsigned int sDim, bool accP, uint64_t s,  vector<bool> f) {
  printf("Using PRNG seed: %020llu \n", s);
  // JAH 20160711 added rng seed 20160730 JAH added sql flags
  auto md0 = new SMPModel( "", s, f);
  //rng->setSeed(s); // seed is now set in Model::Model
  if (0 == numA) {
    double lnMin = log(4);
    double lnMax = log(25);
    // median is 10  = exp( [log(4)+log(25)] /2 ) = sqrt(4*25)
    double na = exp(md0->rng->uniform(lnMin, lnMax));
    numA = ((unsigned int)(na + 0.5)); // i.e. [5,10] inclusive
  }
  if (0 == sDim) {
    sDim = 1 + (md0->rng->uniform() % 3); // i.e. [1,3] inclusive
  }

  cout << "EU State for SMP actors with scalar capabilities" << endl;
  printf("Number of actors; %u \n", numA);
  printf("Number of SMP dimensions %u \n", sDim);

  assert(0 < sDim);
  assert(2 < numA);

  for (unsigned int i = 0; i < sDim; i++) {
    auto buff = KBase::newChars(100);
    sprintf(buff, "SDim-%02u", i);
    md0->addDim(buff);
    delete buff;
    buff = nullptr;
  }
  assert(sDim == md0->numDim);


  SMPState* st0 = new SMPState(md0);
  md0->addState(st0); // now state 0 of the histor

  st0->step = [st0]() {
    return st0->stepBCN();
  };

  for (unsigned int i = 0; i < numA; i++) {
    //string ni = "SActor-";
    //ni.append(std::to_string(i));
    unsigned int nbSize = 15;
    char * nameBuff = new char[nbSize];
    for (unsigned int j = 0; j < nbSize; j++) {
      nameBuff[j] = (char)0;
    }
    sprintf(nameBuff, "SActor-%02u", i);
    auto ni = string(nameBuff);
    delete[] nameBuff;
    nameBuff = nullptr;
    string di = "Random spatial actor";

    auto ai = new SMPActor(ni, di);
    ai->randomize(md0->rng, sDim);
    auto iPos = new VctrPstn(KMatrix::uniform(md0->rng, sDim, 1, 0.0, 1.0)); // SMP is always on [0,1] scale
    md0->addActor(ai);
    st0->addPstn(iPos);
  }

  for (unsigned int i = 0; i < numA; i++) {
    auto ai = ((SMPActor*)(md0->actrs[i]));
    double ri = 0.0; // st0->aNRA(i);
    printf("%2u: %s , %s \n", i, ai->name.c_str(), ai->desc.c_str());
    string vrs = KBase::nameFromEnum<VotingRule>(ai->vr, KBase::VotingRuleNames);
    cout << "voting rule: " << vrs << endl;
    cout << "Pos vector: ";
    VctrPstn * pi = ((VctrPstn*)(st0->pstns[i]));
    trans(*pi).mPrintf(" %+7.4f ");
    cout << "Sal vector: ";
    trans(ai->vSal).mPrintf(" %+7.4f ");
    printf("Capability: %.3f \n", ai->sCap);
    printf("Risk attitude: %+.4f \n", ri);
    cout << endl;
  }

  auto aMat = KBase::iMat(md0->numAct);
  if (accP) {
    cout << "Using randomized matrix for ideal-accomodation"<<endl<<flush;
    for (unsigned int i=0; i<md0->numAct; i++) {
      aMat(i,i) = md0->rng->uniform(0.1, 0.5); // make them lag noticably
    }
  }
  else {
    cout << "Using identity matrix for ideal-accomodation"<<endl<<flush;
  }
  cout << "Accomodate matrix:"<<endl;
  aMat.mPrintf(" %.3f ");
  cout << endl;

  st0->setAccomodate(aMat);
  st0->idealsFromPstns();
  st0->setUENdx();
  st0->setAUtil(-1, ReportingLevel::Silent);
  st0->setNRA(); // TODO: simple setting of NRA

  // with SMP actors, we can always read their ideal position.
  // with strategic voting, they might want to advocate positions
  // separate from their ideal, but this simple demo skips that.
  auto uFn1 = [st0](unsigned int i, unsigned int j) {
    auto ai = ((SMPActor*)(st0->model->actrs[i]));
    auto pj = ((VctrPstn*)(st0->pstns[j])); // aj->iPos;
    double uij = ai->posUtil(pj, st0);
    return uij;
  };

  auto u = KMatrix::map(uFn1, numA, numA);
  cout << "Raw actor-pos util matrix" << endl;
  u.mPrintf(" %.4f ");
  cout << endl << flush;

  auto w = st0->actrCaps(); //  KMatrix::map(wFn, 1, numA);

  // no longer need external reference to the state
  st0 = nullptr;

  // arbitrary but illustrates that we can do an election with arbitrary
  // voting rules - not necessarily the same as the actors would do.
  auto vr = VotingRule::Binary;
  string vrs = KBase::nameFromEnum<VotingRule>(vr, KBase::VotingRuleNames);
  cout << "Using voting rule " << vrs << endl;

  const KBase::VPModel vpm = md0->vpm;
  const KBase::PCEModel pcem = md0->pcem;

  KMatrix p = Model::scalarPCE(numA, numA, w, u, vr, vpm, pcem, ReportingLevel::Medium);

  cout << "Expected utility to actors: " << endl;
  (u*p).mPrintf(" %.3f ");
  cout << endl << flush;

  cout << "Net support for positions: " << endl;
  (w*u).mPrintf(" %.3f ");
  cout << endl << flush;

  auto aCorr = [](const KMatrix & x, const KMatrix & y) {
    using KBase::lCorr;
    using KBase::mean;
    return  lCorr(x - mean(x), y - mean(y));
  };

  // for nearly flat distributions, and nearly flat net support,
  // one can sometimes see negative affine-correlations because of
  // random variations in 3rd or 4th decimal places.
  printf("L-corr of prob and net support: %+.4f \n", KBase::lCorr((w*u), trans(p)));
  printf("A-corr of prob and net support: %+.4f \n", aCorr((w*u), trans(p)));

  SMPModel::configExec(md0);

  delete md0;
  md0 = nullptr;

  return;
}



} // end of namespace


int main(int ac, char **av) {
  using std::cout;
  using std::endl;
  using std::string;
  using KBase::dSeed;
  auto sTime = KBase::displayProgramStart(DemoSMP::appName, DemoSMP::appVersion);
  uint64_t seed = dSeed;
  bool run = true;
  bool euSmpP = false;
  bool randAccP = false;
  bool csvP = false;
  string inputCSV = "";

  bool xmlP = false;
  string inputXML = "";

  bool logMin = false;

  auto showHelp = []() {
    printf("\n");
    printf("Usage: specify one or more of these options\n");
    printf("--help            print this message\n");
    printf("--euSMP           exp. util. of spatial model of politics\n");
    printf("--ra              randomize the adjustment of ideal points with euSMP \n");
    printf("--csv <f>         read a scenario from CSV \n");
    printf("--xml <f>         read a scenario from XML \n");
    printf("--logmin          log only scenario information + position histories\n");
    printf("--seed <n>        set a 64bit seed\n");
    printf("                  0 means truly random\n");
    printf("                  default: %020llu \n", dSeed);
  };

  if (ac > 0) {
    for (int i = 1; i < ac; i++) {
      if (strcmp(av[i], "--seed") == 0) {
        i++;
        seed = std::stoull(av[i]);
      }
      else if (strcmp(av[i], "--csv") == 0) {
        csvP = true;
        i++;
        inputCSV = av[i];
      }
      else if (strcmp(av[i], "--xml") == 0) {
        xmlP = true;
        i++;
        inputXML = av[i];
      }
      else if (strcmp(av[i], "--euSMP") == 0) {
        euSmpP = true;
      }
      else if (strcmp(av[i], "--ra") == 0) {
        randAccP = true;
      }
      else if (strcmp(av[i], "--help") == 0) {
        run = false;
      }
      else if (strcmp(av[i], "--logmin") == 0) {
        logMin = true;
      }
      else {
        run = false;
        printf("Unrecognized argument %s\n", av[i]);
      }
    }
  }
  else {
    run = false; // no arguments supplied
  }

  // JAH 20160730 vector of SQL logging flags for 5 groups of tables:
  // 0 = Information Tables, 1 = Position Tables, 2 = Challenge Tables,
  // 3 = Bargain Resolution Tables, 4 = VectorPosition table
  // JAH 20161010 added group 4 for only VectorPos so it can be logged alone
  std::vector<bool> sqlFlags = {true,true,true,true,true};
  // JAH 20161010 default is to log all tables, if --logmin is passed, only
  // enable logging groups 0 & 4
  if (logMin)
  {
    sqlFlags = {true,false,false,false,true};
  }

  if (!run) {
    showHelp();
    return 0;
  }

  if (0 == seed) {
    PRNG * rng = new PRNG();
    seed = rng->setSeed(seed); // 0 == get a random number
    delete rng;
    rng = nullptr;
  }

  printf("Using PRNG seed:  %020llu \n", seed);
  printf("Same seed in hex:   0x%016llX \n", seed);

  // note that we reset the seed every time, so that in case something
  // goes wrong, we need not scroll back too far to find the
  // seed required to reproduce the bug.
  if (euSmpP) {
    cout << "-----------------------------------" << endl;
    DemoSMP::demoEUSpatial(0, 0, randAccP, seed, sqlFlags);
  }
  if (csvP) {
    cout << "-----------------------------------" << endl;
    SMPLib::SMPModel::csvReadExec(seed, inputCSV, sqlFlags, "testSMP.db");
  }
  if (xmlP) {
    cout << "-----------------------------------" << endl;
    SMPLib::SMPModel::xmlReadExec(inputXML, sqlFlags, "testSMP.db");
  }
  cout << "-----------------------------------" << endl;


  KBase::displayProgramEnd(sTime);
  return 0;
}

// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
