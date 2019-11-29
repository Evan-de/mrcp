#include "MRCPModel.hh"

MRCPModel::MRCPModel(G4String name,
    const G4String& nodeFilePath, const G4String& eleFilePath,
    const G4String& materialFilePath, const G4String& RBMnBSFilePath,
    const G4String& colourFilePath)
: TETModel(name, nodeFilePath, eleFilePath, colourFilePath), fWholeMass(0.)
{
    ImportMaterialData(materialFilePath);
    ImportRBMnBSMassRatioData(RBMnBSFilePath);

    // Mass calculation
    for(const auto& subModelID: GetSubModelIDSet())
    {
        subModelMass_Map[subModelID] =
            GetSubModelVolume(subModelID) * GetSubModelMaterial(subModelID)->GetDensity();
        fWholeMass += subModelMass_Map.at(subModelID);
    }
}

MRCPModel::~MRCPModel()
{}

G4double MRCPModel::GetSubModelMass(G4int subModelID) const
{
    if(subModelMass_Map.find(subModelID) == subModelMass_Map.end())
        return 0.;
    else
        return subModelMass_Map.at(subModelID);
}

G4Material* MRCPModel::GetSubModelMaterial(G4int subModelID) const
{
    if(subModelMaterial_Map.find(subModelID) == subModelMaterial_Map.end())
        return G4NistManager::Instance()->FindOrBuildMaterial("G4_WATER");
    else
        return subModelMaterial_Map.at(subModelID);
}

G4double MRCPModel::GetSubModelRBMMassRatio(G4int subModelID) const
{
    if(subModelRBMMassRatio_Map.find(subModelID) == subModelRBMMassRatio_Map.end())
        return 0.;
    else
        return subModelRBMMassRatio_Map.at(subModelID);
}

G4double MRCPModel::GetSubModelBSMassRatio(G4int subModelID) const
{
    if(subModelBSMassRatio_Map.find(subModelID) == subModelBSMassRatio_Map.end())
        return 0.;
    else
        return subModelBSMassRatio_Map.at(subModelID);
}

void MRCPModel::ImportMaterialData(const G4String& materialFilePath)
{
    // --- Open material file --- //
    std::ifstream ifs(materialFilePath.c_str());
    if(!ifs.is_open()) // Fail to open
    {
        G4Exception("TETModelImport::ImportMaterialData()", "", FatalErrorInArgument,
            G4String("      There is no file '" + materialFilePath + "'").c_str());
        return;
    }

    G4cout << "  Opening material file '" << materialFilePath << "'" <<G4endl;

    // --- Get data --- //
    while(!ifs.eof())
    {
        G4String dummyString;
        G4String subModelName;
        G4double density;
        char dummyChar;
        G4int subModelID;
        std::map<G4int, G4double> zaid_fraction_Map;

        // Read header line
        ifs >> dummyString >> subModelName >> density >> dummyString // C @@ ## g/cm3
            >> dummyChar >> subModelID; // m$$
        density *= g/cm3;

        // Read ZAID & Fraction
        while(true)
        {
            // Get each line
            G4String thisLine;
            std::getline(ifs, thisLine);

            // Check if ZAID ends ( find the character 'C' )
            if(thisLine.c_str()[0] == 'C') break;

            // Split and store zaid & fraction value
            std::stringstream ss(thisLine);
            int zaid;
            double fraction;
            ss >> zaid >> fraction;
            zaid_fraction_Map[zaid/1000] = -fraction;
        }

        // Build material
        G4Material* theMaterial =
            new G4Material(
                subModelName, density, static_cast<G4int>(zaid_fraction_Map.size()),
                kStateSolid, NTP_Temperature, STP_Pressure
                );
        G4NistManager* nistManager = G4NistManager::Instance();
        for(const auto& zaid_fraction: zaid_fraction_Map)
            theMaterial->AddElement(
                nistManager->FindOrBuildElement(zaid_fraction.first),
                zaid_fraction.second
                );

        subModelMaterial_Map[subModelID] = theMaterial;
    }

    // --- Close the file --- //
    ifs.close();
}

void MRCPModel::ImportRBMnBSMassRatioData(const G4String& RBMnBSFilePath)
{
    // --- Open bone mass ratio file (.RBMnBS) --- //
    std::ifstream ifs(RBMnBSFilePath.c_str());
    if(!ifs.is_open()) // Fail to open
    {
        G4Exception("TETModel::ImportBoneMassRatioData()", "", FatalErrorInArgument,
            G4String("      There is no file '" + RBMnBSFilePath + "'").c_str());
        return;
    }

    G4cout << "  Opening bone mass ratio data file '" << RBMnBSFilePath << "'" <<G4endl;

    // --- Get data --- //
    G4int subModelID;
    G4double RBMMassRatio, BSMassRatio;

    // Read data lines
    while(ifs >> subModelID >> RBMMassRatio >> BSMassRatio)
    {
        subModelRBMMassRatio_Map[subModelID] = RBMMassRatio;
        subModelBSMassRatio_Map[subModelID] = BSMassRatio;
    }

    // --- Close the file --- //
    ifs.close();
}

void MRCPModel::Print() const
{
    // --- Print the overall information for each subModel --- //
    G4cout << G4endl
           << std::setw(9)  << "SubModel ID"
           << std::setw(11) << "# of Tets"
           << std::setw(11) << "vol [cm3]"
           << std::setw(11) << "mass [g]"
           << std::setw(11) << "d [g/cm3]"
           << "\t" << "Submodel" << G4endl;
    G4cout << "--------------------------------------------------------------------------------" << G4endl;

    G4cout << std::setiosflags(std::ios::fixed);
    G4cout.precision(3);

    for(auto subModelID: GetSubModelIDSet())
    {
        G4cout << std::setw(9)  << subModelID
               << std::setw(11) << GetSubModelNumTet(subModelID)
               << std::setw(11) << GetSubModelVolume(subModelID)/cm3
               << std::setw(11) << GetSubModelMass(subModelID)/g
               << std::setw(11) << GetSubModelMaterial(subModelID)->GetDensity()/(g/cm3)
               << "\t" << GetSubModelMaterial(subModelID)->GetName() << G4endl;
    }

    // --- Print information for whole Phantom --- //
    G4cout << G4endl;
    G4cout << "   Phantom name              " << GetName() << G4endl;
    G4cout << "   PhantomBox size           "
           << GetBoundingBoxSize().x()/cm << " * "
           << GetBoundingBoxSize().y()/cm << " * "
           << GetBoundingBoxSize().z()/cm << " cm3" << G4endl;
    G4cout << "   PhantomBox Min. position  " << GetBoundingBoxMin()/cm << " cm" << G4endl;
    G4cout << "   PhantomBox Max. position  " << GetBoundingBoxMax()/cm << " cm" << G4endl;
    G4cout << "   PhantomBox Cen. position  " << GetBoundingBoxCen()/cm << " cm" << G4endl;
    G4cout << "   Phantom Volume            " << GetTotalVolume()/cm3 << " cm3" << G4endl;
    G4cout << "   Phantom Mass              " << GetTotalMass()/g << " g" << G4endl;
    G4cout << "   Number of tetrahedrons    " << GetNumTets() << G4endl << G4endl;
}
