#pragma once

#include "CoreMinimal.h"
#include "ProceduralTerrain.h"
#include "GameFramework/Actor.h"
#include "ProceduralTerrainActor.generated.h"

/**
 * AProceduralTerrainActor
 *
 * An actor that manages a UProceduralTerrain component, exposing parameters
 * for terrain generation and auto-building when placed or modified in the editor.
 */
UCLASS()
class DESTRUCTIONTERRAIN_API AProceduralTerrainActor : public AActor
{
	GENERATED_BODY()

public:
	/** Default constructor */
	AProceduralTerrainActor();

protected:

	//────────────────────────────
	// Components
	//────────────────────────────

	/** The procedural terrain component responsible for mesh generation and manipulation. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UProceduralTerrain* ProceduralTerrain;

	//────────────────────────────
	// Editable Terrain Settings
	//────────────────────────────

	/** Number of voxels along one dimension of the terrain cube. */
	UPROPERTY(EditAnywhere, Category = "Terrain Settings")
	int32 TerrainSize = 32;

	/** World-space scale factor between voxels. */
	UPROPERTY(EditAnywhere, Category = "Terrain Settings")
	float TerrainScale = 50.0f;

	/** Frequency of the noise used for density generation. */
	UPROPERTY(EditAnywhere, Category = "Terrain Settings")
	float NoiseScale = 0.1f;

	/** Vertical bias applied to density values (controls terrain elevation). */
	UPROPERTY(EditAnywhere, Category = "Terrain Settings")
	float HeightBias = 20.0f;

	/** Amplitude of the noise modulation (controls roughness). */
	UPROPERTY(EditAnywhere, Category = "Terrain Settings")
	float NoiseStrength = 10.0f;

	/** Threshold value used for the Marching Cubes iso-surface extraction. */
	UPROPERTY(EditAnywhere, Category = "Terrain Settings")
	float IsoLevel = 0.0f;

public:

	//────────────────────────────
	// Actor Lifecycle
	//────────────────────────────

	/** Called every time the actor is (re)constructed in the editor or at runtime. */
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Called when the game starts or the actor is spawned. */
	virtual void BeginPlay() override;

	/** Called when the actor is destroyed or removed from the world. */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	//────────────────────────────
	// Editor Utility
	//────────────────────────────

	/** Forces a manual refresh of the procedural terrain in the editor. */
	void RefreshInEditor();
};