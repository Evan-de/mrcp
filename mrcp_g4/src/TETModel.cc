#include "TETModel.hh"
#include "TETModelStore.hh"

TETModel::TETModel(G4String name,
    const G4String& nodeFilePath, const G4String& eleFilePath,
    const G4String& materialFilePath, const G4String& colourFilePath,
    const G4String& boneMassRatioFilePath, const G4String& bondDRFFilePath)
: fTETModelName(name)
{
    G4cout << "================================================================================"<<G4endl;
    G4cout << "\t" << fTETModelName << " was implemented in this CODE!!   "<< G4endl;
    G4cout << "================================================================================"<<G4endl;

    ImportNodeData(nodeFilePath);
    ImportEleData(eleFilePath);
    ImportMaterialData(materialFilePath);
    ImportColourData(colourFilePath);
    ImportBoneMassRatioData(boneMassRatioFilePath);
    ImportBoneDRFData(bondDRFFilePath);

    CalculateModelDetails();

    PrintTETModelInformation();

    TETModelStore::GetInstance()->Register(this);
}

// --- Get* functions --- //
G4double TETModel::GetSubModelVolume(G4int subModelID) const
{
    if(subModelVolume_Map.find(subModelID) == subModelVolume_Map.end())
        return 0.;
    else
        return subModelVolume_Map.at(subModelID);
}
G4double TETModel::GetSubModelMass(G4int subModelID) const
{
    if(subModelMass_Map.find(subModelID) == subModelMass_Map.end())
        return 0.;
    else
        return subModelMass_Map.at(subModelID);
}
G4Material* TETModel::GetSubModelMaterial(G4int subModelID) const
{
    if(subModelMaterial_Map.find(subModelID) == subModelMaterial_Map.end())
        return G4NistManager::Instance()->FindOrBuildMaterial("G4_WATER");
    else
        return subModelMaterial_Map.at(subModelID);
}
G4Colour TETModel::GetSubModelColour(G4int subModelID) const
{
    if(subModelColour_Map.find(subModelID) == subModelColour_Map.end())
        return G4Colour(1., .752941, .627451, .01);
    else
        return subModelColour_Map.at(subModelID);
}

// --- Bone Data --- //
G4double TETModel::GetSubModelMassRatio(G4int subModelID, BoneSubModel boneSubModel) const
{
    if(subModelMassRatio_Map.find(std::make_pair(subModelID, boneSubModel))
        == subModelMassRatio_Map.end())
        return 0.;
    else
        return subModelMassRatio_Map.at(std::make_pair(subModelID, boneSubModel));
}
G4double TETModel::GetSubModelDRF(G4int subModelID, BoneSubModel boneSubModel, G4double kineticEnergy) const
{
    if(subModelDRFs_Map.find(std::make_pair(subModelID, boneSubModel))
        == subModelDRFs_Map.end())
        return 0.;
    else
        return LogInterp_DRF(kineticEnergy, subModelDRFs_Map.at(std::make_pair(subModelID, boneSubModel)));
}

// --- Private functions --- //
void TETModel::ImportNodeData(const G4String& nodeFilePath)
{
    // --- Open node file --- //
    std::ifstream ifs(nodeFilePath.c_str());
    if(!ifs.is_open())
        G4Exception("TETModel::ImportNodeData()", "", FatalErrorInArgument,
            G4String("      There is no file '" + nodeFilePath + "'").c_str());

    G4cout << "  Opening TETGEN node (vertex points: x y z) file '"
           << nodeFilePath << "'" <<G4endl;

    // --- Get data --- //
    G4double xMin(DBL_MAX), yMin(DBL_MAX), zMin(DBL_MAX);
    G4double xMax(-DBL_MAX), yMax(-DBL_MAX), zMax(-DBL_MAX);

    G4int nNodes;
    G4double x, y, z;
    G4int dummyValue;

    // Read header line
    ifs >> nNodes >> dummyValue >> dummyValue >> dummyValue;

    // Read data lines
    for(G4int i = 0; i < nNodes; ++i)
    {
        ifs >> dummyValue >> x >> y >> z;
        x *= cm;
        y *= cm;
        z *= cm;

        node_Vector.push_back(G4ThreeVector(x, y, z));

        // Update Min & Max
        if (x < xMin) xMin = x;
        if (x > xMax) xMax = x;
        if (y < yMin) yMin = y;
        if (y > yMax) yMax = y;
        if (z < zMin) zMin = z;
        if (z > zMax) zMax = z;
    }

    // Set Min, Max, Cen, Size
    fBoundingBoxMin = G4ThreeVector(xMin, yMin, zMin);
    fBoundingBoxMax = G4ThreeVector(xMax, yMax, zMax);
    fBoundingBoxCen = (fBoundingBoxMin + fBoundingBoxMax)/2.;
    fBoundingBoxSize = fBoundingBoxMax - fBoundingBoxMin;

    // --- Close the file --- //
    ifs.close();
}

