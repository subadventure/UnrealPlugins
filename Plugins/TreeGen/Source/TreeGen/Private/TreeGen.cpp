// Copyright Epic Games, Inc. All Rights Reserved.

#include "TreeGen.h"
#include "Materials/MaterialParameterCollection.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "InstancedFoliageActor.h"
#include "InstancedFoliage.h"
#include "StaticMeshAttributes.h"
#include "PhysicsEngine/BodySetup.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FTreeGenModule"

void FTreeGenModule::StartupModule()
{
	UE_LOG(LogTemp, Warning, TEXT("StartupModule"));
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FTreeGenModule::ShutdownModule()
{
	UE_LOG(LogTemp, Warning, TEXT("ShutdownModule"));

	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FTreeGenModule, TreeGen)


ATreeGenActor::ATreeGenActor()
{
	Spline = CreateDefaultSubobject<USplineComponent>("SplineComponent");
	Spline->SetupAttachment(GetStaticMeshComponent());

	UE_LOG(LogTemp, Display, TEXT("ATreeGenActor Constructor"));
}

void ATreeGenActor::CleanupPreviousBuild()
{

	for (USplineComponent* Branch : BranchComponents) {
		if (Branch)Branch->DestroyComponent();
	}

	Branches.Empty();
	BranchComponents.Empty();

	GetStaticMeshComponent()->SetStaticMesh(nullptr);

	AInstancedFoliageActor* FoliageActor = Cast<AInstancedFoliageActor>(UGameplayStatics::GetActorOfClass(GetWorld(), AInstancedFoliageActor::StaticClass()));

	if (!FoliageActor) {
		FoliageActor = GetWorld()->SpawnActor<AInstancedFoliageActor>();
	}
	else {
		for (TPair<UFoliageType*, TArray<FTransform>> Pair : LeafsInstanceLocations) {

			FFoliageInfo* Info = FoliageActor->FindInfo(Pair.Key);

			TArray<int> Indexes;

			for (FTransform Transform : Pair.Value) {

				int Index = 0; bool OutSucces = false;
				Info->GetInstanceAtLocation(Transform.GetLocation(), Index, OutSucces);
				if (OutSucces) {
					Indexes.Add(Index);
				}
			}
			Info->RemoveInstances(Indexes, false);
			Info->Refresh(false, true);
		}
	}
	LeafsInstanceLocations.Empty();
}

void ATreeGenActor::Generate()
{
	UE_LOG(LogTemp, Display, TEXT("ATreeGenActor Generate"));

	CleanupPreviousBuild();

	InitBranches();

	GetStaticMeshComponent()->SetStaticMesh(CreateRuntimeMesh(CreateMeshDescriptions()));

}
void ATreeGenActor::InitBranches()
{
	UE_LOG(LogTemp, Display, TEXT("ATreeGenActor InitBranches"));

	RealBounds = { 0,0 };

	const float Length = TreeHeight;
	const float Radius = TreeHeight / 30.0;

	FTreeBranchInfos NewInfo(0, 0, Length, Radius, 0, { FVector(0),FVector(0,0,DistanceBetweenSplinePoints) }, { FVector(0),FVector(0,0,DistanceBetweenSplinePoints) });
	AddBranchPoints(NewInfo);

	Branches.Add({ NewInfo });

	for (int i = 0; i < Depth; i++) {		// Depth parameter for Number of Branch layers

		Branches.Add({});

		for (FTreeBranchInfos Branch : Branches[i]) {		// Recursive iteration through each branch layer

			AddBranches(Branch, i + 1);		// Adding branches on each branch
		}
	}
}

void ATreeGenActor::AddBranches(FTreeBranchInfos& InParent, const int InDepth)
{
	//UE_LOG(LogTemp, Display, TEXT("Adding branches for Depth:%i"), InDepth);

	Spline->SetSplineLocalPoints(InParent.Points);

	float Angle = 0;
	const float DepthScale = float(InDepth) / float(Depth) * (float(InParent.Depth) / float(Depth));
	const float SectionStart = FMath::Lerp(BranchHeight,InParent.BaseRadius, DepthScale);
	const float SectionLength = (InParent.Length - SectionStart) / MaxNumberBranchChilds;

	for (int i = 0; i < MaxNumberBranchChilds; i++) {

		const float Distance = SectionStart + float(i) * SectionLength;

		const FVector Center = Spline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::Local);
		const FVector Direction = Spline->GetDirectionAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::Local);
		const FVector UpVector = Spline->GetUpVectorAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::Local);

		const float Radius = FMath::Lerp(InParent.BaseRadius * .6, InParent.BaseRadius * .3, Distance / InParent.Length);
		const float Length = FMath::Lerp(InParent.Length * .7, InParent.Length * .3, Distance / InParent.Length);

		const float DirectionAngle = FMath::Lerp(90, 10, Distance / InParent.Length);
		Angle = UKismetMathLibrary::RandomFloatInRangeFromStream(Stream, Angle + 45, (Angle - 45) + 360.0);

		const FVector BranchDirection = Direction.RotateAngleAxis(DirectionAngle, UpVector).RotateAngleAxis(Angle, Direction);

		FTreeBranchInfos NewInfo (InDepth, Branches[InDepth].Num(), Length, Radius, Distance, {Center, Center + BranchDirection * DistanceBetweenSplinePoints}, {Center,Center});
		AddBranchPoints(NewInfo);
		Branches[InDepth].Add(NewInfo);

	}
	UE_LOG(LogTemp, Display, TEXT("Added branches for Depth:%i - %i"), InDepth, Branches[InDepth].Num());
}
void ATreeGenActor::AddBranchPoints(FTreeBranchInfos& InInfos)
{
	UE_LOG(LogTemp, Display, TEXT("adding branch points.. Infos.Depth:%i  Depth%i"), InInfos.Depth, Depth);
	
	const int NumberPoints = FMath::CeilToInt(InInfos.Length / DistanceBetweenSplinePoints);

	for (int i = 0; i < NumberPoints; i++) {

		const FVector LatestDirection = (InInfos.Points.Last() - InInfos.Points[InInfos.Points.Num() - 2]).GetSafeNormal();

		const float ConeAngle = FMath::Lerp(BranchConeAngle * .2, BranchConeAngle, float(InInfos.Depth) / float(Depth));

		const FVector LocalDirection = UKismetMathLibrary::RandomUnitVectorInConeInDegreesFromStream(Stream, LatestDirection, ConeAngle);

		FVector Location = InInfos.Points.Last() + DistanceBetweenSplinePoints * FVector(LocalDirection.X, LocalDirection.Y, FMath::Abs(LocalDirection.Z));

		InInfos.Points.Add(Location);

		RealBounds += Location;
		InInfos.RealBounds += Location;
	}
}

