#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "ProceduralTerrain.generated.h"

/**
 * UProceduralTerrain
 * 
 * This component extends ProceduralMeshComponent to support voxel-based terrain generation,
 * destruction (digging), and data persistence via JSON serialization.
 * 
 * Typical usage:
 * - Create procedural terrain in editor or at runtime using CreateProceduralTerrain3D().
 * - Modify the terrain dynamically using DigSphere().
 * - Save and load terrain density data using SaveDensityToJSON() and LoadDensityFromJSON().
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent), editinlinenew, Within = Actor, DefaultToInstanced)
class DESTRUCTIONTERRAIN_API UProceduralTerrain : public UProceduralMeshComponent
{
	GENERATED_BODY()

protected:

	// ──────────────── INTERNAL DATA ────────────────
	// Stores the current resolution and parameters used for mesh generation.
	int32 CurrentSize = 0;
	float CurrentScale = 1.0f;
	float CurrentIsoLevel = 0.0f;

public:

	// Density field storing scalar values for voxel sampling (Marching Cubes, etc.)
	TArray<float> Density;

	// ──────────────── CORE MESH GENERATION ────────────────

	/** Rebuilds the procedural mesh from the current Density field. */
	void RebuildMeshFromCurrentDensity();

	/**
	 * Builds a new density field using procedural noise.
	 * @param Size - Number of voxels along each axis.
	 * @param Scale - Scale factor between voxel positions.
	 * @param NoiseScale - Frequency of noise generation.
	 * @param HeightBias - Vertical bias applied to the terrain.
	 * @param NoiseStrength - Amplitude of noise variation.
	 */
	void BuildDensityField(int32 Size, float Scale, float NoiseScale, float HeightBias, float NoiseStrength);

	/** Returns true if a given world position is inside this terrain chunk's bounds. */
	bool ContainsWorldPoint(const FVector& WorldPos, float Radius) const;

	// ──────────────── EDITOR / CHUNK PROPERTIES ────────────────

	/** Coordinates of this terrain chunk (for streaming / world generation). */
	UPROPERTY(EditAnywhere, Category = "Terrain|Chunk")
	FIntVector ChunkCoords = FIntVector::ZeroValue;

	// ──────────────── TERRAIN CREATION ────────────────

	/** Creates a simple 2D procedural terrain heightmap. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Terrain|Generation")
	void CreateProceduralTerrain(
		int32 Height = 100,
		int32 Width = 100,
		float NoiseScale = 0.05f,
		float MaxHeight = 300.0f,
		float Scale = 25.0f
	);

	/** Creates a fully 3D procedural terrain (voxel-based). */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Terrain|Generation")
	void CreateProceduralTerrain3D(
		int32 Size = 32,
		float Scale = 50.0f,
		float NoiseScale = 0.1f,
		float HeightBias = 15.0f,
		float NoiseStrength = 10.0f,
		float IsoLevel = 0.0f
	);

	// ──────────────── EDITOR UTILITIES ────────────────

	/** Clears all mesh sections and density data. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Terrain|Editor")
	void ClearMesh();

	/** Rebuilds the mesh and refreshes the terrain in the editor. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Terrain|Editor")
	void RefreshTerrain();

	// ──────────────── DESTRUCTION / MODIFICATION ────────────────

	/**
	 * Applies a spherical modification to the terrain (digging or adding material).
	 * @param WorldPosition - Center of the sphere in world space.
	 * @param Radius - Radius of the modification sphere.
	 * @param Strength - Signed strength of the change (negative = dig, positive = add).
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Terrain|Destruction")
	void DigSphere(FVector WorldPosition, float Radius, float Strength = -20.0f);

	// ──────────────── PERSISTENCE (SAVE / LOAD) ────────────────

	/** Saves the current density field to a JSON file. */
	UFUNCTION(BlueprintCallable, Category = "Terrain|Persistence")
	void SaveDensityToJSON(const FString& FileName);

	/** Loads density data from a JSON file and rebuilds the terrain mesh. */
	UFUNCTION(BlueprintCallable, Category = "Terrain|Persistence")
	void LoadDensityFromJSON(const FString& FileName);
};