void TETModel::ImportEleData(const G4String& eleFilePath)
{
    // --- Open ele file --- //
    std::ifstream ifs(eleFilePath.c_str());
    if(!ifs.is_open())
        G4Exception("TETModel::ImportEleData()", "", FatalErrorInArgument,
            G4String("      There is no file '" + eleFilePath + "'").c_str());

    G4cout << "  Opening TETGEN elements (tetrahedron with node No.) file '"
           << eleFilePath << "'" <<G4endl;

    // --- Get data --- //
    G4int nTets;
    size_t nodeID0, nodeID1, nodeID2, nodeID3;
    G4int subModelID;
    G4int dummyValue;

    // Read header line
    ifs >> nTets >> dummyValue >> dummyValue;

    // Read data lines
    for(G4int i = 0; i < nTets; ++i)
    {
        ifs >> dummyValue >> nodeID0 >> nodeID1 >> nodeID2 >> nodeID3;

        if(ifs.get() == '\n') subModelID = -1; // NULL ID. when there is no subModelID in ele file.
        else ifs >> subModelID;

        tet_Vector.push_back(
            new G4Tet("Tet_Solid",
            node_Vector[nodeID0]-fBoundingBoxCen,
            node_Vector[nodeID1]-fBoundingBoxCen,
            node_Vector[nodeID2]-fBoundingBoxCen,
            node_Vector[nodeID3]-fBoundingBoxCen)
            );

        subModelID_Set.insert(subModelID);
        tetID_subModelID_Map[i] = subModelID;
    }

    // --- Close the file --- //
    ifs.close();
}

