#include "ProceduralTerrainWorld.h"
#include "ProceduralTerrain.h"
#include "DrawDebugHelpers.h"
#include "Async/Async.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

//────────────────────────────
// Constructor
//────────────────────────────

AProceduralTerrainWorld::AProceduralTerrainWorld()
{
	PrimaryActorTick.bCanEverTick = true; // Enables Tick() for generation progress and debug display.
}

//────────────────────────────
// Editor Construction (OnConstruction)
//────────────────────────────

void AProceduralTerrainWorld::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (bIsGenerating)
	{
		UE_LOG(LogTemp, Warning, TEXT("Generation already in progress, skipping OnConstruction."));
		return;
	}

	// Cleanup previous chunks
	for (UProceduralTerrain* OldChunk : Chunks)
		if (OldChunk)
			OldChunk->DestroyComponent();
	Chunks.Empty();

	bIsGenerating = true;
	CompletedChunks = 0;

	const FString SaveDir = TEXT("TerrainChunks");
	IFileManager::Get().MakeDirectory(*SaveDir, true);

	//────────────────────────────
	// Chunk Grid Creation
	//────────────────────────────

	for (int32 z = 0; z < ChunksZ; ++z)
	for (int32 y = 0; y < ChunksY; ++y)
	for (int32 x = 0; x < ChunksX; ++x)
	{
		FString Name = FString::Printf(TEXT("Chunk_%d_%d_%d"), x, y, z);
		UProceduralTerrain* Chunk = NewObject<UProceduralTerrain>(this, *Name);
		Chunk->RegisterComponent();
		Chunk->AttachToComponent(GetRootComponent() ? GetRootComponent() : nullptr,
			FAttachmentTransformRules::KeepRelativeTransform);

		const FVector Offset = FVector(x, y, z) * (ChunkSize - 1) * TerrainScale;
		Chunk->SetRelativeLocation(Offset);
		Chunk->ChunkCoords = FIntVector(x, y, z);

		Chunks.Add(Chunk);
		UE_LOG(LogTemp, Log, TEXT("Created chunk (%d,%d,%d) at %s"), x, y, z, *Offset.ToString());
	}

	//────────────────────────────
	// Load Existing or Generate New Terrain
	//────────────────────────────

	bool bLoadedExisting = false;
	for (UProceduralTerrain* Chunk : Chunks)
	{
		const FString ChunkFile = FPaths::Combine(SaveDir, FString::Printf(TEXT("TerrainChunks_%s.json"), *Chunk->GetName()));
		const FString FullPath = FPaths::ProjectSavedDir() / ChunkFile;

		if (FPaths::FileExists(FullPath))
		{
			Chunk->LoadDensityFromJSON(ChunkFile);
			Chunk->RebuildMeshFromCurrentDensity();
			bLoadedExisting = true;
		}
	}

	if (bLoadedExisting)
	{
		UE_LOG(LogTemp, Log, TEXT("Loaded existing terrain chunks from disk."));
		bIsGenerating = false;
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("No saved chunks found — generating new terrain grid."));
		GenerateAllChunks();
	}
}

//────────────────────────────
// Asynchronous Chunk Generation
//────────────────────────────

void AProceduralTerrainWorld::GenerateAllChunks()
{
	UE_LOG(LogTemp, Log, TEXT("Starting asynchronous terrain generation (%d chunks)."), Chunks.Num());
	const double StartTime = FPlatformTime::Seconds();

	for (UProceduralTerrain* Chunk : Chunks)
	{
		if (!Chunk) continue;

		Async(EAsyncExecution::ThreadPool, [this, Chunk, StartTime]()
		{
			// Worker thread: compute density field
			Chunk->BuildDensityField(ChunkSize, TerrainScale, NoiseScale, HeightBias, NoiseStrength);

			// Return to game thread to build the mesh
			AsyncTask(ENamedThreads::GameThread, [this, Chunk, StartTime]()
			{
				Chunk->RebuildMeshFromCurrentDensity();
				CompletedChunks++;

				UE_LOG(LogTemp, Log, TEXT("%s completed (%.2fs elapsed)"),
					*Chunk->GetName(), FPlatformTime::Seconds() - StartTime);

				if (CompletedChunks >= Chunks.Num())
				{
					bIsGenerating = false;
					UE_LOG(LogTemp, Log, TEXT("All chunks generated successfully."));
				}
			});
		});
	}
}

