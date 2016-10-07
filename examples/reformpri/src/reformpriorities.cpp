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
// Define the functions and methods unique to the Priority case.
// -------------------------------------------------

#include "reformpriorities.h"
#include <tuple>

namespace RfrmPri {
  // namespace to hold everything related to the
  // "priority of reforms" CDMP. Note that KBase has no access.

  using namespace std;

  using KBase::KMatrix;
  using KBase::PRNG;
  using KBase::VUI;
  using KBase::printVUI;

  // -------------------------------------------------
  // function definitions

  // return the list of the most self-interested position of each actor,
  // with the CP last.
  // As a side-affect, set each actor's min/max permutation values so as to
  // compute normalized utilities later.
  vector<VUI> scanPositions(const RPModel * rpm) {
    unsigned int numA = rpm->numAct;
    unsigned int numRefItem = rpm->numItm;
    assert(numRefItem == rpm->numCat);

    printf("There are %u actors and %u reform items \n", numA, numRefItem);

    

    KMatrix aCap = KMatrix(1, numA);
    for (unsigned int i = 0; i < numA; i++) {
      auto ri = ((const RPActor *)(rpm->actrs[i]));
      aCap(0, i) = ri->sCap;
    }
    cout << "Actor capabilities: " << endl;
    aCap.mPrintf(" %.2f ");
    cout << endl;
    
    
    cout << "Effective gov cost of items:" << endl;
    (rpm->govCost).mPrintf("%.3f ");
    cout << endl; 
    cout << "Government budget: " << rpm->govBudget << endl << flush;
    assert (0 < rpm->govBudget);
    
    
    cout << "Value to actors (rows) of individual reform items (columns):"<<endl; 
     for (unsigned int i=0; i<rpm->actrs.size(); i++) {
    auto rai = ((const RPActor*)(rpm->actrs[i]));
    for (unsigned int j = 0; j <numRefItem; j++) { 
      double vij =  rai->riVals[j];
      printf(" %6.2f ", vij);
    }
    cout << endl << flush;
    }
    
    cout << "Computing positions ... " << endl;
    vector<VUI> positions; // list of all positions
    VUI pstn;
    // build the first permutation: 1,2,3,...
    for (unsigned int i = 0; i < numRefItem; i++) {
      pstn.push_back(i);
    }
    positions.push_back(pstn);
    while (next_permutation(pstn.begin(), pstn.end())) {
      positions.push_back(pstn);
    }
    const unsigned int numPos = positions.size();
    cout << "For " << numRefItem << " reform items there are " << numPos << " positions" << endl;
    
   
    // -------------------------------------------------
    // The next section sets up actor utilities.
    // First, we compute the unnormalized, raw utilities. The 'utilActorPos' checks
    // to see if pvMin/pvMax have been set, and returns the raw scores if not.
    // Then we scan across rows to find that actor's pvMin/pvMax, and record that
    // so utilActorPos can use it in the future. Finally, we normalize the rows and
    // display the normalized utility matrix.
    cout << "Computing utilities of positions ... " << endl;
    auto ruFn = [positions, rpm](unsigned int ai, unsigned int pj) {
      auto pstn = positions[pj];
      double uip = rpm->utilActorPos(ai, pstn);
      return uip;
    };
    auto rawUij = KMatrix::map(ruFn, numA, numPos); // rows are actors, columns are all possible positions
    for (unsigned int i = 0; i < numA; i++) {
      double pvMin = rawUij(i, 0);
      double pvMax = rawUij(i, 0);
      for (unsigned int j = 0; j < numPos; j++) {
        double rij = rawUij(i, j);
        if (rij < pvMin) {
          pvMin = rij;
        }
        if (rij > pvMax) {
          pvMax = rij;
        }
      }
      assert(0 <= pvMin);
      assert(pvMin < pvMax);
      auto ai = ((RPActor*)(rpm->actrs[i]));
      ai->posValMin = pvMin;
      ai->posValMax = pvMax;
    }
    KMatrix uij = KBase::rescaleRows(rawUij, 0.0, 1.0); // von Neumann utility scale

    
    cout << "Complete (normalized) utility matrix of all possible positions (rows) versus actors (columns)" << endl << flush;
    for (unsigned int pj = 0; pj < numPos; pj++) {
      printf("%3u  ", pj);
      auto pstn = positions[pj];
      printVUI(pstn);
      printf("  ");
      for (unsigned int ai = 0; ai < numA; ai++) {
        double uap = uij(ai, pj);
        printf("%6.4f, ", uap);
      }
      cout << endl << flush;
    }

    // -------------------------------------------------
    // The next section determines the most self-interested positions for each actor,
    // as well as the 'central position' over all possible reform priorities
    // (which 'office seeking politicans' would adopt IF proportional voting).
    cout << endl << "Computing best position for each actor" << endl;
    vector<VUI> bestAP; // list of each actor's best position (followed by CP)
    for (unsigned int ai = 0; ai < numA; ai++) {
      unsigned int bestJ = 0;
      double bestV = 0;
      for (unsigned int pj = 0; pj < numPos; pj++) {
        if (bestV < uij(ai, pj)) {
          bestJ = pj;
          bestV = uij(ai, pj);
        }
      }
      printf("Best for %02u is ", ai);
      printVUI(positions[bestJ]);
      cout << endl;
      bestAP.push_back(positions[bestJ]);
    }


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

    const unsigned int maxDisplayed = 720; // factorial(6)
    unsigned int  numPr = (pairs.size() < maxDisplayed) ? pairs.size() : maxDisplayed;

    cout << "Displaying highest " << numPr << endl << flush;
    for (unsigned int i = 0; i < numPr; i++) {
      auto pri = pairs[i];
      unsigned int ni = get<0>(pri);
      double zi = get<1>(pri);
      VUI pi = get<2>(pri);

      printf(" %3u: %4u  %.2f  ", i, ni, zi);
      printVUI(pi);
      cout << endl << flush;
    }

    VUI bestPerm = get<2>(pairs[0]);

    bestAP.push_back(bestPerm);
    return bestAP;
  }

} // end of namespace