void TETModel::ImportMaterialData(const G4String& materialFilePath)
{
    // --- Open material file --- //
    std::ifstream ifs(materialFilePath.c_str());
    if(!ifs.is_open()) // Fail to open
    {
        G4Exception("TETModelImport::ImportMaterialData()", "", JustWarning,
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

void TETModel::ImportColourData(const G4String& colourFilePath)
{
    // --- Open colour file --- //
    std::ifstream ifs(colourFilePath.c_str());
    if(!ifs.is_open()) // Fail to open
    {
        G4Exception("TETModel::ImportColourData()", "", JustWarning,
            G4String("      There is no file '" + colourFilePath + "'").c_str());
        return;
    }

    G4cout << "  Opening colour data file '" << colourFilePath << "'" <<G4endl;

    // --- Get data --- //
    G4int subModelID;
    G4double rValue, gValue, bValue, aValue;

    // Read data lines
    while(ifs >> subModelID >> rValue >> gValue >> bValue >> aValue)
        subModelColour_Map[subModelID] = G4Colour(rValue, gValue, bValue, aValue);

    // --- Close the file --- //
    ifs.close();
}

void TETModel::ImportBoneMassRatioData(const G4String& boneMassRatioFilePath)
{
    // --- Open bone mass ratio file (.RBMnBS) --- //
    std::ifstream ifs(boneMassRatioFilePath.c_str());
    if(!ifs.is_open()) // Fail to open
    {
        G4Exception("TETModel::ImportBoneMassRatioData()", "", JustWarning,
            G4String("      There is no file '" + boneMassRatioFilePath + "'").c_str());
        return;
    }

    G4cout << "  Opening bone mass ratio data file '" << boneMassRatioFilePath << "'" <<G4endl;

    // --- Get data --- //
    G4int subModelID;
    G4double RBMMassRatio, BSMassRatio;

    // Read data lines
    while(ifs >> subModelID >> RBMMassRatio >> BSMassRatio)
    {
        subModelMassRatio_Map[std::make_pair(subModelID, BoneSubModel::RBM)] = RBMMassRatio;
        subModelMassRatio_Map[std::make_pair(subModelID, BoneSubModel::BS)] = BSMassRatio;
    }

    // --- Close the file --- //
    ifs.close();
}

void TETModel::ImportBoneDRFData(const G4String& boneDRFFilePath)
{
    // --- Open colour file --- //
    std::ifstream ifs(boneDRFFilePath.c_str());
    if(!ifs.is_open()) // Fail to open
    {
        G4Exception("TETModel::ImportBoneDRFData()", "", JustWarning,
            G4String("      There is no file '" + boneDRFFilePath + "'").c_str());
        return;
    }

    G4cout << "  Opening DRF data file '" << boneDRFFilePath << "'" <<G4endl;

    // --- Get data --- //
    G4int subModelID;

    G4double DRFValue;

    // Read data lines
    while(!ifs.eof())
    {
        ifs >> subModelID;

        for(size_t i = 0; i < energyBin_DRF.size(); ++i)
        {
            ifs >> DRFValue;
            DRFValue *= gray*m2;
            subModelDRFs_Map[std::make_pair(subModelID, BoneSubModel::RBM)].push_back(DRFValue);
        }

        for(size_t i = 0; i < energyBin_DRF.size(); ++i)
        {
            ifs >> DRFValue;
            DRFValue *= gray*m2;
            subModelDRFs_Map[std::make_pair(subModelID, BoneSubModel::BS)].push_back(DRFValue);
        }
    }

    // --- Close the file --- //
    ifs.close();
}

void TETModel::CalculateModelDetails()
{
    G4cout << "  Calculating model details... " <<G4endl;

    // --- Initialize --- //
    fTETModelVolume = 0.;
    fTETModelMass = 0.;
    for(auto subModelID: subModelID_Set)
    {
        subModelVolume_Map[subModelID] = 0.;
        subModelMass_Map[subModelID] = 0.;
        subModelNumTets_Map[subModelID] = 0;
    }

    // --- Calculate submodel volume, mass, nTets --- //
    for(size_t i = 0; i < tet_Vector.size(); ++i)
    {
        G4double tetVolume = tet_Vector[i]->GetCubicVolume();
        G4int subModelID = tetID_subModelID_Map[static_cast<G4int>(i)];
        G4double tetDensity = GetSubModelMaterial(subModelID)->GetDensity();
        G4double tetMass = tetDensity*tetVolume;

        subModelVolume_Map[subModelID] += tetVolume;
        subModelMass_Map[subModelID] += tetMass;
        ++subModelNumTets_Map[subModelID];

        fTETModelVolume += tetVolume;
        fTETModelMass += tetMass;
    }
}

void TETModel::PrintTETModelInformation() const
{
    // --- Print the overall information for each subModel --- //
    G4cout << G4endl
           << std::setw(9)  << "SubModel ID"
           << std::setw(11) << "# of Tets"
           << std::setw(11) << "vol [cm3]"
           << std::setw(11) << "mass [g]"
           << std::setw(11) << "d [g/cm3]"
           << "\t" << "submodel" << G4endl;
    G4cout << "--------------------------------------------------------------------------------"<<G4endl;

    G4cout << std::setiosflags(std::ios::fixed);
    G4cout.precision(3);

    for(auto subModelID: subModelID_Set)
    {
        G4cout << std::setw(9)  << subModelID
               << std::setw(11) << subModelNumTets_Map.at(subModelID)
               << std::setw(11) << GetSubModelVolume(subModelID)/cm3
               << std::setw(11) << GetSubModelMass(subModelID)/g
               << std::setw(11) << GetSubModelMaterial(subModelID)->GetDensity()/(g/cm3)
               << "\t"<< GetSubModelMaterial(subModelID)->GetName() << G4endl;
    }

    // --- Print information for whole TETModel --- //
    G4cout << G4endl;
    G4cout << "   TETModel name              " << fTETModelName << G4endl;
    G4cout << "   TETModel size              " << fBoundingBoxSize.x()/cm << " * " << fBoundingBoxSize.y()/cm << " * " << fBoundingBoxSize.z()/cm << " cm3" << G4endl;
    G4cout << "   TETModel Min. position     " << fBoundingBoxMin.x()/cm << " * " << fBoundingBoxMin.y()/cm << " * " << fBoundingBoxMin.z()/cm << " cm3" << G4endl;
    G4cout << "   TETModel Max. position     " << fBoundingBoxMax.x()/cm << " * " << fBoundingBoxMax.y()/cm << " * " << fBoundingBoxMax.z()/cm << " cm3" << G4endl;
    G4cout << "   TETModel Cen. position     " << fBoundingBoxCen.x()/cm << " * " << fBoundingBoxCen.y()/cm << " * " << fBoundingBoxCen.z()/cm << " cm3" << G4endl;
    G4cout << "   TETModel Volume            " << fTETModelVolume/cm3 << " cm3" << G4endl;
    G4cout << "   TETModel Mass              " << fTETModelMass/g << " g" << G4endl;
    G4cout << "   Number of tetrahedrons     " << tet_Vector.size() << G4endl << G4endl;
}

G4double TETModel::LogInterp_DRF(G4double energyInterest, const std::vector<G4double>& DRFdata) const
{
    if(0.==DRFdata.at(0)) return 0.; // For N/A values of DRF.

    size_t eIDX1 = 0, eIDX2 = 1;
    for(; eIDX2 < (energyBin_DRF.size() - 1); ++eIDX1, ++eIDX2)
        if(energyInterest <= energyBin_DRF.at(eIDX2)) break;

    G4double log10_DRF = // log(y1) + (log(x)-log(x1)) * (log(y2)-log(y1))/(log(x2)-log(x1))
        std::log10(DRFdata.at(eIDX1)) +
        std::log10(energyInterest/energyBin_DRF.at(eIDX1)) *
        std::log10(DRFdata.at(eIDX2)/DRFdata.at(eIDX1)) / std::log10(energyBin_DRF.at(eIDX2)/energyBin_DRF.at(eIDX1));

    return std::pow(10., log10_DRF);
}
