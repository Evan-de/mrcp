#include "PrimarySamplingHelper.hh"

G4ThreeVector SampleDirectionFromTo(const G4ThreeVector& referencePoint,
                                    const G4String& physicalVolumeName,
                                    G4double& particleWeight,
                                    G4double margin)
{
    // No physical volume setting
    if(!physicalVolumeName.size()) return G4RandomDirection();

    // Invalid physical volume name
    auto physicalVolume = G4PhysicalVolumeStore::GetInstance()->GetVolume(physicalVolumeName);
    if(!physicalVolume)
    {
        G4Exception("RandomDirectionFromTo()", "", JustWarning,
                    G4String("      invalid physical volume '" + physicalVolumeName + "'" ).c_str());
        return G4RandomDirection();
    }

    // Get the envelope box of the physical volume
    G4ThreeVector pvPosition, pvMin, pvMax;
    pvPosition = physicalVolume->GetObjectTranslation();
    physicalVolume->GetLogicalVolume()->GetSolid()->BoundingLimits(pvMin, pvMax);
    // Add margin to the box
    if(margin>0.)
    {
        pvMin = pvMin - G4ThreeVector(margin, margin, margin);
        pvMax = pvMax + G4ThreeVector(margin, margin, margin);
    }

    // Calculate the direction to the box
    G4ThreeVector targetVector = pvPosition - referencePoint;
    G4ThreeVectorList pvBoxVerticies =
    {
        G4ThreeVector(pvMin.x(), pvMin.y(), pvMin.z()) + targetVector,
        G4ThreeVector(pvMin.x(), pvMin.y(), pvMax.z()) + targetVector,
        G4ThreeVector(pvMin.x(), pvMax.y(), pvMin.z()) + targetVector,
        G4ThreeVector(pvMin.x(), pvMax.y(), pvMax.z()) + targetVector,
        G4ThreeVector(pvMax.x(), pvMin.y(), pvMin.z()) + targetVector,
        G4ThreeVector(pvMax.x(), pvMin.y(), pvMax.z()) + targetVector,
        G4ThreeVector(pvMax.x(), pvMax.y(), pvMin.z()) + targetVector,
        G4ThreeVector(pvMax.x(), pvMax.y(), pvMax.z()) + targetVector
    };
    G4double maxTheta = 0.;
    for(const auto& vertex: pvBoxVerticies)
    {
        G4double target2VertexAngle = targetVector.angle(vertex);
        maxTheta = (target2VertexAngle>maxTheta) ? target2VertexAngle : maxTheta;
    }

    // If the ref point is in the bounding box of the physical volume, return 4pi dir.
    G4double cosTheta = std::cos(maxTheta);
    if(cosTheta<=0.) return G4RandomDirection();

    // Multiply particle weight as its solid angle reduction
    G4double solidAngle = CLHEP::twopi * (1 - cosTheta);
    particleWeight *= solidAngle / (4 * CLHEP::pi);

    // Sample a direction and rotate to targetVector
    G4ThreeVector dirVec = G4RandomDirection(cosTheta);
    G4ThreeVector zUnit = G4ThreeVector(0., 0., 1);
    if(!(targetVector.isParallel(zUnit)))
        dirVec.rotate(zUnit.cross(targetVector), zUnit.angle(targetVector));
    else
        dirVec *= targetVector.unit().dot(zUnit);

    return dirVec;
}

RadioNuclide::RadioNuclide(G4String name, const G4String& decayDataFilePath, G4double branchingRatio)
    : fRadioNuclideName(name), fBranchingRatio(branchingRatio)
{
    // --- Open nuclide data file --- //
    // *.RAD file exported from DECDATA Program (ICRP107)
    std::ifstream ifs(decayDataFilePath.c_str());
    if(!ifs.is_open())
        G4Exception("SampleDecayProduct()", "", FatalErrorInArgument,
            G4String("      No nuclide data file '" + decayDataFilePath + "'" ).c_str());

    G4cout << "  Reading nuclide data file '"
           << decayDataFilePath << "'" <<G4endl;

    // --- Get data --- //
    // Find the line "START RADIATION RECORDS"
    while(!ifs.eof())
    {
        // Get each line
        G4String thisLine;
        std::getline(ifs, thisLine);

        if(thisLine=="START RADIATION RECORDS\r") break;
    }

    // Read data until to find the line "END RADIATION RECORDS"
    while(!ifs.eof())
    {
        // Get each line
        G4String thisLine;
        std::getline(ifs, thisLine);

        if(thisLine=="END RADIATION RECORDS\r") break;

        DecayData decayData;
        std::stringstream ss(thisLine);
        G4int iCode;
        G4double yield;
        G4double energy;
        G4String mnemonic;
        ss >> iCode >> yield >> energy >> mnemonic;
        fDecayData[static_cast<Radiation>(iCode)][energy*MeV] = yield;
    }

    // --- Close the file --- //
    ifs.close();
}

