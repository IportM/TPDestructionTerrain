# Procedural Dungeon Generation - Unreal Engine 5.4

A students C++ Unreal Engine project by SENHAJ Mehdi and PETIT Matéo, that generates procedural dungeons using advanced triangulation algorithms. The system combines efficient C++ core logic with Blueprint assets to create dynamic dungeon layouts.

## Features

- **Delaunay Triangulation**: Advanced geometric algorithms for optimal room placement
- **Configurable Room Types**: Data-driven room generation with customizable probabilities
- **Overlap Resolution**: Automatic collision detection and position adjustment
- **Blueprint Integration**: Visual room variations through Blueprint inheritance
- **Step-by-Step Visualization**: Debug mode for understanding the generation process

## Project Structure

```
DungeonProcedural/
├── Source/DungeonProcedural/          # C++ source code
│   ├── RoomManager.h/.cpp             # Main generation logic (World Subsystem)
│   ├── RoomParent.h/.cpp              # Base room actor class
│   ├── Triangle.h/.cpp                # Triangulation algorithms
│   └── ConfigRoomDataAsset.h          # Configuration data asset
└── Content/                           # Unreal assets and blueprints
    ├── NewWorld.umap                  # Main test level
    ├── ConfigRoom.uasset              # Room generation  parameters
    └── Room*.uasset                   # Room type blueprintsconfiguration files
```

## Prerequisites

- **Unreal Engine 5.4** or later
- **Visual Studio 2022** (with C++ development tools)

## Setup Instructions

### 1. Generate Visual Studio Project Files

Right-click on `DungeonProcedural.uproject` and select:
**"Generate Visual Studio project files"**

### 2. Build the Project

#### Visual Studio
1. Open `DungeonProcedural.sln`
2. Set build configuration to **Development Editor**
3. Build the solution (Ctrl+Shift+B)

## Running the Project

1. Launch `DungeonProcedural.uproject` in Unreal Editor

## How to Test

### Setup Testing Environment
1. **Open the Map**: Load `Content/NewWorld.umap` in the Unreal Editor
2. **Configure Room Parameters**: Open the Data Asset `ConfigRoom` to define room spawn probabilities and sizes
3. **Launch Dungeon Tool**: Right-click on the `DungeonTool` file and select **"Run Editor Utility Widget"**

### Using the Dungeon Generation Tool

Once the DungeonTool widget is open, you have several testing options:

#### Quick Generation
- **"Create Dungeon"** - Generates a complete dungeon instantly with all steps automated

#### Manual Generation (Step-by-Step)
Use the buttons in the **Manual** category to control each generation phase:
- Execute each step of the dungeon generation process individually
- Perfect for understanding the generation pipeline
- Allows inspection of intermediate results

#### Auto Generation (Detailed Triangulation)
Use the buttons in the **Auto** category for detailed triangulation analysis:
- Each micro-step of the triangulation algorithm is displayed
- Ideal for debugging triangulation issues
- Visual feedback for each triangulation operation

#### Cleanup
- **"Clear"** - Removes all generated dungeon elements to start fresh

## Troubleshooting

### Compilation Issues
- Ensure Visual Studio 2022 with C++ tools is installed
- Regenerate project files if build errors occur
- Check that Unreal Engine 5.4+ is properly installed

### Runtime Issues
- Verify `Content/NewWorld.umap` is set as the startup level
- Check that room Blueprint assets are properly referenced
- Ensure `ConfigRoom.uasset` contains valid configuration data

### Generation Problems
- Monitor console output for error messages
- Use step-by-step mode for debugging triangulation
- Verify room collision components are properly configured
