#include "Tier2C.h"
#include "Tier3A.h"
#include "Tier3B.h"
#include "Tier3C.h"

ATier2C::ATier2C()
{
    Level = ETierLevel::Tier2;
    Density = ETierDensity::Dense;
}

TSubclassOf<ATierClusterBase> ATier2C::ChildClassForDensity(ETierDensity InDensity) const
{
    switch (InDensity)
    {
        case ETierDensity::Sparse:   return ATier3A::StaticClass();
        case ETierDensity::Standard: return ATier3B::StaticClass();
        case ETierDensity::Dense:    return ATier3C::StaticClass();
    }
    return ATier3C::StaticClass();
}
