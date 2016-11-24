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

#include <assert.h> 

#include "kutils.h"
//#include "kmodel.h"
#include "agenda.h"
#include "demo.h"
using std::cout;
using std::endl;
using std::flush;
using std::vector;
using KBase::KMatrix;
using KBase::PRNG;

namespace AgendaControl {

void setupScenario0(unsigned int &numActor, unsigned int &numItems,
                    KMatrix &vals, KMatrix &caps, PRNG* rng) {

  numActor = 15;
  if (0 == numItems) {
    numItems = 7;
  }
  vals = KMatrix::uniform(rng, numActor, numItems, 0.01, 0.99);
  caps = KMatrix::uniform(rng, numActor, 1, 10.0, 99.99);
  return;
}



void setupScenario1(unsigned int &numActor, unsigned int &numItems,
                    KMatrix &vals, KMatrix &caps) {

  numActor = 22;
  numItems = 5;

  KMatrix plpM = KMatrix::arrayInit(plpA, numActor, numItems);
  KMatrix plwM = KMatrix::arrayInit(plwA, numActor, 1);

  vals = KMatrix(numActor, numItems);
  for (unsigned int i = 0; i < numActor; i++) {
    double rowMax = 0.0;
    double rowMin = 10.0;
    for (unsigned int j = 0; j < numItems; j++) {
      double vij = plpM(i, j);
      if (vij > rowMax) {
        rowMax = vij;
      }
      if (vij < rowMin) {
        rowMin = vij;
      }
    }
    for (unsigned int j = 0; j < numItems; j++) {
      double vij = plpM(i, j);
      vals(i, j) = (vij - rowMin) / (rowMax - rowMin);
    }
  }
  caps = plwM;
  return;
}

void bestAgendaChair(vector<Agenda*> ars, const KMatrix& vals, const KMatrix& caps) {
  unsigned int bestK = 0;
  double bestV = -1.0;
  const double sigDiff = 1E-5; // utility is on [0,1] scale, differences less than this are insignificant
  unsigned int numAgenda = ars.size();
  for (unsigned int ai = 0; ai < numAgenda; ai++) {
    auto ar = ars[ai];
    ars.push_back(ar);
    double v0 = ar->eval(vals, 0); //
    assert(0.0 <= v0);
    assert(v0 <= 1.0);

    if (bestV + sigDiff < v0) {
      bestV = v0;
      bestK = ai;
    }
    //  printf("%4u ", ai); cout << *ar << "  "; printf("%.4f \n", v0);
  }
  printf("Best option for agenda-setting actor 0 is %u with value %.4f  is  ", bestK, bestV);
  cout << *(ars[bestK]) << endl << flush;
  //ars[bestK]->showProbs(1.0);
  return;
}

void demoCounting(unsigned int numI, unsigned int maxU, unsigned int maxS, unsigned int maxB) {
  cout << endl << flush;
  unsigned int n = 5;
  unsigned int m = 2;
  auto cat = AgendaControl::chooseSet(n, m);

  printf("Found |chooseSet(%u, %u)| = %llu \n", n, m, cat.size());
  for (auto lst : cat) {
    for (auto i : lst) {
      printf("%i ", i);
    }
    cout << endl << flush;
  }
  assert(cat.size() == AgendaControl::numSets(n, m));
  cout << endl;

  VUI testI = {};
  for (unsigned int i = 0; i < numI; i++) {
    testI.push_back(10 * (1 + i));
  }


  for (unsigned int i = 1; i <= numI + 2; i++) {
    auto n = AgendaControl::numAgenda(i);
    printf(" %2i -> %llu \n", i, n);
  }
  cout << endl << flush;

  printf("Using %i items: ", numI);
  for (unsigned int i : testI) {
    printf("%i ", i);
  }
  cout << endl << flush;

  auto enumAg = [numI](Agenda::PartitionRule pr, std::string s) {
    vector<Agenda*> testA = Agenda::enumerateAgendas(numI, pr);
    printf("For %i items, found %i distinct %s agendas \n", numI, testA.size(), s.c_str());
    for (auto a : testA) {
      cout << *a << endl;
    }
    cout << endl << flush;
    if (Agenda::PartitionRule::FreePR == pr) {
      assert(testA.size() == AgendaControl::numAgenda(numI));
    }
    return;
  };

  if (numI <= maxU) {
    enumAg(Agenda::PartitionRule::FreePR, "unconstrained");
    enumAg(Agenda::PartitionRule::SeqPR, "sequential");
  }
  if (numI <= maxS) {
    enumAg(Agenda::PartitionRule::ModBalancedPR, "semi-balanced");
  }
  if (numI <= maxB) {
    enumAg(Agenda::PartitionRule::FullBalancedPR, "balanced");
  }

  return;
}

}; // end of namespace

