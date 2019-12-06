#include "Primary_ParticleGun.hh"
#include "TETModelStore.hh"

Primary_ParticleGun::Primary_ParticleGun()
: G4VUserPrimaryGeneratorAction()
{
    fPrimary = new G4ParticleGun();

    // Messenger setting
    fMessenger = new G4GenericMessenger(this, "/gun/");

    auto& angleBiasingCmd =
            fMessenger->DeclareProperty("angleBiasing", fAngleBiasingPVName, "Bias the source direction to a physical volume.");
    angleBiasingCmd.SetParameterName("physicalVolume", true);
    angleBiasingCmd.SetDefaultValue("");

    auto& radioNuclideCmd =
            fMessenger->DeclareMethod("radioNuclide", &Primary_ParticleGun::SetRadioNuclide, "Co60p, Cs137p, Ir192p");
    radioNuclideCmd.SetParameterName("radioNuclideName", true);
    radioNuclideCmd.SetDefaultValue("");
}

Primary_ParticleGun::~Primary_ParticleGun()
{
    delete fPrimary;
    delete fMessenger;
    if(fRadioNuclide) delete fRadioNuclide;
}

void Primary_ParticleGun::GeneratePrimaries(G4Event* anEvent)
{
    G4double particleWeight = 1.;

    if(fRadioNuclide)
    {
        const auto decayProduct = fRadioNuclide->SampleDecayProduct(particleWeight);
        fPrimary->SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle(decayProduct.particleName));
        fPrimary->SetParticleEnergy(decayProduct.energy);
    }

    auto dirVec = SampleDirectionFromTo(
                fPrimary->GetParticlePosition(), fAngleBiasingPVName, particleWeight);
    fPrimary->SetParticleMomentumDirection(dirVec);

    fPrimary->GeneratePrimaryVertex(anEvent);
    anEvent->GetPrimaryVertex()->SetWeight(particleWeight);
}

void Primary_ParticleGun::SetRadioNuclide(const G4String& radioNuclideName)
{
    if(radioNuclideName=="Co60p")
    {
        if(fRadioNuclide) delete fRadioNuclide;

        fRadioNuclide = new RadioNuclide("Co-60", "nuclides/Co-60.RAD");
        fRadioNuclide->AddInterestingRadiation(
                {RadioNuclide::Radiation::Gamma,
                 RadioNuclide::Radiation::Xray,
                 RadioNuclide::Radiation::AnnihilationPhoton}
                );
        fRadioNuclide->SetRadiationEnergyThreshold(RadioNuclide::Radiation::Xray, 1.*keV);
        fRadioNuclide->SetRadiationYieldThreshold(RadioNuclide::Radiation::Xray, 1e-3);

        return;
    }

    if(radioNuclideName=="Cs137p")
    {
        if(fRadioNuclide) delete fRadioNuclide;

        fRadioNuclide = new RadioNuclide("Cs-137", "nuclides/Cs-137.RAD");
        fRadioNuclide->AddInterestingRadiation(
                {RadioNuclide::Radiation::Gamma,
                 RadioNuclide::Radiation::Xray,
                 RadioNuclide::Radiation::AnnihilationPhoton}
                );
        fRadioNuclide->SetRadiationEnergyThreshold(RadioNuclide::Radiation::Xray, 1.*keV);
        fRadioNuclide->SetRadiationYieldThreshold(RadioNuclide::Radiation::Xray, 1e-3);

        auto ba137m = new RadioNuclide("Ba-137m", "nuclides/Ba-137m.RAD", 9.440e-1);
        ba137m->AddInterestingRadiation(
                {RadioNuclide::Radiation::Gamma,
                RadioNuclide::Radiation::Xray,
                RadioNuclide::Radiation::AnnihilationPhoton}
                );
        ba137m->SetRadiationEnergyThreshold(RadioNuclide::Radiation::Xray, 1.*keV);
        ba137m->SetRadiationYieldThreshold(RadioNuclide::Radiation::Xray, 1e-3);

        fRadioNuclide->AddRadioactiveDaughter(ba137m);

        return;
    }

    if(radioNuclideName=="Ir192p")
    {
        if(fRadioNuclide) delete fRadioNuclide;

        fRadioNuclide = new RadioNuclide("Ir-192", "nuclides/Ir-192.RAD");
        fRadioNuclide->AddInterestingRadiation(
                {RadioNuclide::Radiation::Gamma,
                 RadioNuclide::Radiation::Xray,
                 RadioNuclide::Radiation::AnnihilationPhoton}
                );
        fRadioNuclide->SetRadiationEnergyThreshold(RadioNuclide::Radiation::Xray, 1.*keV);
        fRadioNuclide->SetRadiationYieldThreshold(RadioNuclide::Radiation::Xray, 1e-3);

        return;
    }
}
