#include "Tier1A.h"
#include "Tier2A.h"
#include "Tier2B.h"
#include "Tier2C.h"

ATier1A::ATier1A()
{
    Level = ETierLevel::Tier1;
    Density = ETierDensity::Sparse;
}

TSubclassOf<ATierClusterBase> ATier1A::ChildClassForDensity(ETierDensity InDensity) const
{
    switch (InDensity)
    {
        case ETierDensity::Sparse:   return ATier2A::StaticClass();
        case ETierDensity::Standard: return ATier2B::StaticClass();
        case ETierDensity::Dense:    return ATier2C::StaticClass();
    }
    return ATier2A::StaticClass();
}
