#include "RunAction.hh"
#include "MRCPRun.hh"
#include "Primary_ParticleGun.hh"

extern G4String OUTPUT_FILENAME; // From main() argument (-o)

G4String RunAction::fPrimaryInfo;

RunAction::RunAction()
: G4UserRunAction(), fRunTimer(nullptr)
{
    if(!IsMaster()) return;

    fInitTimer = new G4Timer;
    fInitTimer->Start();

    ofs.open(::OUTPUT_FILENAME.c_str());
}

RunAction::~RunAction()
{
    if(!IsMaster()) return;

    if(fInitTimer) delete fInitTimer;
    if(fRunTimer) delete fRunTimer;

    ofs.close();
}

G4Run* RunAction::GenerateRun()
{
    return new MRCPRun;
//    return new G4Run;
}

void RunAction::BeginOfRunAction(const G4Run* aRun)
{
    // --- Set print progress as 10% of total events --- //
    G4RunManager::GetRunManager()->SetPrintProgress(static_cast<G4int>(aRun->GetNumberOfEventToBeProcessed() * 0.1));

    if(!IsMaster()) return;

    // --- Initialization ends --- //
    fInitTimer->Stop();

    // --- Run starts --- //
    fRunTimer = new G4Timer;
    fRunTimer->Start();
}

void RunAction::EndOfRunAction(const G4Run* aRun)
{
    G4int nEvents = aRun->GetNumberOfEvent();
    if(nEvents==0) return;

    // Get source information if possible
    if(!fPrimaryInfo.size())
    {
        auto pga = dynamic_cast<const Primary_ParticleGun*>(G4RunManager::GetRunManager()->GetUserPrimaryGeneratorAction());
        fPrimaryInfo = "NULL";
        if(pga) fPrimaryInfo = pga->GetPrimaryInfo();
    }

    if(!IsMaster()) return;

    // --- Run ends --- //
    fRunTimer->Stop();

    // --- Print the results --- //
    auto theRun = dynamic_cast<const MRCPRun*>(aRun);
    if(theRun)
    {
        PrintResultsInRows(G4cout, theRun);
        PrintResultsInCols(ofs, theRun);
    }

    // --- Initialization starts for next run --- //
    fPrimaryInfo.clear();
    fInitTimer->Start();
}

void RunAction::PrintResultsInRows(std::ostream& out, const MRCPRun* theRun)
{
    G4int nEvents = theRun->GetNumberOfEvent();

    out << std::fixed;
    out << "======================================================================" << G4endl;
    out << " Run ID: " << theRun->GetRunID() << G4endl;
    out << " Initialization time (s): " << fInitTimer->GetRealElapsed() << G4endl;
    out << " Running time (s): " << fRunTimer->GetRealElapsed() << G4endl;
    out << " Number of threads: " << G4Threading::GetNumberOfRunningWorkerThreads() << G4endl;
    out << " Number of event processed: " << nEvents << G4endl;
    out << " Source: " << fPrimaryInfo << G4endl;
    out << "======================================================================" << G4endl;
    out << std::scientific;

    out << std::setw(35) << "Protection Quantity"
        << std::setw(20) << "Mean dose (Gy or Sv)"
        << std::setw(20) << "Relative error" << G4endl;

    for(const auto& protQ: theRun->GetProtectionQuantities())
    {
        G4double totalDose = std::get<1>(protQ);
        G4double totalSquareDose = std::get<2>(protQ);
        G4double meanDose = totalDose/nEvents;
        G4double stdevDose = sqrt( (totalSquareDose/nEvents) - (meanDose * meanDose) );
        G4double relativeError = (stdevDose/sqrt(nEvents)) / meanDose;

        out << std::setw(35) << std::get<0>(protQ)
            << std::setw(20) << meanDose/gray
            << std::setw(20) << relativeError << G4endl;
    }

    out << G4endl << G4endl;
}

void RunAction::PrintResultsInCols(std::ostream& out, const MRCPRun* theRun)
{
    G4int nEvents = theRun->GetNumberOfEvent();
    G4int runID = theRun->GetRunID();
    const auto& protQs = theRun->GetProtectionQuantities();

    // --- Header --- //
    if(runID==0)
    {
        out << std::fixed
            << "RunID" << "\t"
            << "InitT(s)" << "\t"
            << "RunT(s)" << "\t"
            << "NThreads" << "\t"
            << "NEvents" << "\t"
            << "Source" << "\t";

        out << std::scientific;
        for(const auto& protQ: protQs)
            out << std::get<0>(protQ) << "(Gy|Sv)" << "\t"
                << "RelativeError" << "\t";

        out << G4endl;
    }

    // --- Data --- //
    out.precision(3);
    out << std::fixed
        << runID << "\t"
        << fInitTimer->GetRealElapsed() << "\t"
        << fRunTimer->GetRealElapsed() << "\t"
        << G4Threading::GetNumberOfRunningWorkerThreads() << "\t"
        << nEvents << "\t"
        << fPrimaryInfo << "\t";

    out.precision(6);
    out << std::scientific;
    for(const auto& protQ: protQs)
    {
        G4double totalDose = std::get<1>(protQ);
        G4double totalSquareDose = std::get<2>(protQ);
        G4double meanDose = totalDose/nEvents;
        G4double stdevDose = sqrt( (totalSquareDose/nEvents) - (meanDose * meanDose) );
        G4double relativeError = (stdevDose/sqrt(nEvents)) / meanDose;

        out << meanDose/gray << "\t"
            << relativeError << "\t";
    }

    out << G4endl;
}
