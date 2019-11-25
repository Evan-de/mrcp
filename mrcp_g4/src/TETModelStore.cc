#include "TETModelStore.hh"

TETModelStore::TETModelStore(): std::vector<TETModel*>()
{}

TETModelStore::~TETModelStore()
{
    auto pStore = GetInstance();

    G4cout << "Removing TETModels..." << G4endl;
    for(auto pModel: *pStore)
        if(pModel) delete pModel;

    pStore->clear();

    G4cout << "Removing done..." << G4endl;
}

void TETModelStore::Register(TETModel* pModel)
{
    GetInstance()->push_back(pModel);
}

TETModel* TETModelStore::GetTETModel(const G4String& name) const
{
    auto pStore = GetInstance();
    for(auto pModel: *pStore)
        if(pModel->GetName()==name) return pModel;

    G4Exception("TETModelStore::GetTETModel()", "", JustWarning,
        G4String("      No TETModel exists named '" + name + "'." ).c_str());
    return nullptr;
}

