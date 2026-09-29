// TransitNetworkDetails.h — Detail panel customization for ATransitNetworkActor
#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

class FTransitNetworkDetails : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};
