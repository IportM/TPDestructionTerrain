// Fill out your copyright notice in the Description page of Project Settings.


#include "ProceduralTerrain.h"
#include "MarchingCubesTables.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Dom/JsonObject.h"


void UProceduralTerrain::CreateProceduralTerrain(int32 Height, int32 Width, float NoiseScale, float MaxHeight,
                                                 float Scale)
{
	// Tableaux de données du mesh
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;

	// --- Génération des sommets (positions) ---
	for (int32 y = 0; y < Height; y++)
	{
		for (int32 x = 0; x < Width; x++)
		{
			// Valeur de bruit : donne un relief "naturel"
			float NoiseValue = FMath::PerlinNoise2D(FVector2D(x, y) * NoiseScale);

			// Hauteur finale (Z)
			float Z = NoiseValue * MaxHeight;

			// Position 3D du point
			Vertices.Add(FVector(x * Scale, y * Scale, Z));
		}
	}

	// --- Génération des triangles (indices) ---
	for (int32 y = 0; y < Height - 1; y++)
	{
		for (int32 x = 0; x < Width - 1; x++)
		{
			int32 i = x + y * Width;

			// Chaque carré de la grille est formé de 2 triangles
			Triangles.Append({i, i + Width, i + Width + 1});
			Triangles.Append({i, i + Width + 1, i + 1});
		}
	}

	// --- Calcul des normales (pour l'éclairage) ---
	Normals.SetNum(Vertices.Num());

	for (int32 i = 0; i < Triangles.Num(); i += 3)
	{
		int32 i0 = Triangles[i];
		int32 i1 = Triangles[i + 1];
		int32 i2 = Triangles[i + 2];

		const FVector& V0 = Vertices[i0];
		const FVector& V1 = Vertices[i1];
		const FVector& V2 = Vertices[i2];

		FVector Edge1 = V1 - V0;
		FVector Edge2 = V2 - V0;
		FVector Normal = FVector::CrossProduct(Edge1, Edge2).GetSafeNormal();

		// Ajouter cette normale à chaque sommet
		Normals[i0] += Normal;
		Normals[i1] += Normal;
		Normals[i2] += Normal;
	}

	// Normaliser toutes les normales
	for (FVector& N : Normals)
		N.Normalize();

	// --- Création du mesh final ---
	CreateMeshSection(0, Vertices, Triangles, Normals, {}, {}, {}, true);
}

void UProceduralTerrain::CreateProceduralTerrain3D(int32 Size, float Scale, float NoiseScale, float HeightBias, float NoiseStrength, float IsoLevel)
{
	UE_LOG(LogTemp, Warning, TEXT("▶️ Génération terrain : Size=%d, Scale=%.2f, NoiseScale=%.3f, HeightBias=%.2f, Strength=%.2f, Iso=%.2f"),
		Size, Scale, NoiseScale, HeightBias, NoiseStrength, IsoLevel);

	// Sauvegarde des paramètres courants
	CurrentSize     = Size;
	CurrentScale    = Scale;
	CurrentIsoLevel = IsoLevel;

	// Initialisation du champ de densité
	Density.SetNum(Size * Size * Size);

	auto GetIndex = [&](int32 x, int32 y, int32 z)
	{
		return x + y * Size + z * Size * Size;
	};

	// ─────────── GÉNÉRATION DU CHAMP DE DENSITÉ ───────────
	for (int32 z = 0; z < Size; z++)
		for (int32 y = 0; y < Size; y++)
			for (int32 x = 0; x < Size; x++)
			{
				FVector Pos(x, y, z);
				float Noise = FMath::PerlinNoise3D(Pos * NoiseScale);
				float DensityValue = (z - HeightBias) + (Noise * NoiseStrength);
				Density[GetIndex(x, y, z)] = DensityValue;
			}

	// ─────────── CONSTRUCTION DU MESH À PARTIR DE DENSITY ───────────
	RebuildMeshFromCurrentDensity();

	UE_LOG(LogTemp, Warning, TEXT("✅ Terrain 3D généré (%d³ voxels)"), Size);
}