int main(int ac, char **av) {
  using KBase::ReportingLevel;
  using KBase::PRNG;
  using KBase::MtchPstn;
  using KBase::dSeed;
  using RfrmPri::RPModel;
  using RfrmPri::RPState;
  using RfrmPri::RPActor;
  using RfrmPri::printPerm;
  
  auto sTime = KBase::displayProgramStart(RfrmPri::appName, RfrmPri::appVersion); 
  uint64_t seed = dSeed; // arbitrary;
  bool siP = true;
  bool cpP = false;
  bool runP = true;
  unsigned int sNum = 1;
  bool xmlP = false;
  string inputXML = ""; 

  auto showHelp = [ sNum]() {
    printf("\n");
    printf("Usage: specify one or more of these options\n");
    printf("\n");
    printf("--cp              start all actors from the central position \n");
    printf("--si              start each actor from their most self-interested position \n");
    printf("                  If neither si nor cp are specified, it will use si. \n");
    printf("                  If both si and cp are specified, it will use second specified. \n");
    printf("--help            print this message and exit \n");
    printf("--seed <n>        set a 64bit seed \n");
    printf("                  0 means truly random \n");
    printf("                  default: %020llu \n", dSeed);
    printf("--sNum <n>        choose a scenario number \n");
    printf("                  default: %u \n", sNum);
    printf("--xml <f>         read XML scenario from a given file \n");
    printf("\n");
    printf("For example, rpdemo --si  --xml rpdata.xml, would read the file rpdata.xml \n");
    printf("and start all actors from self-interested positions.\n");
    printf("\n");
    printf("If both scenario number and XML file are specified, it will use only the XML.\n");
    printf("\n");
    printf("If neither scenario number nor XML file are specified, \n");
    printf("it will run a hard-coded example, as if --sNum 1 had been specified.\n");
    printf("\n");
  };

  // a list of <keyword, description, λ-fn>
  // might be enough to do this - except for the arguments to options.
  if (ac > 1) {
    for (int i = 1; i < ac; i++) {
      if (strcmp(av[i], "--seed") == 0) {
        i++;
        seed = std::stoull(av[i]);
      }
      else if (strcmp(av[i], "--sNum") == 0) {
        i++;
        sNum = atoi(av[i]);
      }
      else if (strcmp(av[i], "--xml") == 0) {
        xmlP = true;
        i++;
        inputXML = av[i];
      }
      else if (strcmp(av[i], "--si") == 0) {
        cpP = false;
        siP = true;
      }
      else if (strcmp(av[i], "--cp") == 0) {
        siP = false;
        cpP = true;
      }
      else if (strcmp(av[i], "--help") == 0) {
        runP = false;
      }
      else {
        runP = false;
        printf("Unrecognized argument: %s\n", av[i]);
      }
    }
  } 

  if (!runP) {
    showHelp();
    return 0;
  }


  if (0 == seed) {
    PRNG * rng = new PRNG();
    seed = rng->setSeed(0); // 0 == get a random number
    delete rng;
    rng = nullptr;
  }
  printf("Using PRNG seed:  %020llu \n", seed);
  printf("Same seed in hex:   0x%016llX \n", seed);
  // Unix correctly prints all digits with lu, lX, llu, and llX.
  // Windows only prints part, with lu, lX, llu, and llX.


  auto rpm = new RPModel("", seed);
  if (xmlP) {
    rpm->readXML(inputXML);
    cout << "done reading XML" << endl << flush;
  }
  else {
    switch (sNum) {
    case 0:
      rpm->initScen(sNum);
      break;
    case 1:
      rpm->initScen(sNum);
      break;

    case 2:
    case 20:
    case 21:
    case 22:
    case 23:
      rpm->initScen(sNum);
      break;

    case 3:
    case 30:
    case 31:
    case 32:
    case 33:
      rpm->initScen(sNum);
      break;

    default:
      cout << "Unrecognized scenario number " << sNum << endl << flush;
      assert(false);
      break;
    }
  }

  unsigned int numA = rpm->numAct; // the actors are as yet unnamed
  unsigned int numR = rpm->numItm;
  //unsigned int numC = rpm->numCat;

  auto rps0 = new RPState(rpm);
  rpm->addState(rps0);

  auto pstns = RfrmPri::scanPositions(rpm);
  KBase::VUI bestPerm = pstns[numA];
  assert(numR == bestPerm.size());

  // Either start them all at the CP or have each to choose an initial position which
  // maximizes their direct utility, regardless of expected utility.
  for (unsigned int i = 0; i < numA; i++) {
    auto pi = new  MtchPstn();
    pi->numCat = numR;
    pi->numItm = numR;
    if (cpP) {
      pi->match = bestPerm;
    }
    if (siP) {
      pi->match = pstns[i];
    }
    rps0->addPstn(pi);
  }
  assert(numA == rps0->pstns.size());

  rps0->step = [rps0]() {
    return rps0->stepSUSN();
  };
  unsigned int maxIter = 100;
  rpm->stop = [maxIter, rpm](unsigned int iter, const KBase::State * s) {
    bool doneP = iter > maxIter;
    if (doneP) {
      printf("Max iteration limit of %u exceeded \n", maxIter);
    }
    auto s2 = ((const RPState *)(rpm->history[iter]));
    for (unsigned int i = 0; i < iter; i++) {
      auto s1 = ((const RPState *)(rpm->history[i]));
      if (RPModel::equivStates(s1, s2)) {
        doneP = true;
        printf("State number %u matched state number %u \n", iter, i);
      }
    }

    return doneP;
  };

  rps0->setUENdx();
  rpm->run();

  // we already displayed each state as it was processed,
  // so there is no need to show it again
  //rpm->showHist();


  delete rpm; // and actors, and states
  rpm = nullptr;
  rps0 = nullptr;

  KBase::displayProgramEnd(sTime);

  return 0;
}


// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