int main(int ac, char **av) {
  using std::cout;
  using std::endl;
  using std::flush;
  using std::function;
  using KBase::dSeed;
  using AgendaControl::Agenda;
  using AgendaControl::Choice;
  using AgendaControl::Terminal;

  auto sTime = KBase::displayProgramStart();
  uint64_t seed = dSeed;
  bool enumP = false;
  unsigned int enumN = 0;
  bool run = true;

  auto showHelp = []() {
    printf("\n");
    printf("Usage: specify one or more of these options\n");
    printf("--help            print this message \n");
    printf("--enum <n>        enumerate various agendas over N items \n");
    printf("--seed <n>        set 64bit seed to N \n");
    printf("                  0 means truly random\n");
    printf("                  default: %020llu \n", dSeed);
  };

  // tmp args
  enumP = true;
  enumN = 6;


  if (ac > 1) {
    for (int i = 1; i < ac; i++) {
      if (strcmp(av[i], "--seed") == 0) {
        i++;
        seed = std::stoull(av[i]);
      }
      else if (strcmp(av[i], "--enum") == 0) {
        enumP = true;
        i++;
        enumN = std::stoi(av[i]);
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

  PRNG * rng = new PRNG();
  seed = rng->setSeed(seed); // 0 == get a random number
  printf("Using PRNG seed:  %020llu \n", seed);
  printf("Same seed in hex:   0x%016llX \n", seed);
  unsigned int maxU = 8;
  unsigned int maxS = 10;
  unsigned int maxB = 10;
  if (enumP) {
    AgendaControl::demoCounting(enumN, maxU, maxS, maxB);
  }

  unsigned int numActor = 0;
  unsigned int numItems = enumN;
  auto vals = KMatrix();
  auto caps = KMatrix();

  if (true) {
    AgendaControl::setupScenario0(numActor, numItems, vals, caps, rng);
  }
  else {
    AgendaControl::setupScenario1(numActor, numItems, vals, caps);
  }



  // find what's best for agenda-setting actor 0
  caps(0, 0) = caps(0, 0) / 25.0; // agenda-setter has little voting power

  cout << "Value matrix" << endl;
  vals.mPrintf(" %5.3f ");
  cout << endl << flush;


  cout << "Capability matrix" << endl;
  caps.mPrintf(" %5.2f ");
  cout << endl << flush;

  auto enumA = [numItems, vals, caps](Agenda::PartitionRule pr, std::string name) {
    cout << endl;
    cout << "Enumerating all agendas ("<<name<<") over " << numItems << " items ... ";
    auto ars = Agenda::enumerateAgendas(numItems, pr);
    printf("found %i agendas \n", ars.size());
    AgendaControl::bestAgendaChair(ars, vals, caps);
    for (auto ar : ars) {
      delete ar;
      ar = nullptr;
    }
    return;
  };

  enumA(Agenda::PartitionRule::FreePR, "FreePR");
  enumA(Agenda::PartitionRule::SeqPR, "SeqPR");
  enumA(Agenda::PartitionRule::ModBalancedPR, "MBPR");
  enumA(Agenda::PartitionRule::FullBalancedPR , "FBPR");


  delete rng;
  KBase::displayProgramEnd(sTime);
  return 0;
}
// ------------------------------------------
// Copyright KAPSARC. Open Source MIT License
// ------------------------------------------
