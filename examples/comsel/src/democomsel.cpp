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
// Demonstrate a very basic, but highly parameterizable, model of committee selection
//
// --------------------------------------------


#include <cstdio>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Button.H>

#include "kutils.h"
#include "kmatrix.h"
#include "prng.h"
#include "kmodel.h"

#include "smp.h"
#include "comsel.h"
#include "csmain.h"
#include "democomsel.h"


using KBase::VUI;
using KBase::PRNG;
using KBase::KMatrix;
using KBase::Actor;
using KBase::Model;
using KBase::Position;
using KBase::State;
using KBase::VotingRule;


namespace DemoComSel {

  using std::cout;
  using std::endl;
  using std::flush;
  using std::function;
  using std::get;
  using std::string;
  using std::printf;

  using KBase::ReportingLevel;
  using KBase::MtchPstn;

  using ComSelLib::CSModel;
  using ComSelLib::CSActor;
  using ComSelLib::CSState;

  using ComSelLib::intToVB;
  using ComSelLib::vbToInt;

  // -------------------------------------------------
  // like printVUI, but with range-checking and boolean output
  void printCS(const VUI& v) {
    unsigned int n = v.size();
    printf("[CS ");
    for (unsigned int i = 0; i < n; i++) {
      switch (v[i]) {
      case 0:
        printf("-");
        break;
      case 1:
        printf("+");
        break;
      default:
        printf("printCS:: unrecognized case");
        assert(false);
        break;
      }
    }
    printf("]");
    return;
  }

  void demoActorUtils(const uint64_t s, PRNG* rng) {
    printf("Using PRNG seed: %020llu \n", s);
    rng->setSeed(s);
    return;
  }


