// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;
using System.Collections.Generic;

public class WillBeAOS : ModuleRules
{
	public WillBeAOS(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PublicDefinitions.Add("_UNICODE");
			PublicDefinitions.Add("UNICODE");
		}
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "Slate", "SlateCore", "Niagara", 
			"OnlineSubsystem", "OnlineSubsystemEOS", "OnlineSubsystemUtils", "Networking", "HTTP", "Json", "JsonUtilities" });

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AIModule", "NavigationSystem", "AnimGraphRuntime", "AsyncLoadingScreen",
			"GameplayAbilities", "GameplayTasks", "GameplayTags", "AnimNextAnimGraph", "AnimGraphRuntime"
		});
		
		if (Target.Type == TargetType.Server && Target.Platform == UnrealTargetPlatform.Linux)
		{
			PrivateDependencyModuleNames.Add("GameLiftServerSDK");
			PublicDefinitions.Add("WITH_GAMELIFT=1");
		}

        // ���� ��� �߰�
        PublicIncludePaths.Add(ModuleDirectory);
        PublicIncludePaths.Add("WillBeAOS");

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