void UProceduralTerrain::ClearMesh()
{
	const FString FileName = TEXT("TerrainDensity.json");
	const FString FilePath = FPaths::ProjectSavedDir() / FileName;
	
	if (FPaths::FileExists(FilePath))
	{
		const bool bDeleted = IFileManager::Get().Delete(*FilePath);

		if (bDeleted)
		{
			UE_LOG(LogTemp, Warning, TEXT("🗑️ Deleted terrain save file: %s"), *FilePath);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("❌ Failed to delete terrain file: %s"), *FilePath);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("⚠️ File not found, nothing to delete: %s"), *FilePath);
	}
	ClearAllMeshSections();
}

void UProceduralTerrain::DigSphere(FVector WorldPosition, float Radius, float Strength)
{
	UE_LOG(LogTemp, Warning, TEXT("🟢 DigSphere called on: %s"), *GetOwner()->GetName());
	if (Density.Num() == 0 || CurrentSize <= 0 || FMath::IsNearlyZero(CurrentScale))
		return;

	const FTransform ComponentTransform = GetComponentTransform();

	const FVector LocalPositionUnits = ComponentTransform.InverseTransformPosition(WorldPosition);
	const FVector LocalVoxelPosition = LocalPositionUnits / CurrentScale;
	int32 XCenter = FMath::RoundToInt(LocalVoxelPosition.X);
	int32 YCenter = FMath::RoundToInt(LocalVoxelPosition.Y);
	int32 ZCenter = FMath::RoundToInt(LocalVoxelPosition.Z);
	const FVector ComponentScale = ComponentTransform.GetScale3D();
	const float UniformScale = FMath::Max(ComponentScale.GetAbsMax(), KINDA_SMALL_NUMBER);
	const float LocalRadiusUnits = Radius / UniformScale;
	int32 RadiusVoxels = FMath::CeilToInt(LocalRadiusUnits / CurrentScale);
	auto GetIndex = [&](int32 X, int32 Y, int32 Z)
	{
		return X + Y * CurrentSize + Z * CurrentSize * CurrentSize;
	};

	// Parcourt tous les voxels dans une sphère locale
	for (int32 z = ZCenter - RadiusVoxels; z <= ZCenter + RadiusVoxels; z++)
		for (int32 y = YCenter - RadiusVoxels; y <= YCenter + RadiusVoxels; y++)
			for (int32 x = XCenter - RadiusVoxels; x <= XCenter + RadiusVoxels; x++)
			{
				if (x < 0 || y < 0 || z < 0 || x >= CurrentSize || y >= CurrentSize || z >= CurrentSize)
					continue;

				FVector VoxelPos(x, y, z);
				float Dist = FVector::Dist(VoxelPos, LocalVoxelPosition);

				if (Dist <= RadiusVoxels)
				{
					int32 Idx = GetIndex(x, y, z);
					Density[Idx] += Strength * (1.0f - (Dist / RadiusVoxels)); // atténuation
				}
			}
	float MinD = FLT_MAX;
	float MaxD = -FLT_MAX;
	for (float V : Density)
	{
		MinD = FMath::Min(MinD, V);
		MaxD = FMath::Max(MaxD, V);
	}
	UE_LOG(LogTemp, Warning, TEXT("📊 Densité globale : Min=%.2f  Max=%.2f  Iso=%.2f"), MinD, MaxD, CurrentIsoLevel);

	RebuildMeshFromCurrentDensity();
}