  void demoCSC(unsigned int numA, unsigned int nDim, 
               bool cpP, bool siP, const uint64_t s) {
    // need a template for ...
    // position type  (each is a committee)
    // utility to an actor of a position (committee)
    // getting actor's scalar capability

    printf("Using PRNG seed: %020llu \n", s);
    auto trng = new PRNG();
    trng->setSeed(s);

    if (0 == numA) { numA = 2 + (trng->uniform() % 4); } // i.e. [2,6] inclusive 
    if (0 == nDim) { nDim = 1 + (trng->uniform() % 7); } // i.e. [1,7] inclusive 

    printf("Num parties: %u \n", numA);
    printf("Num dimensions: %u \n", nDim);


    unsigned int numItm = numA;
    unsigned int numCat = 2; // out or in, respectively
    unsigned int numPos = exp2(numA); // i.e. numCat ^^ numItm
    vector<VUI> positions = {};
    for (unsigned int i = 0; i < numPos; i++) {
      const VUI vbi = intToVB(i, numA);
      const unsigned int j = vbToInt(vbi);
      assert(j == i);
      const VUI vbj = intToVB(j, numA);
      assert(vbj == vbi);
      positions.push_back(vbi);
    }
    assert(numPos == positions.size());

    printf("Num positions: %u \n", numPos);

    auto ndfn = [](string ns, unsigned int i) {
      auto ali = KBase::newChars(10 + ((unsigned int)(ns.length())));
      std::sprintf(ali, "%s%i", ns.c_str(), i);
      auto s = string(ali);
      delete ali;
      ali = nullptr;
      return s;
    };

    // create a model to hold some random actors
    auto csm = new CSModel(nDim, "csm0", s);

    cout << "Configuring actors: randomizing" << endl;
    for (unsigned int i = 0; i < numA; i++) {
      auto ai = new CSActor(ndfn("csa-", i), ndfn("csaDesc-", i), csm);
      ai->randomize(csm->rng, nDim);
      csm->addActor(ai);
    }
    assert(csm->numAct == numA);
    csm->numItm = numA;
    assert(csm->numCat == 2);

    if ((9 == numA) && (2 == nDim)) {
      for (unsigned int i = 0; i < 3; i++) {
        auto ci = KMatrix::uniform(csm->rng, nDim, 1, 0.1, 0.9);
        for (unsigned int j = 0; j < 3; j++) {
          auto ej = KMatrix::uniform(csm->rng, nDim, 1, -0.05, +0.05);
          const KMatrix pij = clip(ci + ej, 0.0, 1.0);
          unsigned int n = (3 * i) + j;
          auto an = ((CSActor *)(csm->actrs[n]));
          an->vPos = VctrPstn(pij);
        }
      }
    }

    cout << "Scalar positions of actors (fixed) ..." << endl;
    for (auto a : csm->actrs) {
      auto csa = ((CSActor*)a);
      printf("%s v-position: ", csa->name.c_str());
      trans(csa->vPos).mPrintf(" %5.2f");
      printf("%s v-salience: ", a->name.c_str());
      trans(csa->vSal).mPrintf(" %5.2f");
      cout << endl;
    }
    cout << endl << flush;

    cout << "Getting scalar strength of actors ..." << endl;
    KMatrix aCap = KMatrix(1, csm->numAct);
    for (unsigned int i = 0; i < csm->numAct; i++) {
      auto ai = ((const CSActor *)(csm->actrs[i]));
      aCap(0, i) = ai->sCap;
    }

    aCap = (100.0 / sum(aCap)) * aCap;
    cout << "Scalar strengths: " << endl;
    for (unsigned int i = 0; i < csm->numAct; i++) {
      auto ai = ((CSActor *)(csm->actrs[i]));
      ai->sCap = aCap(0, i);
      printf("%3i  %6.2f \n", i, ai->sCap);
    }

    cout << "Computing utilities of positions ... " << endl;
    for (unsigned int i = 0; i < numA; i++) {
      // (0,0) causes computation of entire table
      double uii = csm->getActorCSPstnUtil(i, i); 
    }

    // At this point, we have almost a generic enumerated model,
    // so much of the code below should be easily adaptable to EModel.

    // rows are actors, columns are all possible position
    KMatrix uij = KMatrix(numA, numPos);  
    cout << "Complete (normalized) utility matrix of all possible positions (rows)";
    cout <<" versus actors (columns)" << endl << flush;
    for (unsigned int pj = 0; pj < numPos; pj++) {
      printf("%3i  ", pj);
      auto pstn = positions[pj];
      printCS(pstn);
      printf("  ");
      for (unsigned int ai = 0; ai < numA; ai++) {
        const double uap = csm->getActorCSPstnUtil(ai, pj);
        uij(ai, pj) = uap;
        printf("%6.4f, ", uap);
      }
      cout << endl << flush;
    }

    cout << endl << "Computing best position for each actor" << endl;
    vector<VUI> bestAP = {}; // list of each actor's best position (followed by CP)
    for (unsigned int ai = 0; ai < numA; ai++) {
      unsigned int bestJ = 0;
      double bestV = 0;
      for (unsigned int pj = 0; pj < numPos; pj++) {
        if (bestV < uij(ai, pj)) {
          bestJ = pj;
          bestV = uij(ai, pj);
        }
      }
      printf("Best for %02u is %4u ", ai, bestJ);
      printCS(positions[bestJ]);
      cout << endl;
      bestAP.push_back(positions[bestJ]);
    }

    trans(aCap).mPrintf("%5.2f ");
    cout << endl << flush;

    
    // which happens to indicate the PCW *if* proportional voting,
    // when we actually use PropBin
    cout << "Computing zeta ... " << endl; 
    KMatrix zeta = aCap * uij;
    assert((1 == zeta.numR()) && (numPos == zeta.numC()));

    cout << "Sorting positions from most to least net support ..." << endl << flush;
    auto betterPR = [](tuple<unsigned int, double, VUI> pr1,
      tuple<unsigned int, double, VUI> pr2) {
      double v1 = get<1>(pr1);
      double v2 = get<1>(pr2);
      bool better = (v1 > v2);
      return better;
    };

    auto pairs = vector<tuple<unsigned int, double, VUI>>();
    for (unsigned int i = 0; i < numPos; i++) {
      auto pri = tuple<unsigned int, double, VUI>(i, zeta(0, i), positions[i]);
      pairs.push_back(pri);
    }

    sort(pairs.begin(), pairs.end(), betterPR);

    const unsigned int maxDisplayed = 256;
    unsigned int  numPr = (pairs.size() < maxDisplayed) ? pairs.size() : maxDisplayed;

    cout << "Displaying highest " << numPr << endl << flush;
    for (unsigned int i = 0; i < numPr; i++) {
      auto pri = pairs[i];
      unsigned int ni = get<0>(pri);
      double zi = get<1>(pri);
      VUI pi = get<2>(pri);

      printf(" %3u: %4u  %7.2f  ", i, ni, zi);
      printCS(pi);
      cout << endl << flush;
    }

    VUI bestCS = get<2>(pairs[0]);

    bestAP.push_back(bestCS); // last one is the CP


    auto css0 = new CSState(csm);
    csm->addState(css0);

    // Either start them all at the CP or have each choose an initial position which
    // maximizes their direct utility, regardless of expected utility.
    for (unsigned int i = 0; i < numA; i++) {
      auto pi = new  MtchPstn();
      pi->numCat = numCat;
      pi->numItm = numItm;
      if (cpP) {
        pi->match = bestCS;
      }
      if (siP) {
        pi->match = bestAP[i];
      }
      css0->addPstn(pi);
    }
    assert(numA == css0->pstns.size());

    css0->step = [css0]() {
      return css0->stepSUSN();
    };

    unsigned int maxIter = 1000;
    csm->stop = [maxIter, csm](unsigned int iter, const KBase::State * s) {
      bool doneP = iter > maxIter;
      if (doneP) {
        printf("Max iteration limit of %u exceeded \n", maxIter);
      }
      auto s2 = ((const CSState *)(csm->history[iter]));
      for (unsigned int i = 0; i < iter; i++) {
        auto s1 = ((const CSState *)(csm->history[i]));
        if (CSModel::equivStates(s1, s2)) {
          doneP = true;
          printf("State number %u matched state number %u \n", iter, i);
        }
      }

      return doneP;
    };

    css0->setUENdx();


    csm->run();

    return;
  }