//────────────────────────────
// Tick (Progress Display + Debug Bounds)
//────────────────────────────

void AProceduralTerrainWorld::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Show async generation progress
	if (bIsGenerating && Chunks.Num() > 0)
	{
		const float Progress = static_cast<float>(CompletedChunks) / static_cast<float>(Chunks.Num());
		const FVector Pos = GetActorLocation() + FVector(0, 0, 300);

		const FString Text = FString::Printf(TEXT("Generating terrain: %d/%d chunks (%.0f%%)"),
			CompletedChunks, Chunks.Num(), Progress * 100.0f);

		DrawDebugString(GetWorld(), Pos, Text, nullptr, FColor::Yellow, 0.0f, true);
	}

	// Draw chunk bounding boxes if enabled
	if (!bShowChunkBounds)
		return;

	UWorld* World = GetWorld();
	if (!World)
		return;

	const float ChunkWorldSize = (ChunkSize - 1) * TerrainScale;
	const FVector HalfExtent(ChunkWorldSize * 0.5f);

	for (UProceduralTerrain* Chunk : Chunks)
	{
		if (!Chunk) continue;

		const FVector Origin = Chunk->GetComponentLocation();
		const FVector Center = Origin + HalfExtent;

		DrawDebugBox(
			World,
			Center,
			HalfExtent,
			ChunkBoundsColor,
			false, // bPersistentLines
			0.0f,  // Duration
			0,
			ChunkBoundsThickness
		);
	}
}

//────────────────────────────
// BeginPlay (Runtime Initialization)
//────────────────────────────

void AProceduralTerrainWorld::BeginPlay()
{
	Super::BeginPlay();

	PersistentChunks.Reset();

	GetWorldTimerManager().SetTimer(
		StreamingTimer,
		this,
		&AProceduralTerrainWorld::UpdateStreamedChunks,
		UpdateInterval,
		true
	);

	UE_LOG(LogTemp, Log, TEXT("Chunk streaming system initialized."));

	const FString SaveDir = TEXT("TerrainChunks");
	IFileManager::Get().MakeDirectory(*SaveDir, true);

	for (UProceduralTerrain* Chunk : Chunks)
	{
		const FString ChunkFile = FPaths::Combine(SaveDir, FString::Printf(TEXT("TerrainChunks_%s.json"), *Chunk->GetName()));
		const FString FullPath = FPaths::ProjectSavedDir() / ChunkFile;

		if (FPaths::FileExists(FullPath))
		{
			Chunk->LoadDensityFromJSON(ChunkFile);
		}
		else
		{
			GenerateAllChunks();
			break; // Avoid regenerating multiple times
		}
	}

	for (UProceduralTerrain* Chunk : Chunks)
		PersistentChunks.Add(Chunk);
}

//────────────────────────────
// EndPlay (Save Persistent Chunks)
//────────────────────────────

void AProceduralTerrainWorld::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	const FString SaveDir = TEXT("TerrainChunks");
	IFileManager::Get().MakeDirectory(*SaveDir, true);

	for (UProceduralTerrain* Chunk : PersistentChunks)
	{
		const FString ChunkFile = FPaths::Combine(SaveDir, FString::Printf(TEXT("TerrainChunks_%s.json"), *Chunk->GetName()));
		Chunk->SaveDensityToJSON(ChunkFile);
	}

	UE_LOG(LogTemp, Log, TEXT("All persistent chunks saved on EndPlay."));
	Super::EndPlay(EndPlayReason);
}

