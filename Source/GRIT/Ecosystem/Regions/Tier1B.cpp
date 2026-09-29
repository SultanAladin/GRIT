#include "Tier1B.h"
#include "Tier2A.h"
#include "Tier2B.h"
#include "Tier2C.h"

ATier1B::ATier1B()
{
    Level = ETierLevel::Tier1;
    Density = ETierDensity::Standard;
}

TSubclassOf<ATierClusterBase> ATier1B::ChildClassForDensity(ETierDensity InDensity) const
{
    switch (InDensity)
    {
        case ETierDensity::Sparse:   return ATier2A::StaticClass();
        case ETierDensity::Standard: return ATier2B::StaticClass();
        case ETierDensity::Dense:    return ATier2C::StaticClass();
    }
    return ATier2B::StaticClass();
}
