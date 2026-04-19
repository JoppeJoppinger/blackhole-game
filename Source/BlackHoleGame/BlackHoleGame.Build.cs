// Copyright 2024 BlackHoleGame. All Rights Reserved.
using UnrealBuildTool;

public class BlackHoleGame : ModuleRules
{
	public BlackHoleGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"Niagara",
			"RenderCore",
			"Renderer",
			"RHI",
			"Slate",
			"SlateCore",
			"UMG"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"HeadMountedDisplay",
			"ProceduralMeshComponent"
		});

		// Enable double precision for physics
		PublicDefinitions.Add("UE_USE_DOUBLE_PRECISION_VECTORS=1");
	}
}