//────────────────────────────
// Runtime Digging
//────────────────────────────

void AProceduralTerrainWorld::DigAt(FVector WorldPosition, float Radius, float Strength)
{
	UE_LOG(LogTemp, Log, TEXT("Dig operation at %s (Radius=%.1f, Strength=%.1f)"),
		*WorldPosition.ToString(), Radius, Strength);

	const FSphere DigSphere(WorldPosition, Radius);
	bool bAnyAffected = false;

	for (UProceduralTerrain* Chunk : Chunks)
	{
		if (!Chunk || Chunk->Density.Num() == 0)
			continue;

		const FVector ChunkMin = Chunk->GetComponentLocation();
		const FVector ChunkMax = ChunkMin + FVector(ChunkSize - 1) * TerrainScale;
		const FBox ChunkBox(ChunkMin, ChunkMax);

		if (!FMath::SphereAABBIntersection(DigSphere, ChunkBox))
			continue;

		// Convert world position to local voxel coordinates
		const FVector LocalPosUnits = WorldPosition - Chunk->GetComponentLocation();
		const FVector LocalVoxelPos = LocalPosUnits / TerrainScale;

		const int32 XCenter = FMath::RoundToInt(LocalVoxelPos.X);
		const int32 YCenter = FMath::RoundToInt(LocalVoxelPos.Y);
		const int32 ZCenter = FMath::RoundToInt(LocalVoxelPos.Z);
		const int32 RadiusVoxels = FMath::CeilToInt(Radius / TerrainScale);

		auto GetIndex = [&](int32 X, int32 Y, int32 Z)
		{
			return X + Y * ChunkSize + Z * ChunkSize * ChunkSize;
		};

		int32 Modified = 0;
		for (int32 z = ZCenter - RadiusVoxels; z <= ZCenter + RadiusVoxels; z++)
		for (int32 y = YCenter - RadiusVoxels; y <= YCenter + RadiusVoxels; y++)
		for (int32 x = XCenter - RadiusVoxels; x <= XCenter + RadiusVoxels; x++)
		{
			if (x < 0 || y < 0 || z < 0 || x >= ChunkSize || y >= ChunkSize || z >= ChunkSize)
				continue;

			const FVector VoxelPos(x, y, z);
			const float Dist = FVector::Dist(VoxelPos, FVector(XCenter, YCenter, ZCenter));

			if (Dist <= RadiusVoxels)
			{
				const int32 Idx = GetIndex(x, y, z);
				Chunk->Density[Idx] += Strength * (1.0f - (Dist / RadiusVoxels));
				++Modified;
			}
		}

		Chunk->RebuildMeshFromCurrentDensity();
		UE_LOG(LogTemp, Log, TEXT("Modified %d voxels in %s"), Modified, *Chunk->GetName());
		bAnyAffected = true;
	}

	if (!bAnyAffected)
		UE_LOG(LogTemp, Warning, TEXT("No chunks were affected by the dig operation."));
}

//────────────────────────────
// Manual Refresh
//────────────────────────────

void AProceduralTerrainWorld::RefreshTerrain()
{
	const FString SaveDir = TEXT("TerrainChunks");

	for (UProceduralTerrain* Chunk : Chunks)
	{
		const FString ChunkFile = FPaths::Combine(SaveDir, FString::Printf(TEXT("TerrainChunks_%s.json"), *Chunk->GetName()));
		const FString FullPath = FPaths::ProjectSavedDir() / ChunkFile;

		if (FPaths::FileExists(FullPath))
		{
			Chunk->LoadDensityFromJSON(ChunkFile);
			UE_LOG(LogTemp, Log, TEXT("Reloaded chunk from JSON: %s"), *Chunk->GetName());
		}
	}

	UE_LOG(LogTemp, Log, TEXT("All chunks manually refreshed in editor."));
}