const bool ATreeGenActor::IsIntersectingBranch(const FTreeBranchInfos& InfoA, const FTreeBranchInfos& InfoB)
{

	USplineComponent* BranchA = NewObject<USplineComponent>();
	BranchA->SetSplineLocalPoints(InfoA.Points);

	USplineComponent* BranchB = NewObject<USplineComponent>();
	BranchB->SetSplineLocalPoints(InfoB.Points);

	for (float DistanceA = 0; DistanceA < BranchA->GetSplineLength(); DistanceA += InfoA.BaseRadius) {

		const FSphere SphereA(BranchA->GetLocationAtDistanceAlongSpline(DistanceA, ESplineCoordinateSpace::Local), InfoA.BaseRadius);

		for (float DistanceB = 0; DistanceB < BranchB->GetSplineLength(); DistanceB += InfoB.BaseRadius) {

			const FSphere SphereB(BranchB->GetLocationAtDistanceAlongSpline(DistanceB, ESplineCoordinateSpace::Local), InfoB.BaseRadius);

			if (SphereA.Intersects(SphereB)) {

				BranchA->DestroyComponent();
				BranchB->DestroyComponent();
				return true;
			}
		}
	}
	BranchA->DestroyComponent();
	BranchB->DestroyComponent();
	return false;
}

