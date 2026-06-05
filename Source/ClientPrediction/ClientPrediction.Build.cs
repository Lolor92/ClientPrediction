// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class ClientPrediction : ModuleRules
{
	public ClientPrediction(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "GameplayTags" });
	}
}
