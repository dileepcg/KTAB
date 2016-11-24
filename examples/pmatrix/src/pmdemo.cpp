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



#include "pmdemo.h"

using std::cout;
using std::endl;
using std::flush;
using std::get;
using std::string;


namespace PMatDemo {
using KBase::PRNG;
using KBase::Model;
using KBase::VotingRule;
using KBase::trans;


void runPMM(uint64_t s, bool cpP, const KMatrix& wMat, const KMatrix& uMat, const vector<string> & aNames) {
  assert(0 != s);
  printf("Creating PMatrixModel objects ... \n");

  auto eKEM = new PMatrixModel("PMatrixModel", s);

  eKEM->pcem = KBase::PCEModel::MarkovIPCM;

  cout << "Actor weight vector: " << endl;
  wMat.mPrintf("%6.2f  ");
  cout << endl;

  cout << "Utility(actor, option) matrix:" << endl;
  uMat.mPrintf("%5.3f  ");
  cout << endl;

  eKEM->setWeights(wMat);
  eKEM->setPMatrix(uMat);
  eKEM->setActors(aNames, aNames);

  const unsigned int numSimPol = 20;
  if (eKEM->numOptions() < numSimPol) {
    eKEM->nSim = 0;
  } else {
    eKEM->nSim = numSimPol;
  }

  const unsigned int maxIter = 1000;

  eKEM->stop = [maxIter, eKEM](unsigned int iter, const KBase::State * s) {
    bool doneP = iter > maxIter;
    if (doneP) {
      printf("Max iteration limit of %u exceeded \n", maxIter);
    }
    auto s2 = ((const PMatrixState *)(eKEM->history[iter]));
    for (unsigned int i = 0; i < iter; i++) {
      auto s1 = ((const PMatrixState *)(eKEM->history[i]));
      if (eKEM->equivStates(s1, s2)) {
        doneP = true;
        printf("State number %u matched state number %u \n", iter, i);
      }
    }
    return doneP;
  };


  const unsigned int nOpt = eKEM->numOptions();
  printf("Number of options %u \n", nOpt);
  printf("Number of actors %u \n", eKEM->numAct);


  const auto probTheta = Model::scalarPCE(eKEM->numAct, nOpt,
                                          wMat, uMat,
                                          VotingRule::Proportional,
                                          eKEM->vpm, eKEM->pcem,
                                          ReportingLevel::Silent);

  const auto p2 = trans(probTheta);
  cout << "PCE over entire option-space:" << endl;
  p2.mPrintf(" %5.3f ");

  auto zeta = wMat * uMat;
  cout << "Zeta over entire option-space:" << endl;
  zeta.mPrintf(" %5.1f ");

  auto aCorr = [](const KMatrix & x, const KMatrix &y) {
    return lCorr(x - mean(x), y - mean(y));
  };

  printf("af-corr(p2,zeta): %.3f \n", aCorr(p2, zeta));

  auto logP = KMatrix::map([](double x) {
    return log(x);
  }, p2);
  printf("af-corr(logp2,zeta): %.3f \n", aCorr(logP, zeta));

  for (unsigned int i = 0; i < nOpt; i++) {
    printf("%2u  %6.4f  %+8.3f  %5.1f  \n", i, p2(0, i), logP(0, i), zeta(0, i));
  }

  double maxZ = -1.0;
  unsigned int ndxMaxZ = 0;
  for (unsigned int i = 0; i < nOpt; i++) {
    if (zeta(0, i) > maxZ) {
      maxZ = zeta(0, i);
      ndxMaxZ = i;
    }
  }
  cout << "Central position is number " << ndxMaxZ << endl;


  auto es1 = new PMatrixState(eKEM);

  if (cpP) {
    cout << "Assigning actors to the central position" << endl;
  }
  else {
    cout << "Assigning actors to their self-interested initial positions" << endl;
  }
  for (unsigned int i = 0; i < eKEM->numAct; i++) {
    double maxU = -1.0;
    unsigned int bestJ = 0;
    for (unsigned int j = 0; j < nOpt; j++) {
      double uij = uMat(i, j);
      if (uij > maxU) {
        maxU = uij;
        bestJ = j;
      }
    }
    unsigned int ki = cpP ? ndxMaxZ : bestJ;
    auto pi = new PMatrixPos(eKEM, ki);
    es1->addPstn(pi);
  }

  es1->setUENdx();

  // test instantiation of templates
  auto sFn0 = [es1] { return es1->stepSUSN(); };
  auto sFn1 = [es1] { return es1->stepBCN(); };
  auto sFn2 = [es1] { return es1->stepMCN(); };

  es1->step = sFn2;
  eKEM->addState(es1);

  cout << "--------------" << endl;
  cout << "First state:" << endl;
  es1->show();

  eKEM->run();

  const unsigned int histLen = eKEM->history.size();
  PMatrixState* esA = (PMatrixState*)(eKEM->history[histLen - 1]);
  cout << endl << flush;
  printf("Last State %i \n", histLen - 1);
  esA->show();
  esA->setAUtil(-1, ReportingLevel::Medium);
  auto lastPDU = esA->pDist(-1);
  KMatrix pd = get<0>(lastPDU);
  VUI un = get<1>(lastPDU);
  KMatrix umh = esA->uMatH(-1);
  auto eu = umh*pd;

  cout << "Unique indices:" << endl;
  for (unsigned int i : un) {
    printf(" %2u ", i);
  }

  cout << endl ;
  cout << "Corresponding policies:  ";
  cout << endl ;
  for (unsigned int i : un) {
    auto posI = (EPosition<unsigned int>*)(esA->pstns[i]);
    auto ni = posI->getIndex();
    printf(" %2u ",ni);
  }
  cout << endl << endl;
  cout << "Policy probabilities:" << endl;
  for (unsigned int j=0; j<un.size(); j++) {
    unsigned int i = un[j];
    auto posI = (EPosition<unsigned int>*)(esA->pstns[i]);
    auto ni = posI->getIndex();
    double probI = pd(j, 0);
    printf("%2u:  %.4f \n", ni, probI);
  }
  cout << endl;
  cout << "Actor expected utilities:" << endl;
  trans(eu).mPrintf(" %.4f ");
  cout << endl << flush;
  return;
}

void fitFile(string fName, uint64_t seed) {
  const double bigR = +0.5;
  cout << "Fitting file "<<fName<<endl;
  auto fParams = pccCSV(fName);
  vector<string> aNames = get<0>(fParams);
  auto uMat = PMatrixModel::utilFromFP(fParams, bigR);

  cout << "RA-Util from outcomes: " << endl << flush;
  uMat.mPrintf(" %.4f  ");
  cout << endl << flush;
  auto c12 = PMatrixModel::minProbError(fParams, bigR, 1100.0);
  // 250 for 2016-08-27 results

  // retrieve the weight-matrices which were fitted, so we can
  // use them to assess coalitions in two different situations.
  //const double score = get<0>(c12);
  const KMatrix w1 = get<1>(c12);
  const KMatrix w2 = get<2>(c12);

  cout << endl;
  cout << "=====================================" << endl;
  cout << "EMod with BAU-case weights" << endl << flush;
  runPMM(seed, false, w1, uMat, aNames);
  cout << endl;
  cout << "=====================================" << endl;
  cout << "EMod with change-case weights" << endl << flush;
  runPMM(seed, false, w2, uMat, aNames);
  cout << endl;

  return;
}

void genPMM(uint64_t sd) {
  assert(0 != sd);
  auto rng = new PRNG(sd);

  bool cpP = (1 == (rng->uniform() % 2));
  unsigned int numAct = rng->uniform(10, 25);
  unsigned int numOpt = rng->uniform(8, 30);
  auto wMat = KMatrix::uniform(rng, 1, numAct, 5.0, 10.0);
  wMat = KMatrix::map(KBase::sqr, wMat);
  auto r1Mat = KMatrix::uniform(rng, numAct, numOpt, -1000.0, +10000.0);
  // if left totally random, there would be no reason for actors to cooperate
  // so I correlate their results into three groups
  auto avrgFn = [r1Mat, numAct](unsigned int i1, unsigned int j) {
    unsigned int i0 = ((i1 + numAct) - 3) % numAct;
    unsigned int i2 = ((i1 + numAct) + 3) % numAct;
    double rij = (r1Mat(i0, j) + r1Mat(i1, j) + r1Mat(i2, j)) / 3.0;
    return rij;
  };
  auto r2Mat = KMatrix::map(avrgFn, numAct, numOpt);
  auto uMat = KBase::rescaleRows(r2Mat, 0.0, 1.0);
  vector<string> aNames = {};
  for (unsigned int i = 0; i < numAct; i++) {
    auto ni = genName("Actor", i);
    aNames.push_back(ni);
  }

  runPMM(sd, cpP, wMat, uMat, aNames);

  delete rng;
  rng = nullptr;
  return;
}


}
// end of namespace


