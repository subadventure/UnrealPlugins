// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "MeshDescription.h"
#include "Components/SplineComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Modules/ModuleManager.h"
#include "FoliageType_InstancedStaticMesh.h"
#include "TreeGen.generated.h"

class FTreeGenModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

struct FTreeBranchInfos {

	FTreeBranchInfos(const int InDepth, const int InIndex, const float InLength, const float InBaseRadius, const float InParentDistance, const TArray<FVector>& InPoints, const FBox& InBounds) : 
		Depth(InDepth), Index(InIndex), Length(InLength), BaseRadius(InBaseRadius), ParentDistance(InParentDistance), Points(InPoints), RealBounds(InBounds){}
	FTreeBranchInfos(){}

	int Depth = -1;
	int Index = -1;

	float Length = -1;
	float BaseRadius = -1;
	float ParentDistance = -1;

	FBox RealBounds;

	// TODO: Set the radius for each point for more realistic look

	TArray<float>Radii;
	TArray<FVector>Points;

};

UCLASS(Blueprintable)
class ATreeGenActor : public AStaticMeshActor {
	GENERATED_BODY()

public:
	ATreeGenActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree")
	FRandomStream Stream;

	UPROPERTY(EditAnywhere, Category = "Tree")
	float TreeHeight = 1000;

	// Height from where child branches can grow
	UPROPERTY(EditAnywhere, Category = "Tree")
	float BranchHeight = 300;

	// Maximum length percent from Parent Length; MaxBRanchLength=.7f = Parent = 100.0f Child = 70.0f
	UPROPERTY(EditAnywhere, Category = "Tree")
	float MaxBranchLength = .7f;

	// Maximum child branches for each branch
	UPROPERTY(EditAnywhere, Category = "Tree")
	int MaxNumberBranchChilds = 6;

	// In general it also says like how many spline points should be created / Length 
	UPROPERTY(EditAnywhere, Category = "Tree")
	float DistanceBetweenSplinePoints = 200;

	// Basically it is some kind of density value for Leafs
	UPROPERTY(EditAnywhere, Category = "Tree")
	float DistanceBetweenLeafs = 50;

	// Maximum Angle in wich the Direction can change
	UPROPERTY(EditAnywhere, Category = "Tree")
	float BranchConeAngle = 45;

	// How many BeanchChildren should we add
	UPROPERTY(EditAnywhere, Category = "Tree")
	int Depth = 3;

	// Debug Spline editing
	UPROPERTY(EditAnywhere, Category = "Tree")
	int ShowDepth = 0;

	// Specifies euch Level of Detail with a float, where 1 means lowest resolution and a lower number a higher resolution
	UPROPERTY(EditAnywhere, Category="Tree")
	TArray<float> LevelsLOD = { 0.05f,0.1f,0.2f,0.3f };

	// Material used for Trunk
	UPROPERTY(EditAnywhere, Category = "Tree")
	UMaterialInterface* M_TrunkMaterial;

	// All of theese will be used as Leafs;
	UPROPERTY(EditAnywhere, Category = "Tree")
	TArray<UFoliageType_InstancedStaticMesh*> Leafs;

	// for debug editing
	UPROPERTY(EditAnywhere, Category="Tree")
	TArray<USplineComponent*> BranchComponents;

	// Generating a new Tree
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Tree")
	void Generate();

	// Convert to an UStaticMesh Asset and save it to the project's content browser
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Tree")
	void ConvertToStaticMesh();

protected:

	virtual void PostEditChangeProperty(FPropertyChangedEvent& InEvent) override;

private:
	FBox RealBounds;
	USplineComponent* Spline;
	TArray<TArray<FTreeBranchInfos>> Branches;
	TMap<UFoliageType*, TArray<FTransform>> LeafsInstanceLocations;

	void CleanupPreviousBuild();

	void InitBranches();
	void AddBranches(FTreeBranchInfos& InParent, const int InDepth = 0);
	void AddBranchPoints(FTreeBranchInfos& InInfos);
	const bool IsIntersectingBranch(const FTreeBranchInfos& BranchA, const FTreeBranchInfos& BranchB);

	TArray< FMeshDescription> CreateMeshDescriptions(const TArray<FTransform>& InLeafTransforms = TArray<FTransform>());
	UStaticMesh* CreateRuntimeMesh(TArray<FMeshDescription> Descriptions);

};