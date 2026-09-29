// TransitNetworkDetails.cpp — Detail panel for ATransitNetworkActor
#include "TransitNetworkDetails.h"
#include "TransitNetworkActor.h"
#include "TransitSplineComponent.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "TransitNetworkDetails"

TSharedRef<IDetailCustomization> FTransitNetworkDetails::MakeInstance()
{
	return MakeShareable(new FTransitNetworkDetails);
}

void FTransitNetworkDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	// Get the actor being edited
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder.GetObjectsBeingCustomized(Objects);

	TWeakObjectPtr<ATransitNetworkActor> NetworkActorWeak;
	for (const TWeakObjectPtr<UObject>& Obj : Objects)
	{
		if (ATransitNetworkActor* Actor = Cast<ATransitNetworkActor>(Obj.Get()))
		{
			NetworkActorWeak = Actor;
			break;
		}
	}

	// Add a quick-actions category at the top
	IDetailCategoryBuilder& ActionsCategory = DetailBuilder.EditCategory(
		"Transit Architect|Actions", LOCTEXT("ActionsCategory", "Actions"), ECategoryPriority::Important);

	ActionsCategory.AddCustomRow(LOCTEXT("RebuildRow", "Rebuild"))
		.WholeRowContent()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2)
			[
				SNew(SButton)
				.Text(LOCTEXT("RebuildButton", "Rebuild Network"))
				.OnClicked_Lambda([NetworkActorWeak]() -> FReply
				{
					if (ATransitNetworkActor* Actor = NetworkActorWeak.Get())
					{
						Actor->RebuildNetwork();
					}
					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2)
			[
				SNew(SButton)
				.Text(LOCTEXT("AddSplineButton", "Add Transit Spline"))
				.OnClicked_Lambda([NetworkActorWeak]() -> FReply
				{
					if (ATransitNetworkActor* Actor = NetworkActorWeak.Get())
					{
						Actor->AddTransitSpline();
					}
					return FReply::Handled();
				})
			]
		];

	// Show spline count
	ActionsCategory.AddCustomRow(LOCTEXT("SplineCountRow", "Spline Count"))
		.NameContent()
		[
			SNew(STextBlock).Text(LOCTEXT("SplineCountLabel", "Splines"))
		]
		.ValueContent()
		[
			SNew(STextBlock)
			.Text_Lambda([NetworkActorWeak]() -> FText
			{
				if (ATransitNetworkActor* Actor = NetworkActorWeak.Get())
				{
					TArray<UActorComponent*> Comps;
					Actor->GetComponents(UTransitSplineComponent::StaticClass(), Comps);
					return FText::AsNumber(Comps.Num());
				}
				return FText::AsNumber(0);
			})
		];
}

#undef LOCTEXT_NAMESPACE
