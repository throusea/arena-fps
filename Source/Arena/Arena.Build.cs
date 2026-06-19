// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class Arena : ModuleRules
{
	public Arena(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"Niagara",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			ModuleDirectory,
			Path.Combine(ModuleDirectory, "Variant_Horror"),
			Path.Combine(ModuleDirectory, "Variant_Horror", "UI"),
			Path.Combine(ModuleDirectory, "Variant_Shooter"),
			Path.Combine(ModuleDirectory, "Variant_Shooter", "AI"),
			Path.Combine(ModuleDirectory, "Variant_Shooter", "UI"),
			Path.Combine(ModuleDirectory, "Variant_Shooter", "Weapons"),
			Path.Combine(ModuleDirectory, "Variant_Combat"),
			Path.Combine(ModuleDirectory, "Variant_Combat", "AI"),
			Path.Combine(ModuleDirectory, "Variant_Combat", "Animation"),
			Path.Combine(ModuleDirectory, "Variant_Combat", "Gameplay"),
			Path.Combine(ModuleDirectory, "Variant_Combat", "Interfaces"),
			Path.Combine(ModuleDirectory, "Variant_Combat", "UI")
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
