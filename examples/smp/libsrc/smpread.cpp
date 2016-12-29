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
KMatrix SMPModel::accM;

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

    cout << "Setting ideal-accomodation matrix to identity matrix" << endl;
    accM = KBase::iMat(numActor);

    // now that it is read and verified, use the data
    auto sm0 = initModel(actorNames, actorDescs, dNames, cap, pos, sal, accM,  s, f, scenDesc, scenName);
    return sm0;
}
// end of readCSVStream

SMPModel * SMPModel::xmlRead(string fName, vector<bool> f) {
    using KBase::enumFromName;
    cout << "Start SMPModel::readXML of " << fName << endl;

    auto getFirstChild = [](XMLElement* prntEl, const char * name) {
        XMLElement* childEl = prntEl->FirstChildElement(name);
        return childEl;
    };

    unsigned int numAct = 0;
    unsigned int numDim = 0;
    uint64_t seed = KBase::dSeed;
    XMLDocument d1;

    // declare variables out here, so as to be in-scope at 'return'
    SMPModel* smp = nullptr;  
    vector<string> actorNames = {};
    vector<string> actorDescs = {};
    vector<string> dNames = {};
    KMatrix capM, posM, salM; // all size 0-by-0

	const char * sName = new char[512];
	//strcpy(sName, "");
	const char* sn2 = "";

    try {
        d1.LoadFile(fName.c_str());
        auto eid = d1.ErrorID();
        if (0 != eid) {
            cout << "tinyxml2 ErrorID: " << eid << endl;
            throw KException(d1.GetErrorStr1());
        }
        // missing data causes the missing XMLElement* to come back as nullptr
        XMLElement* scenEl = d1.FirstChildElement("Scenario");
        assert(nullptr != scenEl);
        auto scenNameEl = getFirstChild(scenEl, "name");
		try {
            sName = scenNameEl->GetText();
			if (sName == NULL)
			{
				sName = new char[512];
				strcpy((char *)sName, "");
			}
            printf("Name of scenario: %s\n", sName);
        }
        catch (...) {
            throw (KException("Error reading file header"));
        }
        auto scenDescEl = getFirstChild(scenEl, "desc");
		sn2 = scenDescEl->GetText();
		if (sn2 == NULL)
		{
			sn2 = new char[512];
			strcpy((char *)sn2, "");
		}
        assert(nullptr != sn2);
        auto seedEl = getFirstChild(scenEl, "prngSeed");
        const char* sd2 = seedEl->GetText();
        assert(nullptr != sd2);
        seed = std::stoull(sd2);
        printf("Read PRNG seed:  %020llu \n", seed);

        smp = new SMPModel(sn2, seed);
        SMPState* sms = new SMPState(smp);
        smp->addState(sms);

        auto modelParamsEl = getFirstChild(scenEl, "ModelParameters");
        if (nullptr == modelParamsEl) {
            cout << "No model parameters in XML scenario. Using defaults:" << endl << flush;
            cout << smp->vpm <<endl;
            cout << smp->vrCltn <<endl;
            cout << smp->pcem  <<endl;
            cout << smp->stm  <<endl;
            cout << smp->bigRRng  <<endl;
            cout << smp->bigRAdj  <<endl;
            cout << smp->tpCommit  <<endl;
            cout << smp->ivBrgn  <<endl;
            cout << smp->brgnMod  <<endl;
            cout << flush;
        }
        else {
            function <string (const char*)> showChild = [getFirstChild, modelParamsEl](const char* name ) {
                auto el = getFirstChild(modelParamsEl, name);
                if (nullptr == el) {
                    throw (KException("SMPModel::xmlRead - failed to find required XML element"));
                }
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
        }

        // read all the dimensions
        try {
            numDim = 0;
            XMLElement* dimsEl = scenEl->FirstChildElement("Dimensions");
            assert(nullptr != dimsEl);
            XMLElement* dEl = dimsEl->FirstChildElement("dName");
            assert(nullptr != dEl); // has to be at least one}
            while (nullptr != dEl) {
                const char* dn = dEl->GetText();
                dNames.push_back(string(dn));
                cout << "Adding dimension "<<dn<<endl;
                smp->addDim(dn);
                // move to the next, if any
                numDim++;
                dEl = dEl->NextSiblingElement("dName");
            }
            printf("Found %u dimensions \n", numDim);
            assert(smp->numDim == numDim);
        }
        catch (...)
        {
            throw (KException("SMPModel::readXML: Error reading Dimensions data"));
        }
        
        // Count the actors
        try {
          numAct = 0;
          XMLElement* actorsEl = scenEl->FirstChildElement("Actors");
          assert(nullptr != actorsEl);
          XMLElement* aEl = actorsEl->FirstChildElement("Actor");
          assert(nullptr != aEl); // has to be at least one
          while (nullptr != aEl) {
            // move to the next, if any
            numAct++;
            aEl = aEl->NextSiblingElement("Actor"); 
          }
        }
        catch (...)
        {
          throw (KException("SMPModel::readXML: Error reading Actors data"));
        }
        printf("Found %u actors \n", numAct);
        cout << endl;
        
 
        capM = KMatrix(numAct, 1);
        posM = KMatrix(numAct, numDim);
        salM = KMatrix(numAct, numDim);

        // Read all the actors
        try {
            XMLElement* actorsEl = scenEl->FirstChildElement("Actors");
            assert(nullptr != actorsEl);
            XMLElement* aEl = actorsEl->FirstChildElement("Actor");
            assert(nullptr != aEl); // has to be at least one
            unsigned int actCntr = 0;
            while (nullptr != aEl) { 
                const char* aName = aEl->FirstChildElement("name")->GetText();
                const char* aDesc = aEl->FirstChildElement("description")->GetText();

                actorNames.push_back(string(aName));
                actorDescs.push_back(string(aDesc));

                double cap = -10.0; // another impossible value
                aEl->FirstChildElement("capability")->QueryDoubleText(&cap);
                assert(0.0 < cap); // could be quite large
                capM(actCntr, 0) = cap;
                auto  posEl = aEl->FirstChildElement("Position");
                auto vPos = KMatrix(numDim, 1);
                unsigned int dimCntr = 0; // count and verify dimensions
                auto pdEl = posEl->FirstChildElement("dCoord");
                while (nullptr != pdEl) {
                  double pd = 0.0;
                  pdEl->QueryDoubleText(&pd);
                  printf("Read dimension %u: %.2f \n", dimCntr, pd);
                  vPos(dimCntr, 0) = pd;
                  pdEl = pdEl->NextSiblingElement("dCoord");
                  dimCntr++; // got one
                }
                printf("Read %u dimensional components \n", dimCntr);
                assert(numDim == dimCntr);

                auto vpp = new VctrPstn(vPos);
                sms->addPstn(vpp);
                

                auto  salEl = aEl->FirstChildElement("Salience");
                auto vSal = KMatrix(numDim, 1);
                dimCntr = 0;
                auto vsEl = salEl->FirstChildElement("dSal");
                while (nullptr != vsEl) {
                  double vs = 0.0;
                  vsEl->QueryDoubleText(&vs);
                  printf("Read salience %u: %.2f \n", dimCntr, vs);
                  vSal(dimCntr, 0) = vs;
                  vsEl = vsEl->NextSiblingElement("dSal");
                  dimCntr++; // got one
                }
                printf("Read %u salience components \n", dimCntr);
                assert(numDim == dimCntr);

                auto ri = new SMPActor(aName, aDesc);
                ri->sCap = cap;
                ri->vr = smp->vrCltn;
                ri->vSal = vSal;
                smp->addActor(ri);
                capM(actCntr,0)=cap;
                for (unsigned int i=0; i<numDim; i++){
                    salM(actCntr, i)=vSal(i,0);
                    posM(actCntr, i) = vPos(i,0);
                }
                // move to the next, if any
                actCntr++;
                aEl = aEl->NextSiblingElement("Actor");
                cout << endl << flush;
            }
        }
        catch (...)
        {
            throw (KException("SMPModel::readXML: Error reading Actors data"));
        }
        
        // Read the accomodation matrix
        cout << "Setting ideal-accomodation matrix to identity matrix (initially, needs reset)" << endl;
        accM = KBase::iMat(numAct);
        try {
            XMLElement* iadEl = scenEl->FirstChildElement("IdealAdjustment");
            if (nullptr != iadEl) {
                cout << "Reading IdealAdjustment matrix"<<endl;
                auto nameNdx = [numAct, actorNames] (const string n) {
                    int ndx = -1;
                    for (unsigned int i=0; i<numAct; i++){
                        if (actorNames[i] == n){
                            ndx = i;
                        }
                    }
                    assert (0 <= ndx);
                    return ndx;
                };
                unsigned int numIAPairs = 0;
                XMLElement* iaEl = iadEl->FirstChildElement("iaPair");
                while (nullptr != iaEl){
                    auto aiStr = string(iaEl->FirstChildElement("adjustingIdeal")->GetText());
                    auto aiNdx = nameNdx(aiStr);
                    auto rpStr = string(iaEl->FirstChildElement("referencePos")->GetText());
                    auto rpNdx = nameNdx(rpStr);
                    auto adjEl = iaEl->FirstChildElement("adjust");
                    double adjV = 0.0;
                    adjEl->QueryDoubleText(&adjV);
                    printf(" %6s (%u) :  %6s  (%u) = %.3f \n", aiStr.c_str(), aiNdx, rpStr.c_str(), rpNdx, adjV);
                    accM(aiNdx, rpNdx)=adjV;
                    
                    numIAPairs++;
                    iaEl = iaEl->NextSiblingElement("iaPair");
                }
                cout << "Found "<<numIAPairs << " iaPair"<<endl<<flush;
            }
        }
        catch (...)
        {
            throw (KException("SMPModel::readXML: Error reading accomodation data"));
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

    posM = posM / 100.0;
    salM = salM / 100.0;
    cout << "End SMPModel::readXML of " << fName << endl;
    // now that it is read and verified, use the data  
    delete smp;
    smp = initModel(actorNames, actorDescs, dNames, capM, posM, salM, accM, seed, f, sn2, sName);
    assert (smp != nullptr);
    return smp;
}
// end of readXML


}; // end of namespace

// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
