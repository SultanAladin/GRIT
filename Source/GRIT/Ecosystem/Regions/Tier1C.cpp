#include "Tier1C.h"
#include "Tier2A.h"
#include "Tier2B.h"
#include "Tier2C.h"

ATier1C::ATier1C()
{
    Level = ETierLevel::Tier1;
    Density = ETierDensity::Dense;
}

TSubclassOf<ATierClusterBase> ATier1C::ChildClassForDensity(ETierDensity InDensity) const
{
    switch (InDensity)
    {
        case ETierDensity::Sparse:   return ATier2A::StaticClass();
        case ETierDensity::Standard: return ATier2B::StaticClass();
        case ETierDensity::Dense:    return ATier2C::StaticClass();
    }
    return ATier2C::StaticClass();
}