//────────────────────────────
// Chunk Streaming (Dynamic Loading / Unloading)
//────────────────────────────

void AProceduralTerrainWorld::UpdateStreamedChunks()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC) return;

	APawn* Pawn = PC->GetPawn();
	if (!Pawn) return;

	const FVector PlayerPos = Pawn->GetActorLocation();
	const FVector ChunkWorldSize = FVector(ChunkSize - 1) * TerrainScale;
	const FIntVector PlayerChunkCoords(
		FMath::FloorToInt(PlayerPos.X / ChunkWorldSize.X),
		FMath::FloorToInt(PlayerPos.Y / ChunkWorldSize.Y),
		0
	);

	// Skip if player is still within the same central chunk
	const FVector PlayerChunkCenter = FVector(PlayerChunkCoords) * ChunkWorldSize;
	if (FVector::Dist(PlayerChunkCenter, LastPlayerChunkCenter) < ChunkWorldSize.X * 0.5f)
		return;

	LastPlayerChunkCenter = PlayerChunkCenter;
	UE_LOG(LogTemp, Log, TEXT("Player moved to chunk (%d, %d)."), PlayerChunkCoords.X, PlayerChunkCoords.Y);

	// Determine which chunks should be loaded
	TSet<FIntVector> DesiredChunks;
	for (int32 dx = -StreamRadius; dx <= StreamRadius; ++dx)
	for (int32 dy = -StreamRadius; dy <= StreamRadius; ++dy)
		DesiredChunks.Add(FIntVector(PlayerChunkCoords.X + dx, PlayerChunkCoords.Y + dy, 0));

	// Remove distant chunks
	for (int32 i = Chunks.Num() - 1; i >= 0; --i)
	{
		UProceduralTerrain* Chunk = Chunks[i];
		if (!Chunk) continue;

		if (!DesiredChunks.Contains(Chunk->ChunkCoords))
		{
			// Skip persistent (initial) chunks
			if (PersistentChunks.Contains(Chunk))
				continue;

			UE_LOG(LogTemp, Log, TEXT("Removing distant chunk: %s"), *Chunk->GetName());
			Chunk->DestroyComponent();
			Chunks.RemoveAt(i);
		}
	}

	// Create missing chunks
	for (const FIntVector& Coords : DesiredChunks)
	{
		bool bExists = false;
		for (UProceduralTerrain* Existing : Chunks)
		{
			if (Existing && Existing->ChunkCoords == Coords)
			{
				bExists = true;
				break;
			}
		}

		if (!bExists)
		{
			FString Name = FString::Printf(TEXT("Chunk_%d_%d_%d"), Coords.X, Coords.Y, Coords.Z);
			UE_LOG(LogTemp, Log, TEXT("Creating streamed chunk: %s"), *Name);

			UProceduralTerrain* NewChunk = NewObject<UProceduralTerrain>(this, *Name);
			NewChunk->RegisterComponent();
			NewChunk->AttachToComponent(GetRootComponent() ? GetRootComponent() : nullptr,
				FAttachmentTransformRules::KeepRelativeTransform);

			const FVector Offset = FVector(Coords) * (ChunkSize - 1) * TerrainScale;
			NewChunk->SetRelativeLocation(Offset);
			NewChunk->ChunkCoords = Coords;

			const FString ChunkFile = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("TerrainChunks"), FString::Printf(TEXT("TerrainChunks_%s.json"), *NewChunk->GetName()));
			if (FPaths::FileExists(ChunkFile))
			{
				NewChunk->LoadDensityFromJSON(ChunkFile);
				NewChunk->RebuildMeshFromCurrentDensity();
			}
			else
			{
				NewChunk->BuildDensityField(ChunkSize, TerrainScale, NoiseScale, HeightBias, NoiseStrength);
				NewChunk->RebuildMeshFromCurrentDensity();
			}

			Chunks.Add(NewChunk);
		}
	}
}