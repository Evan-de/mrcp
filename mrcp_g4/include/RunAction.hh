#ifndef RunAction_hh_
#define RunAction_hh_

#include "G4UserRunAction.hh"
#include "G4Run.hh"
#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4Timer.hh"

#include <fstream>

class MRCPRun;
class Primary_ParticleGun;

class RunAction: public G4UserRunAction
{
public:
    RunAction();
    virtual ~RunAction();

    virtual G4Run* GenerateRun();
    virtual void BeginOfRunAction(const G4Run*);
    virtual void EndOfRunAction(const G4Run*);

private:
    void PrintResultsInRows(std::ostream& out, const MRCPRun* theRun);
    void PrintResultsInCols(std::ostream& out, const MRCPRun* theRun);

    G4Timer* fInitTimer;
    G4Timer* fRunTimer;

    static G4String fPrimaryInfo;

    std::ofstream ofs;
};

#endif