// -------------------------------------------------


int main(int ac, char **av) {
  using KBase::dSeed;
  using KBase::PRNG;

  auto sTime = KBase::displayProgramStart();
  uint64_t seed = dSeed;
  bool run = true;
  bool pmm = false;
  bool fit = false;
  string fitFileCSV = "";

  auto showHelp = []() {
    printf("\n");
    printf("Usage: specify one or more of these options\n");
    printf("--fit <file>  read CSV file and fit to it \n");
    printf("--help        print this message \n");
    printf("--pmm         run random PMatrixModel \n");
    printf("--seed <n>    set a 64bit seed \n");
    printf("              0 means truly random \n");
    printf("              default: %020llu \n", dSeed);
  };

  // tmp args

  if (ac > 1) {
    for (int i = 1; i < ac; i++) {
      //cout << "Argument " << i << " is =|" << av[i] << "|=" << endl << flush;
      if (strcmp(av[i], "--seed") == 0) {
        i++;
        seed = std::stoull(av[i]);
      }
      else if (strcmp(av[i], "--fit") == 0) {
        fit = true;
        i++;
        fitFileCSV = av[i];
      }
      else if (strcmp(av[i], "--pmm") == 0) {
        pmm = true;
      }
      else if (strcmp(av[i], "--help") == 0) {
        run = false;
      }
      else {
        run = false;
        printf("Unrecognized argument %s\n", av[i]);
      }
    }
  }
  else {
    run = false;
  }

  if (!run) {
    showHelp();
    return 0;
  }

  PRNG * rng = new PRNG();
  seed = rng->setSeed(seed); // 0 == get a random number
  printf("Using PRNG seed:  %020llu \n", seed);
  printf("Same seed in hex:   0x%016llX \n", seed);

  if (pmm) {
    auto pmm = PMatDemo::pmmCreation(seed);
    assert(nullptr != pmm);
    auto pmp = PMatDemo::pmpCreation(pmm);
    assert(nullptr != pmp);
    auto pms = PMatDemo::pmsCreation(pmm);
    assert(nullptr != pms);

    PMatDemo::genPMM(seed);

  }

  if (fit) {
    PMatDemo::fitFile(fitFileCSV, seed);
  }


  delete rng;
  KBase::displayProgramEnd(sTime);
  return 0;
}


// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