void UProceduralTerrain::SaveDensityToJSON(const FString& FileName)
{
	if (Density.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("❌ No density data to save."));
		return;
	}

	TArray<TSharedPtr<FJsonValue>> DensityArray;
	DensityArray.Reserve(Density.Num());
	for (float Value : Density)
	{
		DensityArray.Add(MakeShareable(new FJsonValueNumber(Value)));
	}

	TSharedRef<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetArrayField(TEXT("Density"), DensityArray);
	JsonObject->SetNumberField(TEXT("Size"), CurrentSize);
	JsonObject->SetNumberField(TEXT("Scale"), CurrentScale);
	JsonObject->SetNumberField(TEXT("IsoLevel"), CurrentIsoLevel);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject, Writer);

	const FString SavePath = FPaths::ProjectSavedDir() / FileName;
	if (FFileHelper::SaveStringToFile(OutputString, *SavePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("💾 Terrain saved to JSON: %s"), *SavePath);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("❌ Failed to save terrain JSON: %s"), *SavePath);
	}
}

void UProceduralTerrain::LoadDensityFromJSON(const FString& FileName)
{
	const FString LoadPath = FPaths::ProjectSavedDir() / FileName;
	FString JsonContent;

	if (!FFileHelper::LoadFileToString(JsonContent, *LoadPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("⚠️ No JSON file found: %s"), *LoadPath);
		return;
	}

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("❌ Failed to parse terrain JSON file."));
		return;
	}

	CurrentSize = JsonObject->GetIntegerField(TEXT("Size"));
	CurrentScale = JsonObject->GetNumberField(TEXT("Scale"));
	CurrentIsoLevel = JsonObject->GetNumberField(TEXT("IsoLevel"));

	const TArray<TSharedPtr<FJsonValue>>* DensityArray;
	if (JsonObject->TryGetArrayField(TEXT("Density"), DensityArray))
	{
		Density.SetNum(DensityArray->Num());
		for (int32 i = 0; i < DensityArray->Num(); i++)
		{
			Density[i] = (*DensityArray)[i]->AsNumber();
		}

		RebuildMeshFromCurrentDensity();

		UE_LOG(LogTemp, Warning, TEXT("✅ Terrain loaded from JSON (%d voxels)"), Density.Num());
	}
}

void UProceduralTerrain::RefreshTerrain()
{
	LoadDensityFromJSON(TEXT("TerrainDensity.json"));
	// MarkRenderStateDirty();
	// Modify();
	// MarkPackageDirty();
	UE_LOG(LogTemp, Warning, TEXT("🔁 Terrain refreshed manually in editor"));
}


