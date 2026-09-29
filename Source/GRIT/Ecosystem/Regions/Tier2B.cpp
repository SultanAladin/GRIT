#include "Tier2B.h"
#include "Tier3A.h"
#include "Tier3B.h"
#include "Tier3C.h"

ATier2B::ATier2B()
{
    Level = ETierLevel::Tier2;
    Density = ETierDensity::Standard;
}

TSubclassOf<ATierClusterBase> ATier2B::ChildClassForDensity(ETierDensity InDensity) const
{
    switch (InDensity)
    {
        case ETierDensity::Sparse:   return ATier3A::StaticClass();
        case ETierDensity::Standard: return ATier3B::StaticClass();
        case ETierDensity::Dense:    return ATier3C::StaticClass();
    }
    return ATier3B::StaticClass();
}
