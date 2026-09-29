#include "Tier2A.h"
#include "Tier3A.h"
#include "Tier3B.h"
#include "Tier3C.h"

ATier2A::ATier2A()
{
    Level = ETierLevel::Tier2;
    Density = ETierDensity::Sparse;
}

TSubclassOf<ATierClusterBase> ATier2A::ChildClassForDensity(ETierDensity InDensity) const
{
    switch (InDensity)
    {
        case ETierDensity::Sparse:   return ATier3A::StaticClass();
        case ETierDensity::Standard: return ATier3B::StaticClass();
        case ETierDensity::Dense:    return ATier3C::StaticClass();
    }
    return ATier3A::StaticClass();
}