void UProceduralTerrain::RebuildMeshFromCurrentDensity()
{
 const int32 Size      = CurrentSize;
	const float Scale     = CurrentScale;
	const float IsoLevel  = CurrentIsoLevel;

	if (Size <= 1 || Density.Num() != Size * Size * Size)
	{
		UE_LOG(LogTemp, Warning, TEXT("⚠️ Impossible de reconstruire le mesh : données incohérentes."));
		ClearAllMeshSections();
		return;
	}

	auto GetIndex = [&](int32 x, int32 y, int32 z)
	{
		return x + y * Size + z * Size * Size;
	};

	auto SampleDensity = [&](int32 sx, int32 sy, int32 sz)
	{
		return Density[GetIndex(
			FMath::Clamp(sx, 0, Size - 1),
			FMath::Clamp(sy, 0, Size - 1),
			FMath::Clamp(sz, 0, Size - 1))];
	};

	TArray<FVector> Gradients;
	Gradients.SetNum(Density.Num());

	for (int32 z = 0; z < Size; z++)
	for (int32 y = 0; y < Size; y++)
	for (int32 x = 0; x < Size; x++)
	{
		float dx = SampleDensity(x + 1, y, z) - SampleDensity(x - 1, y, z);
		float dy = SampleDensity(x, y + 1, z) - SampleDensity(x, y - 1, z);
		float dz = SampleDensity(x, y, z + 1) - SampleDensity(x, y, z - 1);

		FVector Gradient(dx, dy, dz);
		if (!FMath::IsNearlyZero(Scale))
		{
			Gradient.X /= Scale;
			Gradient.Y /= Scale;
			Gradient.Z /= Scale;
		}

		Gradients[GetIndex(x, y, z)] = Gradient;
	}

	TArray<FVector> Vertices;
	TArray<int32>   Triangles;
	TArray<FVector> Normals;

	struct FVertexInterpResult
	{
		FVector Position;
		FVector Normal;
	};

	auto VertexInterp = [&](const FVector& p1, const FVector& p2, float valp1, float valp2,
	                        const FVector& n1, const FVector& n2)
	{
		if (FMath::Abs(IsoLevel - valp1) < KINDA_SMALL_NUMBER)
			return FVertexInterpResult{p1, n1.GetSafeNormal()};
		if (FMath::Abs(IsoLevel - valp2) < KINDA_SMALL_NUMBER)
			return FVertexInterpResult{p2, n2.GetSafeNormal()};
		if (FMath::Abs(valp1 - valp2) < KINDA_SMALL_NUMBER)
			return FVertexInterpResult{p1, n1.GetSafeNormal()};

		float mu = (IsoLevel - valp1) / (valp2 - valp1);

		FVector Position = p1 + mu * (p2 - p1);
		FVector Normal   = (n1 + mu * (n2 - n1));
		if (!Normal.Normalize())
		{
			Normal = FVector::UpVector;
		}

		return FVertexInterpResult{Position, Normal};
	};

	for (int32 z = 0; z < Size - 1; z++)
	for (int32 y = 0; y < Size - 1; y++)
	for (int32 x = 0; x < Size - 1; x++)
	{
		FVector p[8];
		float   val[8];
		FVector grad[8];

		static const int CornerOffsets[8][3] = {
			{0, 0, 0},
			{1, 0, 0},
			{1, 1, 0},
			{0, 1, 0},
			{0, 0, 1},
			{1, 0, 1},
			{1, 1, 1},
			{0, 1, 1}
		};

		for (int i = 0; i < 8; i++)
		{
			const int dx = CornerOffsets[i][0];
			const int dy = CornerOffsets[i][1];
			const int dz = CornerOffsets[i][2];

			p[i]    = FVector((x + dx) * Scale, (y + dy) * Scale, (z + dz) * Scale);
			val[i]  = Density[GetIndex(x + dx, y + dy, z + dz)];
			grad[i] = Gradients[GetIndex(x + dx, y + dy, z + dz)].GetSafeNormal();
		}

		int cubeIndex = 0;
		if (val[0] < IsoLevel) cubeIndex |= 1;
		if (val[1] < IsoLevel) cubeIndex |= 2;
		if (val[2] < IsoLevel) cubeIndex |= 4;
		if (val[3] < IsoLevel) cubeIndex |= 8;
		if (val[4] < IsoLevel) cubeIndex |= 16;
		if (val[5] < IsoLevel) cubeIndex |= 32;
		if (val[6] < IsoLevel) cubeIndex |= 64;
		if (val[7] < IsoLevel) cubeIndex |= 128;

		if (edgeTable[cubeIndex] == 0)
			continue;

		FVertexInterpResult vertList[12];

		if (edgeTable[cubeIndex] & 1)    vertList[0]  = VertexInterp(p[0], p[1], val[0], val[1], grad[0], grad[1]);
		if (edgeTable[cubeIndex] & 2)    vertList[1]  = VertexInterp(p[1], p[2], val[1], val[2], grad[1], grad[2]);
		if (edgeTable[cubeIndex] & 4)    vertList[2]  = VertexInterp(p[2], p[3], val[2], val[3], grad[2], grad[3]);
		if (edgeTable[cubeIndex] & 8)    vertList[3]  = VertexInterp(p[3], p[0], val[3], val[0], grad[3], grad[0]);
		if (edgeTable[cubeIndex] & 16)   vertList[4]  = VertexInterp(p[4], p[5], val[4], val[5], grad[4], grad[5]);
		if (edgeTable[cubeIndex] & 32)   vertList[5]  = VertexInterp(p[5], p[6], val[5], val[6], grad[5], grad[6]);
		if (edgeTable[cubeIndex] & 64)   vertList[6]  = VertexInterp(p[6], p[7], val[6], val[7], grad[6], grad[7]);
		if (edgeTable[cubeIndex] & 128)  vertList[7]  = VertexInterp(p[7], p[4], val[7], val[4], grad[7], grad[4]);
		if (edgeTable[cubeIndex] & 256)  vertList[8]  = VertexInterp(p[0], p[4], val[0], val[4], grad[0], grad[4]);
		if (edgeTable[cubeIndex] & 512)  vertList[9]  = VertexInterp(p[1], p[5], val[1], val[5], grad[1], grad[5]);
		if (edgeTable[cubeIndex] & 1024) vertList[10] = VertexInterp(p[2], p[6], val[2], val[6], grad[2], grad[6]);
		if (edgeTable[cubeIndex] & 2048) vertList[11] = VertexInterp(p[3], p[7], val[3], val[7], grad[3], grad[7]);

		for (int i = 0; triTable[cubeIndex][i] != -1; i += 3)
		{
			const FVertexInterpResult& r0 = vertList[triTable[cubeIndex][i]];
			const FVertexInterpResult& r1 = vertList[triTable[cubeIndex][i + 1]];
			const FVertexInterpResult& r2 = vertList[triTable[cubeIndex][i + 2]];

			const FVector& v0 = r0.Position;
			const FVector& v1 = r1.Position;
			const FVector& v2 = r2.Position;

			int32 BaseIndex = Vertices.Num();
			Vertices.Add(v0);
			Vertices.Add(v1);
			Vertices.Add(v2);

			Triangles.Add(BaseIndex);
			Triangles.Add(BaseIndex + 1);
			Triangles.Add(BaseIndex + 2);

			FVector FaceNormal = FVector::CrossProduct(v1 - v0, v2 - v0).GetSafeNormal();

			auto AddNormal = [&](const FVector& Candidate)
			{
				FVector Result = Candidate;
				if (!Result.Normalize())
				{
					Result = FaceNormal;
				}
				Normals.Add(Result);
			};

			AddNormal(r0.Normal);
			AddNormal(r1.Normal);
			AddNormal(r2.Normal);
		}
	}
	ClearAllMeshSections();
	CreateMeshSection(0, Vertices, Triangles, Normals, {}, {}, {}, true);

	UE_LOG(LogTemp, Warning, TEXT("✅ Mesh reconstruit (%d sommets / %d triangles)"),
		Vertices.Num(), Triangles.Num() / 3);
}

