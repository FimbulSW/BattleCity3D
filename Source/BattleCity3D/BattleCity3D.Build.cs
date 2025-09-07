// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class BattleCity3D : ModuleRules
{
	public BattleCity3D(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateIncludePaths.AddRange(new string[]
		{
			"BattleCity3D/Private/BattleBases",
            "BattleCity3D/Private/Components/EnemyMovement",
            "BattleCity3D/Private/Components/EnemyMovement/EnemyMovePolicies",
            "BattleCity3D/Private/Components/GridPathFollow",
            "BattleCity3D/Private/Enemies",
            "BattleCity3D/Private/Enemies/EnemyGoalPolicies",
            "BattleCity3D/Private/GameClasses",
            "BattleCity3D/Private/Map",
            "BattleCity3D/Private/Player",
            "BattleCity3D/Private/Projectiles",
            "BattleCity3D/Private/Spawner",
            "BattleCity3D/Private/Spawner/SpawnPointPolicies",
            "BattleCity3D/Private/Utils",
        });

	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });

		PrivateDependencyModuleNames.AddRange(new string[] { "Json", "JsonUtilities" });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
