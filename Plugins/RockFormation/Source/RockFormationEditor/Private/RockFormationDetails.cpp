#include "RockFormationDetails.h"
#include "RockFormationActor.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "RockFormationDetails"

TSharedRef<IDetailCustomization> FRockFormationDetails::MakeInstance()
{
	return MakeShareable(new FRockFormationDetails);
}

void FRockFormationDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder.GetObjectsBeingCustomized(Objects);

	TWeakObjectPtr<ARockFormationActor> Actor;
	if (Objects.Num() > 0)
	{
		Actor = Cast<ARockFormationActor>(Objects[0].Get());
	}

	IDetailCategoryBuilder& ActionsCategory =
		DetailBuilder.EditCategory("Rock Formation Actions", FText::GetEmpty(), ECategoryPriority::Important);

	// Rebuild button
	ActionsCategory.AddCustomRow(LOCTEXT("RebuildRow", "Rebuild"))
	.NameContent()
	[
		SNew(STextBlock)
		.Text(LOCTEXT("RebuildLabel", "Rebuild Formation"))
		.Font(IDetailLayoutBuilder::GetDetailFont())
	]
	.ValueContent()
	.MaxDesiredWidth(200.f)
	[
		SNew(SButton)
		.Text(LOCTEXT("RebuildBtn", "Rebuild"))
		.OnClicked_Lambda([Actor]()
		{
			if (Actor.IsValid())
			{
				Actor->RebuildFormation();
			}
			return FReply::Handled();
		})
	];

	// Bake button
	ActionsCategory.AddCustomRow(LOCTEXT("BakeRow", "Bake"))
	.NameContent()
	[
		SNew(STextBlock)
		.Text(LOCTEXT("BakeLabel", "Bake to StaticMesh"))
		.Font(IDetailLayoutBuilder::GetDetailFont())
	]
	.ValueContent()
	.MaxDesiredWidth(200.f)
	[
		SNew(SButton)
		.Text(LOCTEXT("BakeBtn", "Bake"))
		.OnClicked_Lambda([Actor]()
		{
			if (Actor.IsValid())
			{
				Actor->BakeMesh();
			}
			return FReply::Handled();
		})
	];

	// Clear button
	ActionsCategory.AddCustomRow(LOCTEXT("ClearRow", "Clear"))
	.NameContent()
	[
		SNew(STextBlock)
		.Text(LOCTEXT("ClearLabel", "Clear Formation"))
		.Font(IDetailLayoutBuilder::GetDetailFont())
	]
	.ValueContent()
	.MaxDesiredWidth(200.f)
	[
		SNew(SButton)
		.Text(LOCTEXT("ClearBtn", "Clear"))
		.OnClicked_Lambda([Actor]()
		{
			if (Actor.IsValid())
			{
				Actor->ClearFormation();
			}
			return FReply::Handled();
		})
	];

	// Status display
	ActionsCategory.AddCustomRow(LOCTEXT("StatusRow", "Status"))
	.NameContent()
	[
		SNew(STextBlock)
		.Text(LOCTEXT("StatusLabel", "Status"))
		.Font(IDetailLayoutBuilder::GetDetailFont())
	]
	.ValueContent()
	.MaxDesiredWidth(400.f)
	[
		SNew(STextBlock)
		.Text_Lambda([Actor]()
		{
			if (Actor.IsValid())
			{
				return FText::FromString(FString::Printf(TEXT("%d rocks | %s"),
					Actor->RockCount, *Actor->StatusReport));
			}
			return FText::GetEmpty();
		})
		.Font(IDetailLayoutBuilder::GetDetailFont())
	];
}

#undef LOCTEXT_NAMESPACE