  void demoCSG(unsigned int nParty, unsigned int nDim, const uint64_t s) {
    printf("Using PRNG seed: %020llu \n", s);
    auto rng = new PRNG();
    rng->setSeed(s);

    if (0 == nParty) {
      nParty = 2 + (rng->uniform() % 4); // i.e. [2,6] inclusive
    }
    if (0 == nDim) {
      nDim = 1 + (rng->uniform() % 7); // i.e. [1,7] inclusive
    }

    printf("Num parties: %u \n", nParty);
    printf("Num dimensions: %u \n", nDim);

    Fl::scheme("standard"); // standard, plastic, gtk+, gleam

    auto mw = new CSMain();
    mw->mainWindow->show();
    Fl::run();
    delete mw;
    mw = nullptr;

    return;
  }


} // end of namespace


int main(int ac, char **av) {
  using std::cout;
  using std::endl;
  using std::string;
  using KBase::dSeed;
  auto sTime = KBase::displayProgramStart(DemoComSel::appName, DemoComSel::appVersion);
  uint64_t seed = dSeed; // arbitrary
  bool run = true;
  bool guiP = false;
  bool cpP = false;
  bool siP = true;

  auto showHelp = []() {
    printf("\n");
    printf("Usage: specify one or more of these options\n");
    printf("--gui       show empty GUI \n");
    printf("--cp        start each actor from the central position \n");
    printf("--si        start each actor from their most self-interested position \n");
    printf("            If neither cp nor si are specified, it will use si \n");
    printf("            If both cp and si are specified, it will use the second specified \n");
    printf("--help      print this message \n");
    printf("--seed <n>  set a 64bit seed \n");
    printf("            0 means truly random \n");
    printf("            default: %020llu \n", dSeed);
  };

  // tmp args
  // seed = 0;

  if (ac > 1) {
    for (int i = 1; i < ac; i++) {
      if (strcmp(av[i], "--seed") == 0) {
        i++;
        seed = std::stoull(av[i]);
      }
      else if (strcmp(av[i], "--gui") == 0) {
        guiP = true;
      }
      else if (strcmp(av[i], "--cp") == 0) {
        siP = false;
        cpP = true;
      }
      else if (strcmp(av[i], "--si") == 0) {
        cpP = false;
        siP = true;
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

  if (!run) {
    showHelp();
    return 0;
  }

  if (0 == seed) {
    PRNG * rng = new PRNG();
    seed = rng->setSeed(0); // 0 == get a random number
    delete rng;
    rng = nullptr;
  }
  printf("Using PRNG seed: %020llu \n", seed);
  printf("Same seed in hex: 0x%016llX \n", seed);


  cout << "Creating objects from SMPLib ... " << endl << flush;
  auto sm = new SMPLib::SMPModel("", seed); // , "SMPScen-010101"
  auto sa = new SMPLib::SMPActor("Bob", "generic spatial actor");
  delete sm;
  sm = nullptr;
  delete sa;
  sa = nullptr;
  cout << endl;
  cout << "Done creating objects from SMPLib." << endl << flush;

  if (guiP) { // dummy GUI
    DemoComSel::demoCSG(6, 5, seed);
  }
  else {
    // note that we reset the seed every time, so that in case something
    // goes wrong, we need not scroll back too far to find the
    // seed required to reproduce the bug.
    DemoComSel::demoCSC(9, // actors trying to get onto committee
      2, // issues to be addressed by the committee
      cpP, siP,
      seed);
  }
   
  KBase::displayProgramEnd(sTime);
  return 0;
}

// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
