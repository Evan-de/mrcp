#ifndef TETModelStore_hh_
#define TETModelStore_hh_

#include "TETModel.hh"

#include <vector>

class TETModelStore: public std::vector<TETModel*>
{
public:
    static TETModelStore* GetInstance()
    {
        static TETModelStore* fInstance = new TETModelStore;
        return fInstance;
    }
    static void Register(TETModel* pTET);

    TETModel* GetTETModel(const G4String& name) const;

    virtual ~TETModelStore();

private:
    TETModelStore();
};

#endif