RadioNuclide::~RadioNuclide()
{
    for(auto& radioactiveDaughter: fRadioactiveDaughters)
        delete radioactiveDaughter;
}

void RadioNuclide::AddInterestingRadiation(Radiation radiation)
{
    fNormalized = false;
    if(fDecayData.find(radiation)!=fDecayData.end())
        fDecayDataInteresting[radiation] = fDecayData.at(radiation);
}

void RadioNuclide::AddInterestingRadiation(std::vector<Radiation> radiations)
{
    for(const auto& radiation: radiations)
        AddInterestingRadiation(radiation);
}

void RadioNuclide::RemoveInterestingRadiation(Radiation radiation)
{
    fNormalized = false;
    fDecayDataInteresting.erase(radiation);
}

void RadioNuclide::SetRadiationEnergyThreshold(Radiation radiation, G4double energy)
{
    fNormalized = false;

    if(fDecayDataInteresting.find(radiation)==fDecayDataInteresting.end())
        return;

    for(const auto& energyYieldData: fDecayDataInteresting.at(radiation))
        if(energyYieldData.first<energy)
            fDecayDataInteresting[radiation].erase(energyYieldData.first);
}

void RadioNuclide::SetRadiationYieldThreshold(Radiation radiation, G4double yield)
{
    fNormalized = false;

    if(fDecayDataInteresting.find(radiation)==fDecayDataInteresting.end())
        return;

    for(const auto& energyYieldData: fDecayDataInteresting.at(radiation))
        if(energyYieldData.second<yield)
            fDecayDataInteresting[radiation].erase(energyYieldData.first);
}

void RadioNuclide::Normalize()
{
    // --- Calculate total yield --- //
    fTotalYield = 0.;
    // Itself
    for(const auto& decayData: fDecayDataInteresting)
        for(const auto& energyYieldData: decayData.second)
            fTotalYield += energyYieldData.second;
    // Its radioactive daughters
    for(const auto& radioactiveDaughter: fRadioactiveDaughters)
        for(const auto& decayData: radioactiveDaughter->fDecayDataInteresting)
            for(const auto& energyYieldData: decayData.second)
                fTotalYield += energyYieldData.second * radioactiveDaughter->fBranchingRatio;

    // --- Normalize the probabilities and record the cumulative probabilities --- //
    G4double cumulativeProbability = 0.;
    fNormalizedDecayData_Vector.clear();
    // Itself
    for(const auto& decayData: fDecayDataInteresting)
        for(const auto& energyYieldData: decayData.second)
        {
            DecayProduct decayProduct{RadioNuclide::ICode2G4ParticleName(decayData.first), energyYieldData.first};
            fNormalizedDecayData_Vector.push_back(
                        std::make_pair(cumulativeProbability += energyYieldData.second / fTotalYield, decayProduct)
                        );
        }
    // Its radioactive daughters
    for(const auto& radioactiveDaughter: fRadioactiveDaughters)
        for(const auto& decayData: radioactiveDaughter->fDecayDataInteresting)
            for(const auto& energyYieldData: decayData.second)
            {
                DecayProduct decayProduct{RadioNuclide::ICode2G4ParticleName(decayData.first), energyYieldData.first};
                fNormalizedDecayData_Vector.push_back(
                            std::make_pair(cumulativeProbability += energyYieldData.second * radioactiveDaughter->fBranchingRatio / fTotalYield, decayProduct)
                            );
            }

    fNormalized = true;
}

RadioNuclide::DecayProduct RadioNuclide::SampleDecayProduct(G4double& particleWeight)
{
    if(!fNormalized) Normalize();

    particleWeight *= fTotalYield;

    G4double rnd = G4UniformRand();
    size_t i = 0;
    while(rnd>std::get<0>(fNormalizedDecayData_Vector.at(i))) ++i;

    return std::get<1>(fNormalizedDecayData_Vector.at(i));
}

G4String RadioNuclide::ICode2G4ParticleName(Radiation radiation)
{
    switch(radiation)
    {
    case Radiation::Gamma:
    case Radiation::Xray:
    case Radiation::AnnihilationPhoton:
        return "gamma";
    case Radiation::BetaPlus:
        return "e+";
    case Radiation::BetaMinus:
    case Radiation::InternalConversionElectron:
    case Radiation::AugerElectron:
        return "e-";
    case Radiation::Alpha:
        return "alpha";
    case Radiation::Neutron:
        return "neutron";
    default:
        return "geantino"; // not supported
    }
}
