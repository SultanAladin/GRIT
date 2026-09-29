// RoadIntersection.cpp — Road network intersection node implementation
#include "RoadIntersection.h"
#include "RoadSegment.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

ARoadIntersection::ARoadIntersection()
{
	PrimaryActorTick.bCanEverTick = false;

	IntersectionMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("IntersectionMesh"));
	RootComponent = IntersectionMesh;
}

void ARoadIntersection::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshSockets();

	// Rebuild any connected road segments so they follow when the intersection moves
	for (const FSocketConnection& Conn : Connections)
	{
		if (Conn.ConnectedSegment)
		{
			Conn.ConnectedSegment->RebuildRoad();
		}
	}
}

#if WITH_EDITOR
void ARoadIntersection::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName PropName = PropertyChangedEvent.GetPropertyName();
	if (PropName == GET_MEMBER_NAME_CHECKED(ARoadIntersection, IntersectionMesh))
	{
		RefreshSockets();
	}
}
#endif

void ARoadIntersection::RefreshSockets()
{
	if (!IntersectionMesh || !IntersectionMesh->GetStaticMesh())
	{
		Connections.Empty();
		return;
	}

	TArray<FName> SocketNames = IntersectionMesh->GetAllSocketNames();

	// Build a map of existing connections so we can preserve them
	TMap<FName, ARoadSegment*> ExistingMap;
	for (const FSocketConnection& Conn : Connections)
	{
		if (Conn.ConnectedSegment)
		{
			ExistingMap.Add(Conn.SocketName, Conn.ConnectedSegment);
		}
	}

	Connections.Empty();
	Connections.Reserve(SocketNames.Num());

	for (const FName& Name : SocketNames)
	{
		FSocketConnection Entry;
		Entry.SocketName = Name;
		Entry.ConnectedSegment = ExistingMap.FindRef(Name); // nullptr if not found
		Connections.Add(Entry);
	}
}

TArray<FName> ARoadIntersection::GetAvailableSockets() const
{
	TArray<FName> Available;
	for (const FSocketConnection& Conn : Connections)
	{
		if (!Conn.ConnectedSegment)
		{
			Available.Add(Conn.SocketName);
		}
	}
	return Available;
}

ARoadSegment* ARoadIntersection::ConnectTo(FName LocalSocket, ARoadIntersection* Other, FName OtherSocket)
{
	if (!Other || Other == this)
	{
		UE_LOG(LogTemp, Warning, TEXT("RoadIntersection::ConnectTo — Invalid target intersection."));
		return nullptr;
	}

	// Validate local socket exists and is free
	FSocketConnection* LocalConn = Connections.FindByPredicate(
		[&](const FSocketConnection& C) { return C.SocketName == LocalSocket; });
	if (!LocalConn)
	{
		UE_LOG(LogTemp, Warning, TEXT("RoadIntersection::ConnectTo — Socket '%s' not found on this intersection."), *LocalSocket.ToString());
		return nullptr;
	}
	if (LocalConn->ConnectedSegment)
	{
		UE_LOG(LogTemp, Warning, TEXT("RoadIntersection::ConnectTo — Socket '%s' is already connected."), *LocalSocket.ToString());
		return nullptr;
	}

	// Validate other socket
	FSocketConnection* OtherConn = Other->Connections.FindByPredicate(
		[&](const FSocketConnection& C) { return C.SocketName == OtherSocket; });
	if (!OtherConn)
	{
		UE_LOG(LogTemp, Warning, TEXT("RoadIntersection::ConnectTo — Socket '%s' not found on target intersection."), *OtherSocket.ToString());
		return nullptr;
	}
	if (OtherConn->ConnectedSegment)
	{
		UE_LOG(LogTemp, Warning, TEXT("RoadIntersection::ConnectTo — Socket '%s' on target is already connected."), *OtherSocket.ToString());
		return nullptr;
	}

	// Spawn the road segment
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ARoadSegment* Segment = GetWorld()->SpawnActor<ARoadSegment>(
		ARoadSegment::StaticClass(), GetActorLocation(), FRotator::ZeroRotator, SpawnParams);

	if (!Segment)
	{
		UE_LOG(LogTemp, Error, TEXT("RoadIntersection::ConnectTo — Failed to spawn ARoadSegment."));
		return nullptr;
	}

	// Wire up both ends
	LocalConn->ConnectedSegment = Segment;
	OtherConn->ConnectedSegment = Segment;

	Segment->Initialize(this, LocalSocket, Other, OtherSocket);

	return Segment;
}

void ARoadIntersection::Disconnect(FName SocketName)
{
	FSocketConnection* Conn = Connections.FindByPredicate(
		[&](const FSocketConnection& C) { return C.SocketName == SocketName; });
	if (!Conn || !Conn->ConnectedSegment)
	{
		return;
	}

	ARoadSegment* Segment = Conn->ConnectedSegment;

	// Clear the other end's connection
	ARoadIntersection* OtherIntersection = nullptr;
	FName OtherSocket;

	if (Segment->StartIntersection == this)
	{
		OtherIntersection = Segment->EndIntersection;
		OtherSocket = Segment->EndSocket;
	}
	else
	{
		OtherIntersection = Segment->StartIntersection;
		OtherSocket = Segment->StartSocket;
	}

	if (OtherIntersection)
	{
		FSocketConnection* OtherConn = OtherIntersection->Connections.FindByPredicate(
			[&](const FSocketConnection& C) { return C.SocketName == OtherSocket; });
		if (OtherConn)
		{
			OtherConn->ConnectedSegment = nullptr;
		}
	}

	// Clear local connection and destroy the segment
	Conn->ConnectedSegment = nullptr;
	Segment->Destroy();
}
