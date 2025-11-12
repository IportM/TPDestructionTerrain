#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralTerrainWorld.generated.h"

class UProceduralTerrain;

/**
 * AProceduralTerrainWorld
 *
 * High-level actor that coordinates multiple procedural terrain chunks.
 * Handles asynchronous terrain generation, dynamic streaming around the player,
 * and automatic saving/loading of persistent chunks.
 */
UCLASS()
class DESTRUCTIONTERRAIN_API AProceduralTerrainWorld : public AActor
{
	GENERATED_BODY()

public:
	/** Default constructor */
	AProceduralTerrainWorld();

protected:

	//────────────────────────────
	// Unreal Lifecycle
	//────────────────────────────

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	//────────────────────────────
	// Terrain Configuration
	//────────────────────────────

	/** Number of voxels along one side of a chunk (resolution per chunk). */
	UPROPERTY(EditAnywhere, Category = "Terrain|Chunk Settings")
	int32 ChunkSize = 32;

	/** World-space scale factor for each voxel. */
	UPROPERTY(EditAnywhere, Category = "Terrain|Chunk Settings")
	float TerrainScale = 50.0f;

	/** Frequency of the procedural noise used for terrain generation. */
	UPROPERTY(EditAnywhere, Category = "Terrain|Noise Settings")
	float NoiseScale = 0.003f;

	/** Vertical bias applied to the generated terrain (controls height offset). */
	UPROPERTY(EditAnywhere, Category = "Terrain|Noise Settings")
	float HeightBias = 20.0f;

	/** Amplitude of the procedural noise (controls roughness). */
	UPROPERTY(EditAnywhere, Category = "Terrain|Noise Settings")
	float NoiseStrength = 3.0f;

	/** Iso-surface threshold used for Marching Cubes extraction. */
	UPROPERTY(EditAnywhere, Category = "Terrain|Chunk Settings")
	float IsoLevel = 0.0f;

	//────────────────────────────
	// Chunk Grid
	//────────────────────────────

	/** Number of chunks generated along the X axis. */
	UPROPERTY(EditAnywhere, Category = "Chunks|Grid Dimensions")
	int32 ChunksX = 5;

	/** Number of chunks generated along the Y axis. */
	UPROPERTY(EditAnywhere, Category = "Chunks|Grid Dimensions")
	int32 ChunksY = 5;

	/** Number of chunks generated along the Z axis (use 1 for flat terrain). */
	UPROPERTY(EditAnywhere, Category = "Chunks|Grid Dimensions")
	int32 ChunksZ = 1;

	/** All currently loaded terrain chunks. */
	UPROPERTY(VisibleAnywhere, Category = "Chunks|Runtime")
	TArray<UProceduralTerrain*> Chunks;

	//────────────────────────────
	// Debug Visualization
	//────────────────────────────

	/** Display debug bounding boxes around each chunk. */
	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bShowChunkBounds = true;

	/** Color of the debug chunk boxes. */
	UPROPERTY(EditAnywhere, Category = "Debug")
	FColor ChunkBoundsColor = FColor::Green;

	/** Line thickness of the debug boxes. */
	UPROPERTY(EditAnywhere, Category = "Debug")
	float ChunkBoundsThickness = 5.0f;

	//────────────────────────────
	// Terrain Streaming & Persistence
	//────────────────────────────

	/** Radius (in chunks) around the player to keep loaded. */
	UPROPERTY(EditAnywhere, Category = "Terrain|Streaming")
	int32 StreamRadius = 2;

	/** Time interval (seconds) between streaming updates. */
	UPROPERTY(EditAnywhere, Category = "Terrain|Streaming")
	float UpdateInterval = 0.1f;

	/** Persistent chunks that are never unloaded (initial grid). */
	UPROPERTY()
	TArray<UProceduralTerrain*> PersistentChunks;

	/** Center of the last player chunk (to detect movement between chunks). */
	FVector LastPlayerChunkCenter;

	/** Timer used to periodically update streamed chunks. */
	FTimerHandle StreamingTimer;

	//────────────────────────────
	// Internal State
	//────────────────────────────

	/** Prevents multiple generations running simultaneously. */
	bool bIsGenerating = false;

	/** Number of chunks that have completed asynchronous generation. */
	int32 CompletedChunks = 0;

	//────────────────────────────
	// Core Internal Functions
	//────────────────────────────

private:

	/** Asynchronously generates all chunks (called at first construction). */
	void GenerateAllChunks();

public:

	//────────────────────────────
	// Gameplay & Editor Functions
	//────────────────────────────

	/**
	 * Performs a spherical modification of the terrain at the specified world position.
	 * @param WorldPosition - Center of the dig sphere in world space.
	 * @param Radius - Radius of the operation in world units.
	 * @param Strength - Signed intensity of the modification (negative = dig).
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Terrain|Destruction")
	void DigAt(FVector WorldPosition, float Radius, float Strength);

	/** Reloads all saved chunks from JSON files (manual editor refresh). */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Terrain|Persistence")
	void RefreshTerrain();

	/** Updates which chunks are loaded or unloaded based on player position. */
	void UpdateStreamedChunks();
};