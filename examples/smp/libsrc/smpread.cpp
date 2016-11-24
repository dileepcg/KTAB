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
// Demonstrate a very basic, but highly parameterizable, Spatial Model of Politics.
// --------------------------------------------
// TODO: consolidate error checking for read functions.
// These reader functions do a lot of very similar error checking,
// so duplicative code should be put in sub-functions.
// --------------------------------------------

#include <string>
#include <tinyxml2.h>
#include <vector>

#include "kmodel.h"
#include "smp.h"
#include "minicsv.h"


namespace SMPLib {
using std::cout;
using std::endl;
using std::flush;
using std::function;
using std::get;
using std::string;

using KBase::PRNG;
using KBase::KMatrix;
using KBase::KException;
using KBase::Actor;
using KBase::Model;
using KBase::Position;
using KBase::VctrPstn;
using KBase::BigRAdjust;
using KBase::BigRRange;
using KBase::VPModel;
using KBase::State;
using KBase::StateTransMode;
using KBase::VotingRule;
using KBase::PCEModel;
using KBase::ReportingLevel;

using tinyxml2::XMLElement;
using tinyxml2::XMLDocument;

// -------------------------------------------- 


// --------------------------------------------

SMPModel * SMPModel::csvRead(string fName, uint64_t s, vector<bool> f) {
  using KBase::KException;
  char * errBuff; // as sprintf requires

  csv::ifstream inStream(fName.c_str());
  inStream.set_delimiter(',', "$$");
  inStream.enable_trim_quote_on_str(true, '\"');

  const bool opened = inStream.is_open();
  assert (opened);

  string scenName = "";
  string scenDesc = "";
  unsigned int numActor = 0;
  unsigned int numDim = 0;
  inStream.read_line();
  inStream >> scenName >> scenDesc >> numActor >> numDim;


  cout << "Scenario name: |" << scenName << "|" << endl;
  cout << flush;
  assert(scenName.length() <= Model::maxScenNameLen);

  printf("Number of actors: %u \n", numActor);
  printf("Number of dimensions: %u \n", numDim);
  cout << endl << flush;

  if (numDim < 1) { // lower limit
    throw(KBase::KException("SMPModel::readCSVStream: Invalid number of dimensions"));
  }
  assert(0 < numDim);
  if ((numActor < minNumActor) || (maxNumActor < numActor)) { // avoid impossibly low or ridiculously large
    throw(KBase::KException("SMPModel::readCSVStream: Invalid number of actors"));
  }
  assert(minNumActor <= numActor);
  assert(numActor <= maxNumActor);


  // get the names of dimensions.
  // format (for 3 dimensions) is like this:
  // Actor,Description,Power,Pstn1,Sal1,Pstn2,Sal2,Pstn3,Sal3,
  inStream.read_line();
  string dummyField;
  inStream >> dummyField; // skip "Actor"
  inStream >> dummyField; // skip "Descripton"
  inStream >> dummyField; // skip "Power"
  auto dNames = vector<string>();
  for (unsigned int d = 0; d < numDim; d++) {
    string dimName, salName;
    inStream >> dimName;
    inStream >> salName;
    assert(dimName.length() <= maxDimDescLen);
    dNames.push_back(dimName);
    printf("Dimension %2u: %s \n", d, dNames[d].c_str());
  }
  cout << endl;

  // Read actor data
  auto actorNames = vector<string>();
  auto actorDescs = vector<string>();
  auto cap = KMatrix(numActor, 1);
  auto nra = KMatrix(numActor, 1);
  auto pos = KMatrix(numActor, numDim);
  auto sal = KMatrix(numActor, numDim);

  for (unsigned int i = 0; i < numActor; i++) {
    string aName = "";
    string aDesc = "";
    double aCap = 0.0;
    inStream.read_line();
    inStream >> aName >> aDesc >> aCap;

    // names must have at least 1 character
    assert(0 < aName.length());
    assert(aName.length() <= Model::maxActNameLen);
    actorNames.push_back(aName);
    printf("Actor %3u name: %s \n", i, actorNames[i].c_str());

    // empty descriptions are allowed
    assert(aDesc.length() <= Model::maxActDescLen);
    actorDescs.push_back(aDesc);
    printf("Actor %3u desc: %s \n", i, actorDescs[i].c_str());


    printf("Actor %3u power: %5.1f \n", i, aCap);
    assert(0 <= aCap); // zero weight is pointless, but not incorrect
    assert(aCap < 1E8); // no real upper limit, so this is just a sanity-check
    cap(i, 0) = aCap;


    // get position and salience for each dimension
    double salI = 0.0;
    for (unsigned int d = 0; d < numDim; d++) {
      double dPos = 0.0; // on [0, 100] scale
      double dSal = 0.0; // on [0, 100] scale
      inStream >> dPos >> dSal;

      printf("pos[%3u , %3u] =  %5.3f \n", i, d, dPos);
      cout << flush;
      if ((dPos < 0.0) || (+100.0 < dPos)) { // lower and upper limit
        errBuff = newChars(100);
        sprintf(errBuff, "SMPModel::readCSVStream: Out-of-bounds position for actor %u on dimension %u:  %f",
                i, d, dPos);
        throw(KException(errBuff));
      }
      assert(0.0 <= dPos);
      assert(dPos <= 100.0);
      pos(i, d) = dPos;

      if ((dSal < 0.0) || (+100.0 < dSal)) { // lower and upper limit
        errBuff = newChars(100);
        sprintf(errBuff, "SMPModel::readCSVStream: Out-of-bounds salience for actor %u on dimension %u:  %f",
                i, d, dSal);
        throw(KException(errBuff));
      }
      assert(0.0 <= dSal);
      salI = salI + dSal;
      printf("sal[%3u, %3u] = %5.3f \n", i, d, dSal);
      cout << flush;
      if (+100.0 < salI) { // upper limit: no more than 100% of attention to all issues
        errBuff = newChars(100);
        sprintf(errBuff,
                "SMPModel::readCSVStream: Out-of-bounds total salience for actor %u:  %f",
                i, salI);
        throw(KException(errBuff));
      }
      assert(salI <= 100.0);
      sal(i, d) = dSal;
    }
    cout << endl << flush;
  }

  cout << "Position matrix:" << endl;
  pos.mPrintf("%5.1f  ");
  cout << endl << endl << flush;
  cout << "Salience matrix:" << endl;
  sal.mPrintf("%5.1f  ");
  cout << endl << flush;

  // get them into the proper internal scale:
  pos = pos / 100.0;
  sal = sal / 100.0;

  // TODO: figure out representation of "accomodate" matrix
  cout << "Setting ideal-accomodation matrix to identity matrix" << endl;
  auto accM = KBase::iMat(numActor);

  // now that it is read and verified, use the data

  auto sm0 = SMPModel::initModel(actorNames, actorDescs, dNames, cap, pos, sal, accM,  s, f);
  return sm0;
}
// end of readCSVStream

SMPModel * SMPModel::xmlRead(string fName, vector<bool> f) {
  using KBase::enumFromName;
  cout << "Start SMPModel::readXML of " << fName << endl;
  SMPModel* smp = nullptr;

  auto getFirstChild = [](XMLElement* prntEl, const char * name) {
    XMLElement* childEl = prntEl->FirstChildElement(name);
    assert(nullptr != childEl);
    return childEl;
  };

  XMLDocument d1;
  try {
    d1.LoadFile(fName.c_str());
    auto eid = d1.ErrorID();
    if (0 != eid) {
      cout << "ErrorID: " << eid << endl;
      throw KException(d1.GetErrorStr1());
    }
    else {
      // missing data causes the missing XMLElement* to come back as nullptr
      XMLElement* scenEl = d1.FirstChildElement("Scenario");
      assert(nullptr != scenEl);
      auto scenNameEl = getFirstChild(scenEl, "name");
      try {
        const char * sName = scenNameEl->GetText();
        printf("Name of scenario: %s\n", sName);
      }
      catch (...) {
        throw (KException("Error reading file header"));
      }
      auto scenDescEl = getFirstChild(scenEl, "desc");
      const char* sn2 = scenDescEl->GetText();
      assert(nullptr != sn2);
      auto seedEl = getFirstChild(scenEl, "prngSeed");
      const char* sd2 = seedEl->GetText();
      assert(nullptr != sd2);
      uint64_t seed = std::stoull(sd2);
      printf("Read PRNG seed:  %020llu \n", seed);

      smp = new SMPModel(sn2, seed); // TODO: something besides default SQL flags

      auto modelParamsEl = getFirstChild(scenEl, "ModelParameters");

      function <string (const char*)> showChild = [getFirstChild, modelParamsEl](const char* name ) {
        auto el = getFirstChild(modelParamsEl, name);
        string s = el->GetText();
        cout << "  " << name << ":  " << s << endl << flush;
        return s;
      };

      cout << "Reading model parameters from XML scenario ..." << endl << flush;
      auto vpmScen = enumFromName<VPModel>(showChild("VictoryProbModel"), KBase::VPModelNames);
      auto vrScen = enumFromName<VotingRule>(showChild("VotingRule"), KBase::VotingRuleNames);
      auto pcemScen = enumFromName<PCEModel>(showChild("PCEModel"), KBase::PCEModelNames);
      auto stmScen = enumFromName<StateTransMode>(showChild("StateTransitions"), KBase::StateTransModeNames);
      auto bigRRangScen = enumFromName<BigRRange>(showChild("BigRRange"), KBase::BigRRangeNames);
      auto bigRAdjScen = enumFromName<BigRAdjust>(showChild("BigRAdjust"), KBase::BigRAdjustNames);
      auto tpcScen = enumFromName<ThirdPartyCommit>(showChild("ThirdPartyCommit"), KBase::ThirdPartyCommitNames);
      auto ivbScen = enumFromName<InterVecBrgn>(showChild("InterVecBrgn"), InterVecBrgnNames);
      auto bModScen = enumFromName<SMPBargnModel>(showChild("BargnModel"), SMPBargnModelNames);
      cout << "Setting SMPModel parameters from XML scenario ..." << endl << flush;
      smp->vpm = vpmScen;
      smp->vrCltn = vrScen;
      smp->pcem = pcemScen;
      smp->stm = stmScen;
      smp->bigRRng = bigRRangScen;
      smp->bigRAdj = bigRAdjScen;
      smp->tpCommit = tpcScen;
      smp->ivBrgn = ivbScen;
      smp->brgnMod = bModScen;

      // IF we have an SMPModel, try to read all the actors into it
      try {
        unsigned int na = 0;
        XMLElement* actorsEl = scenEl->FirstChildElement("Actors");
        assert(nullptr != actorsEl);
        XMLElement* aEl = actorsEl->FirstChildElement("Actor");
        assert(nullptr != aEl); // has to be at least one
        while (nullptr != aEl) {
          const char* aName = aEl->FirstChildElement("name")->GetText();
          const char* aDesc = aEl->FirstChildElement("description")->GetText();
          double cap = 0.0; // another impossible value
          aEl->FirstChildElement("capability")->QueryDoubleText(&cap);
          assert(0.0 < cap);

          auto ri = new SMPActor(aName, aDesc);
          ri->sCap = cap;
          ri->vr = vrScen;
          smp->addActor(ri);
          // move to the next, if any
          na++;
          aEl = aEl->NextSiblingElement("Actor");
        }
        printf("Found %u actors \n", na);
        assert(minNumActor <= na);
        assert(na <= maxNumActor);
      }
      catch (...)
      {
        throw (KException("SMPModel::readXML: Error reading Actors data"));
      }
    }
  }
  catch (const KException& ke)
  {
    cout << "Caught KException in SMPModel::readXML: " << ke.msg << endl << flush;
  }
  catch (...)
  {
    cout << "Caught unidentified exception in SMPModel::readXML" << endl << flush;
  }

  cout << "End SMPModel::readXML of " << fName << endl;
  assert (smp != nullptr);
  return smp;
}
// end of readXML


}; // end of namespace

// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
