// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
// ********************************************************************
// * MRCP (Mesh-type Reference Computational Phantom)                 *
// * Geant4 code for the simulation of MRCP.                          *
// *                                                                  *
// * Author: Evan Kim (evandde@gmail.com)                             *
// * This code was tested with Geant4 10.05.                          *
// * Nov. 25, 2019.                                                   *
// ********************************************************************
//

#include "DetectorConstruction.hh"
#include "PhysicsList.hh"
#include "ActionInitialization.hh"

// G4Runmanager and mandatory classes
#ifdef G4MULTITHREADED
#include "G4MTRunManager.hh"
#include "G4Threading.hh"
#else
#include "G4RunManager.hh"
#endif

// UI and visualization classes
#include "G4UImanager.hh"
#include "G4UIterminal.hh"
#ifdef G4UI_USE
#include "G4UIExecutive.hh"
#endif
#ifdef G4VIS_USE
#include "G4VisExecutive.hh"
#endif

// Randomize class to set seed number
#include "Randomize.hh"

// --- main() Arguments usage explanation --- //
namespace
{
void PrintUsage()
{
    G4cerr << " Usage: " << G4endl
        << " ProjectName [-option1 value1] [-option2 value2] ..." << G4endl;
    G4cerr << "\t--- Option lists ---"
        << "\n\t[-m] <Set macrofile> default: ""init_vis.mac"", inputtype: string"
        << "\n\t[-o] <Set outfile> default: ""[MACRO].out"", inputtype: string"
        << "\n\t[-p] <Set tetra model file & path> "
        << "\n\t\tdefault: ""$PHANTOM or ../phantoms/ICRP-AM"", inputtype: string"
        << "\n\t[-t] <Set nThreads> default: 1, inputtype: int"
        << "\n\t[-u] <Set UISession> default: tcsh, inputtype: string"
        << G4endl;
    G4cerr << "\tNote: -t option is available only for multi-threaded mode. Max: "
        << G4Threading::G4GetNumberOfCores() << G4endl;
}
}

// --- Global variables for main() arguments --- //
G4String OUTPUT_FILENAME; // Passed to RunAction class.

int main(int argc, char** argv)
{
    // --- Default setting for main() arguments ---//
    G4String macro_FileName = "";
    ::OUTPUT_FILENAME = "";
    G4String mainPhantom_FilePath = "../phantoms/ICRP-AM";
    const char* envVar_PHANTOM = ::getenv("PHANTOM") ;
    if(envVar_PHANTOM != nullptr) // Use if $PHANTOM environment variable exist
        mainPhantom_FilePath = G4String(envVar_PHANTOM);
#ifdef G4MULTITHREADED
    G4int nThreads = 1;
#endif
    G4String session = "tcsh";

    // --- Parsing main() Arg   uments --- //
    for(G4int i = 1; i<argc; i += 2)
    {
        if(G4String(argv[i])=="-m") macro_FileName = argv[i+1];
        else if(G4String(argv[i])=="-o") ::OUTPUT_FILENAME = argv[i+1];
        else if(G4String(argv[i])=="-p") mainPhantom_FilePath = argv[i+1];
#ifdef G4MULTITHREADED
        else if(G4String(argv[i])=="-t") nThreads = G4UIcommand::ConvertToInt(argv[i+1]);
#endif
        else if(G4String(argv[i])=="-u") session = argv[i+1];
        else
        {
            PrintUsage();
            return 1;
        }
    }
    if (argc>11) // print usage when there are too many arguments
    {
        PrintUsage();
        return 1;
    }
    if(macro_FileName.size() && !(::OUTPUT_FILENAME.size()))
    {
        size_t fileNameStartPos = macro_FileName.rfind('/');
        size_t extensionStartPos = macro_FileName.rfind('.');

        ::OUTPUT_FILENAME = "output/" + macro_FileName.substr(fileNameStartPos).substr(0, extensionStartPos - fileNameStartPos) + ".out";
    }

    // --- Choose the Random engine --- //
    G4Random::setTheEngine(new CLHEP::RanecuEngine);
    G4Random::setTheSeed(time(nullptr));

    // --- Construct runmanager & Set UserInit --- //
#ifdef G4MULTITHREADED
    auto runManager = new G4MTRunManager;
    runManager->SetNumberOfThreads(nThreads);
#else
    auto runManager = new G4RunManager;
#endif
    G4VUserDetectorConstruction* mainDC = new DetectorConstruction(mainPhantom_FilePath);
    runManager->SetUserInitialization(mainDC);
    G4VModularPhysicsList* mainPhys = new PhysicsList;
    runManager->SetUserInitialization(mainPhys);
    runManager->SetUserInitialization(new ActionInitialization);
    // runManager->Initialize() // commented. It would be in the macro file.

    // --- Batch mode or Interactive mode setting --- //
    auto uiManager = G4UImanager::GetUIpointer();
    if(macro_FileName.size()) // macro was provided
    {
        G4String command = "/control/execute ";
        uiManager->ApplyCommand(command + macro_FileName);
    }
    else // interactive mode
    {
#ifdef G4UI_USE
        auto ui = new G4UIExecutive(argc, argv, session);
#else
        G4cerr << "ERROR: Interactive mode is not available. Please provide macro file." << G4endl;
        return 1;
#endif

#ifdef G4VIS_USE
        G4VisManager* visManager = new G4VisExecutive;
        visManager->Initialize();

        uiManager->ApplyCommand("/control/execute init_vis.mac");
#endif

#ifdef G4UI_USE
        ui->SessionStart();
#endif

#ifdef G4VIS_USE
        delete visManager;
#endif

#ifdef G4UI_USE
        delete ui;
#endif
    }

    // --- Job termination --- //
    delete runManager;

    return 0;
}
