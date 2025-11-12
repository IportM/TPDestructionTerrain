# Procedural Destructible Terrain – Unreal Engine 5.4

A C++ Unreal Engine project by **PETIT Matéo**, that lets you **create and destroy fully 3D voxel-based terrain** in real time.  
The project implements a **procedurally generated terrain system** using **Marching Cubes**, supports **on-the-fly editing** of the density field, and provides **JSON-based save/load persistence** for each chunk.

---

## Key Features

- **Procedural 3D Generation**  
  Builds a volumetric density field using layered Perlin noise, then extracts a mesh surface through the **Marching Cubes algorithm** (`UProceduralTerrain`).

- **Chunk-Based World**  
  The terrain is divided into multiple chunks generated and updated independently, enabling **asynchronous streaming** and **infinite expansion** (`AProceduralTerrainWorld`).

- **Terrain Modification**  
  Provides spherical digging and smoothing at runtime with automatic mesh reconstruction (`DigSphere`, `RebuildMeshFromCurrentDensity`).

- **Persistence System**  
  Saves and loads per-chunk density data as **JSON files** inside the project’s `Saved/TerrainChunks/` directory.

- **Editor Tooling**  
  Exposes generation parameters (size, scale, bias, noise, iso-level) and debug visualizations for chunk bounds.  
  Includes *Call in Editor* functions to rebuild or refresh terrain from JSON.
---

## Project Structure

```
DestructionTerrain.uproject
├── Content/                        # Assets, levels, and blueprints
├── Config/                         # Unreal Engine configuration files
└── Source/
    └── DestructionTerrain/
        ├── ProceduralTerrain.h/.cpp         # Voxel mesh & Marching Cubes algorithms
        ├── ProceduralTerrainActor.h/.cpp    # Simple actor wrapper around a terrain instance
        ├── ProceduralTerrainWorld.h/.cpp    # Chunk streaming, management, and persistence
        ├── MarchingCubesTables.h            # Edge tables used by Marching Cubes
        └── DestructionTerrain*.h/.cpp       # Module boilerplate files
```

## Requirements
- Unreal Engine **5.4** (or the TP-compatible version)
- Visual Studio **2022** with the C++ toolset for UE5 development
- Enough disk space to generate and save chunk data (`Saved/`)

## Setup Instructions

## Getting Started
1. Clone the repository or extract the provided archive.
2. Right-click `DestructionTerrain.uproject` and choose **"Generate Visual Studio project files"**.
3. Open `DestructionTerrain.sln` in Visual Studio.
4. Select the **Development Editor** configuration and the **Win64** platform.
5. Build the solution (`Ctrl+Shift+B`).

### Build the Project

## Quick Usage
1. Launch the project in the Unreal Editor.
2. Place the `ProceduralTerrainWorld` actor (or use the included sample level).
3. Adjust generation settings in the Details panel or in the BP_ChunkGenerator (chunk size, scale, noise, iso-level, debug display, etc.) and clic .
4. Click Compile or move the actor slightly to trigger OnConstruction→ Chunks are generated asynchronously — progress appears above the actor
5. Use gameplay interactions (explain on the HUD) to modify the terrain.
6. Use the TerrainTool → Refresh button to reload data in the editor.
6. To reset the world, delete the actor or delete the `Saved/TerrainChunks_*.json` files.


## Notes
The initial 5×5 grid of chunks is persistent:
always visible and saved when quitting the game.

Dynamically generated streamed chunks (infinite world)
can be modified but are not saved intentionally for performance reasons.

### Troubleshooting
## Compilation
Ensure Visual Studio 2022 with C++ tools is installed.
Regenerate project files if build errors occur.
Check your Unreal Engine 5.4+ installation path and version.

## Runtime
Make sure AProceduralTerrainWorld is placed in the level.
Verify ChunkSize and TerrainScale values are reasonable.
Move the actor or hit Compile to trigger regeneration.

## Generation / Streaming
Watch the Output Log for generation progress or errors.
If terrain is empty, confirm that no invalid JSON files exist in Saved/TerrainChunks/.
Adjust StreamRadius and UpdateInterval to tune streaming behavior.