void UProceduralTerrain::BuildDensityField(int32 Size, float Scale, float NoiseScale, float HeightBias,
	float NoiseStrength)
{
	CurrentSize  = Size;
	CurrentScale = Scale;
	Density.SetNum(Size * Size * Size);

	FVector ChunkWorldOrigin = GetComponentLocation();

	auto GetIndex = [&](int32 x, int32 y, int32 z)
	{
		return x + y * Size + z * Size * Size;
	};
	check(Density.Num() == Size * Size * Size);
	check(Size > 0);
	for (int32 z = 0; z < Size; ++z)
		for (int32 y = 0; y < Size; ++y)
			for (int32 x = 0; x < Size; ++x)
			{
				FVector WorldPos = ChunkWorldOrigin + FVector(x * Scale, y * Scale, z * Scale);
				float Noise = FMath::PerlinNoise3D(WorldPos * NoiseScale);
				float DensityValue = (z - HeightBias) + (Noise * NoiseStrength);
				int32 Index = GetIndex(x, y, z);
				check(Index >= 0 && Index < Density.Num());
				Density[Index] = DensityValue;
				Density[GetIndex(x, y, z)] = DensityValue;
			}
}

bool UProceduralTerrain::ContainsWorldPoint(const FVector& WorldPos, float Radius) const
{
	const FBox ChunkBox = Bounds.GetBox();
	return FMath::SphereAABBIntersection(FSphere(WorldPos, Radius), ChunkBox);
}