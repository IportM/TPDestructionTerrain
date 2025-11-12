#include "ProceduralTerrainActor.h"
#include "EngineUtils.h"
#include "ProceduralTerrain.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"

//────────────────────────────
// Constructor
//────────────────────────────

AProceduralTerrainActor::AProceduralTerrainActor()
{
	// Enable ticking if needed (can be disabled to improve performance).
	PrimaryActorTick.bCanEverTick = true;

	// Create and attach the procedural terrain component as the root.
	ProceduralTerrain = CreateDefaultSubobject<UProceduralTerrain>(TEXT("ProceduralTerrain"));
	SetRootComponent(ProceduralTerrain);
}

//────────────────────────────
// Construction (Editor-Time)
//────────────────────────────

void AProceduralTerrainActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (!ProceduralTerrain)
		return;

	const FString FileName = TEXT("TerrainDensity.json");
	const FString SavePath = FPaths::ProjectSavedDir() / FileName;

	// If a previously saved terrain exists, load it in editor
	if (FPaths::FileExists(SavePath))
	{
		ProceduralTerrain->LoadDensityFromJSON(FileName);
		UE_LOG(LogTemp, Log, TEXT("Loaded saved terrain data (Editor View)."));
	}
	else
	{
		// Otherwise, create a new default terrain
		ProceduralTerrain->CreateProceduralTerrain3D(TerrainSize, TerrainScale, NoiseScale, HeightBias, NoiseStrength, IsoLevel);
		UE_LOG(LogTemp, Log, TEXT("Created new procedural terrain (Editor View)."));
	}
}

//────────────────────────────
// BeginPlay (Runtime Initialization)
//────────────────────────────

void AProceduralTerrainActor::BeginPlay()
{
	Super::BeginPlay();

	if (!ProceduralTerrain)
		return;

	const FString FileName = TEXT("TerrainDensity.json");
	const FString SavePath = FPaths::ProjectSavedDir() / FileName;

	// Try to load terrain from disk; if not found, create a new one
	if (FPaths::FileExists(SavePath))
	{
		ProceduralTerrain->LoadDensityFromJSON(FileName);
		UE_LOG(LogTemp, Log, TEXT("Loaded terrain from JSON at BeginPlay."));
	}
	else
	{
		ProceduralTerrain->CreateProceduralTerrain3D(TerrainSize, TerrainScale, NoiseScale, HeightBias, NoiseStrength, IsoLevel);
		UE_LOG(LogTemp, Log, TEXT("Generated new terrain at BeginPlay."));
	}
}

//────────────────────────────
// EndPlay (Runtime Cleanup)
//────────────────────────────

void AProceduralTerrainActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ProceduralTerrain)
	{
		ProceduralTerrain->SaveDensityToJSON(TEXT("TerrainDensity.json"));
		UE_LOG(LogTemp, Log, TEXT("Saved terrain data on EndPlay."));
	}

	Super::EndPlay(EndPlayReason);
}

//────────────────────────────
// Editor Utility
//────────────────────────────

void AProceduralTerrainActor::RefreshInEditor()
{
	if (ProceduralTerrain)
	{
		ProceduralTerrain->RefreshTerrain();
		UE_LOG(LogTemp, Log, TEXT("Manual terrain refresh triggered in editor."));
	}
}