TArray<FMeshDescription> ATreeGenActor::CreateMeshDescriptions(const TArray<FTransform>& InLeafTransforms)
{
	TArray<FMeshDescription> Descriptions;

	for (int LOD = 0; LOD < LevelsLOD.Num(); LOD++) {	//Create a Mesh Description for every LOD

		Descriptions.Add(FMeshDescription());
		FStaticMeshAttributes(Descriptions.Last()).Register();
		Descriptions.Last().CreatePolygonGroup();
		Descriptions.Last().CreatePolygonGroup();

		int Index = 0;

		TVertexAttributesRef<FVector3f> Locations = Descriptions.Last().GetVertexPositions();
		TVertexInstanceAttributesRef<FVector3f> Normals = FStaticMeshAttributes(Descriptions.Last()).GetVertexInstanceNormals();
		TVertexInstanceAttributesRef<FVector2f> UVs = FStaticMeshAttributes(Descriptions.Last()).GetVertexInstanceUVs();
		TVertexInstanceAttributesRef<FVector4f> Colors = FStaticMeshAttributes(Descriptions.Last()).GetVertexInstanceColors();

		for (const TArray<FTreeBranchInfos>& Infos : Branches) {

			for (const FTreeBranchInfos& Info : Infos) {

				Spline->SetSplineLocalPoints(Info.Points);

				const int NumberVertexCircles = FMath::CeilToInt(Spline->GetSplineLength() / (Spline->GetSplineLength() * LevelsLOD[LOD]));

				for (int x = 0; x < NumberVertexCircles; x++) {

					const float Distance = float(x) / float(NumberVertexCircles - 1) * Spline->GetSplineLength();
					const float Radius = FMath::Max(1, (1.0 - FMath::Pow(Distance / Spline->GetSplineLength(), 2) )* Info.BaseRadius);
					const int NumberVertices = FMath::Max(3, FMath::CeilToInt((PI * 2 * Radius) / (PI * 2 * Radius * LevelsLOD[LOD])));

					for (int y = 0; y < NumberVertices; y++) {

						Descriptions.Last().CreateVertexInstance(Descriptions.Last().CreateVertex());
					}
				}

				for (int x = 0; x < NumberVertexCircles; x++) {

					const float Distance = float(x) / float(NumberVertexCircles - 1) * Spline->GetSplineLength();
					const float Radius = FMath::Max(1, (1.0 - FMath::Pow(Distance / Spline->GetSplineLength(), 2)) * Info.BaseRadius);
					const int NumberVertices = FMath::Max(3, FMath::CeilToInt((PI * 2 * Radius) / (PI * 2 * Radius * LevelsLOD[LOD])));

					const FVector Center = Spline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::Local);
					const FVector Direction = Spline->GetDirectionAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::Local);
					const FRotator Rotation = Spline->GetRotationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::Local);
					const FVector UpVector = Spline->GetUpVectorAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::Local).
						RotateAngleAxis(-Rotation.Yaw + Rotation.Pitch + Rotation.Roll, Direction);

					for (int y = 0; y < NumberVertices; y++) {

						//Rotate clockwise around the center location
						const FVector Normal = UpVector.RotateAngleAxis(float(y) / float(NumberVertices - 1) * 360, Direction);
						const FVector Location = x < NumberVertices - 1 ? Center + Normal * (Radius) : Center;
						const FVector2D UV = FVector2D(float(y) / float(NumberVertices - 1), float(x) / float(NumberVertexCircles - 1));

						Locations[Index] = FVector3f(Location);
						Normals[Index] = FVector3f(Normal);
						UVs[Index] = FVector2f(UV);
						Colors[Index] = FLinearColor::Black;

						if (x < NumberVertexCircles - 1 && y < NumberVertices - 1) {

							const float NextDistance = float(x + 1) / float(NumberVertexCircles - 1) * Spline->GetSplineLength();
							const float NextRadius = FMath::Max(1, (1.0 - FMath::Pow(NextDistance / Spline->GetSplineLength(), 2)) * Info.BaseRadius);
							const int NumberNextVertices = FMath::Max(3, FMath::CeilToInt((PI * 2 * NextRadius) / (PI * 2 * NextRadius * LevelsLOD[LOD])));

							int Delta = NumberVertices - NumberNextVertices;


							int T0 = Index;
							int T1 = Index + 1;
							int T2 = Index + NumberVertices - FMath::FloorToInt(float(y) / float(NumberVertices - 1) * float(Delta));

							Descriptions.Last().CreateTriangle(0, { T0,T2,T1 });

							if (x > 0) {

								const float PreviousDistance = float(x - 1) / float(NumberVertexCircles - 1) * Spline->GetSplineLength();
								const float PreviousRadius = FMath::Max(1, (1.0 - FMath::Pow(PreviousDistance / Spline->GetSplineLength(), 2)) * Info.BaseRadius);
								const int NumberPreviousVertices = FMath::Max(3, FMath::CeilToInt((PI * 2 * PreviousRadius) / (PI * 2 * PreviousRadius * LevelsLOD[LOD])));

								Delta = NumberPreviousVertices - NumberVertices;

								T0 = Index + 1;
								T1 = Index;
								T2 = Index - NumberPreviousVertices + 1 + FMath::FloorToInt(float(y) / float(NumberVertices - 1) * float(Delta));

								Descriptions.Last().CreateTriangle(0, { T0,T2,T1 });
							}
						}
						Index++;
					}
				}
				if (!Leafs.IsEmpty()) {
					AInstancedFoliageActor* FoliageActor = Cast<AInstancedFoliageActor>(UGameplayStatics::GetActorOfClass(GetWorld(), AInstancedFoliageActor::StaticClass()));

					if (!FoliageActor) FoliageActor = GetWorld()->SpawnActor<AInstancedFoliageActor>();

					if (LeafsInstanceLocations.IsEmpty()) {

						for (UFoliageType_InstancedStaticMesh* Foliage : Leafs) {

							FoliageActor->FindOrAddMesh(Foliage);
							LeafsInstanceLocations.Add(Foliage);
						}
					}

					float Distance = Info.BaseRadius;

					while (Distance < Spline->GetSplineLength() * .9) {

						const FVector Center = Spline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::Local);
						const FVector UpVector = Spline->GetUpVectorAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::Local);
						const FVector Direction = Spline->GetDirectionAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::Local);

						const float Radius = FMath::Max(1, (1.0 - FMath::Pow(Distance / Spline->GetSplineLength(), 2)) * Info.BaseRadius);
						const float Angle = UKismetMathLibrary::RandomFloatFromStream(Stream) * 360.0f;

						const float Scale = UKismetMathLibrary::RandomFloatFromStream(Stream) * .4 * (1.0 - (Distance / Spline->GetSplineLength())) + .8;

						const FVector Location = Center + UpVector.RotateAngleAxis(Angle, Direction) * Radius * Scale;
						const FRotator Rotation = UpVector.RotateAngleAxis(Angle, Direction).Rotation();


						const FTransform Transform = FTransform(Rotation, GetActorLocation() + Location, FVector(Scale));

						FFoliageInstance Instance;
						Instance.SetInstanceWorldTransform(Transform);

						const int LeafIndex = UKismetMathLibrary::RandomIntegerFromStream(Stream, Leafs.Num() - 1);

						FFoliageInfo* FoliageInfo = FoliageActor->FindInfo(Leafs[LeafIndex]);

						FoliageInfo->AddInstance(Leafs[LeafIndex], Instance);
						FoliageInfo->Refresh(true, false);

						LeafsInstanceLocations.FindOrAdd(Leafs[LeafIndex]).Add(Transform);
						Distance += FMath::Lerp(DistanceBetweenLeafs, DistanceBetweenLeafs * .5, Distance / Spline->GetSplineLength());

					}

				}
			}
		}

		Descriptions.Last().ComputeBoundingBox();
		if (Descriptions.Last().NeedsCompact())UE_LOG(LogTemp, Display, TEXT("Description no.%i needCompact"), LOD);
	}
	return Descriptions;
}
UStaticMesh* ATreeGenActor::CreateRuntimeMesh(TArray<FMeshDescription> Descriptions)
{
	TArray<const FMeshDescription*> Descr;

	for (FMeshDescription& Description : Descriptions) {
		Descr.Add(&Description);
	}

	UStaticMesh* Mesh = NewObject<UStaticMesh>(this);
	Mesh->bDoFastBuild = true;
	Mesh->AddMaterial(M_TrunkMaterial);
	Mesh->AddMaterial(Leafs.Num() > 0 ? Cast<UFoliageType_InstancedStaticMesh>(Leafs[0])->GetStaticMesh()->GetMaterial(0) : nullptr);

	Mesh->Modify();

	Mesh->BuildFromMeshDescriptions(Descr, UStaticMesh::FBuildMeshDescriptionsParams::FBuildMeshDescriptionsParams());

	return Mesh;

}
void ATreeGenActor::ConvertToStaticMesh()
{
	TArray<int> LeafIndexes;
	for (const TPair< UFoliageType*, TArray<FTransform>>& Pair : LeafsInstanceLocations) {
		for (const FTransform& Transform : Pair.Value) {
			LeafIndexes.Add(UKismetMathLibrary::RandomIntegerFromStream(Stream, Leafs.Num() - 1));
		}
	}
	UStaticMesh* Mesh = GetStaticMeshComponent()->GetStaticMesh();

	TArray<FMeshDescription>Descriptions;

	for (int i = 0; i < Mesh->GetNumLODs(); i++) {

		Descriptions.Add(FMeshDescription());
		Mesh->CloneMeshDescription(i, Descriptions.Last());

		TVertexAttributesRef<FVector3f> Locations = Descriptions.Last().GetVertexPositions();
		TVertexInstanceAttributesRef<FVector3f> Normals = FStaticMeshAttributes(Descriptions.Last()).GetVertexInstanceNormals();
		TVertexInstanceAttributesRef<FVector2f> UVs = FStaticMeshAttributes(Descriptions.Last()).GetVertexInstanceUVs();
		TVertexInstanceAttributesRef<FVector4f> Colors = FStaticMeshAttributes(Descriptions.Last()).GetVertexInstanceColors();

		int Index = 0;

		for (const TPair< UFoliageType*, TArray<FTransform>>& Pair : LeafsInstanceLocations) {

			for (const FTransform& Transform : Pair.Value) {

				const int LeafIndex = LeafIndexes[Index];

				if (UFoliageType_InstancedStaticMesh* Foliage = Cast<UFoliageType_InstancedStaticMesh>(Leafs[LeafIndex])) {

					FMeshDescription* LeafDescription = nullptr;
					UStaticMesh* LeafMesh = Foliage->GetStaticMesh();

					if (LeafMesh->IsSourceModelValid(i)) {
						FStaticMeshSourceModel& Model = LeafMesh->GetSourceModel(FMath::Min(i, Foliage->GetStaticMesh()->GetNumLODs() - 1));
						if (Model.IsSourceModelInitialized()) {
							if (!Model.IsMeshDescriptionValid()) {
								if (Model.IsRawMeshEmpty()) {
									UE_LOG(LogTemp, Display, TEXT("RawMesh is Empty.. If you want more Leaf LODs you need to import LODs on that mesh"));
								}
							}
							LeafDescription = Model.GetOrCacheMeshDescription();
						}
					}
					else {
						UE_LOG(LogTemp, Display, TEXT("SourceModel invalid - try to reimport leafs"));
					}

					if (LeafDescription) {

						TVertexAttributesRef<FVector3f> LeafLocations = LeafDescription->GetVertexPositions();
						TVertexInstanceAttributesRef<FVector3f> LeafNormals = FStaticMeshAttributes(*LeafDescription).GetVertexInstanceNormals();
						TVertexInstanceAttributesRef<FVector2f> LeafUVs = FStaticMeshAttributes(*LeafDescription).GetVertexInstanceUVs();
						TVertexInstanceAttributesRef<FVector4f> LeafColors = FStaticMeshAttributes(*LeafDescription).GetVertexInstanceColors();

						TMap<FVertexID, FVertexID> VertexMap;
						TMap<FVertexInstanceID, FVertexInstanceID> InstanceMap;

						for (const FVertexID SourceVertexID : LeafDescription->Vertices().GetElementIDs())
						{
							const FVertexID DestinationVertexID = Descriptions.Last().CreateVertex();

							VertexMap.Add(SourceVertexID, DestinationVertexID);

							FVector Location = ((FVector)LeafLocations[SourceVertexID]) * Transform.GetScale3D();

							Location = Location
								.RotateAngleAxis(Transform.Rotator().Roll, FVector::XAxisVector)
								.RotateAngleAxis(Transform.Rotator().Pitch, FVector::YAxisVector)
								.RotateAngleAxis(Transform.Rotator().Yaw, FVector::ZAxisVector);

							Location += (Transform.GetLocation() - GetActorLocation());

							Locations[DestinationVertexID] = FVector3f(Location);

							for (const FVertexInstanceID SourceInstanceID : LeafDescription->GetVertexVertexInstances(SourceVertexID))
							{
								const FVertexInstanceID DestinationInstanceID =
									Descriptions.Last().CreateVertexInstance(DestinationVertexID);

								InstanceMap.Add(SourceInstanceID, DestinationInstanceID);

								Normals[DestinationInstanceID] = LeafNormals[SourceInstanceID];
								UVs[DestinationInstanceID] = LeafUVs[SourceInstanceID];
								Colors[DestinationInstanceID] = LeafColors[SourceInstanceID];
							}
						}

						for (const FTriangleID TriangleID : LeafDescription->Triangles().GetElementIDs())
						{
							const TArrayView<const FVertexInstanceID> Instances =
								LeafDescription->GetTriangleVertexInstances(TriangleID);

							Descriptions.Last().CreateTriangle(1, { InstanceMap[Instances[0]],	InstanceMap[Instances[1]],	InstanceMap[Instances[2]] });
						}
						Index++;
					}
				}
			}
		}
	}
	TArray<const FMeshDescription*>Descr;
	for (FMeshDescription& D : Descriptions) {
		Descr.Add(&D);
	}

	const FString Name = "TreeMesh";
	const FString PackageName = "/Game/" + Name;

	UPackage* Package = CreatePackage(*PackageName);

	UStaticMesh* NewMesh = NewObject<UStaticMesh>(Package, *Name, RF_Public | RF_Standalone);

	NewMesh->AddMaterial(M_TrunkMaterial);
	NewMesh->AddMaterial(Leafs.Num() > 0 ? Cast<UFoliageType_InstancedStaticMesh>(Leafs[0])->GetStaticMesh()->GetMaterial(0) : nullptr);
	NewMesh->bDoFastBuild = true;

	NewMesh->BuildFromMeshDescriptions(Descr, UStaticMesh::FBuildMeshDescriptionsParams::FBuildMeshDescriptionsParams());

	FAssetRegistryModule::AssetCreated(NewMesh);

}

void ATreeGenActor::PostEditChangeProperty(FPropertyChangedEvent& InEvent)
{
	UE_LOG(LogTemp, Warning, TEXT("ATreeGenActor PostEditChangeProperty"));

	Super::PostEditChangeProperty(InEvent);

	if (InEvent.GetPropertyName() == "ShowDepth" && Branches.IsValidIndex(ShowDepth) && !Branches[ShowDepth].IsEmpty()) {

		for (USplineComponent* Branch : BranchComponents) {
			if (Branch)Branch->DestroyComponent();
		}

		BranchComponents.Empty();

		for (const FTreeBranchInfos& Info : Branches[ShowDepth]) {

			BranchComponents.Add(NewObject<USplineComponent>(this));
			BranchComponents.Last()->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
			BranchComponents.Last()->RegisterComponent();
			BranchComponents.Last()->SetSplineLocalPoints(Info.Points);

		}

	}


}


