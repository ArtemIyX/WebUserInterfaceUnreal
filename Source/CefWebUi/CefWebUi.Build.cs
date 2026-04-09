// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class CefWebUi : ModuleRules
{
	public CefWebUi(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[]
			{
				// ... add public include paths required here ...
			}
		);


		PrivateIncludePaths.AddRange(
			new string[]
			{
				// ... add other private include paths required here ...
			}
		);


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
		);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"InputCore",
				"Projects",
				"UMG",
				"RHI",
				"D3D12RHI",
				"RenderCore"
				// ... add private dependencies that you statically link with here ...	
			}
		);


		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
		);


		string CefDir = Path.Combine(PluginDirectory, "Source", "ThirdParty", "Cef");
		string[] extensions = { "*.dll", "*.pak", "*.bin", "*.dat", "*.exe", "*.so" };
		foreach (string ext in extensions)
		{
			RuntimeDependencies.Add(
				Path.Combine("$(BinaryOutputDir)", "Cef", ext),
				Path.Combine(CefDir, ext)
			);
		}

		// Locales subdirectory
		RuntimeDependencies.Add(
			Path.Combine("$(BinaryOutputDir)", "Cef", "locales", "..."),
			Path.Combine(CefDir, "locales", "...")
		);
		string EnginePath = Path.GetFullPath(Target.RelativeEnginePath);
		PrivateIncludePaths.Add(Path.Combine(EnginePath, "Source/Runtime/D3D12RHI/Private"));
		PrivateIncludePaths.Add(Path.Combine(EnginePath, "Source/ThirdParty/Windows/DirectX/include"));
		AddEngineThirdPartyPrivateStaticDependencies(Target, "DX12");
	}
}