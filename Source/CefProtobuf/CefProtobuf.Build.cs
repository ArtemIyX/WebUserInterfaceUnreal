using UnrealBuildTool;
using System.IO;

public class CefProtobuf : ModuleRules
{
    public CefProtobuf(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore"
            }
        );

        string protobufRoot = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", "ThirdParty", "Protobuf"));
        string protobufIncludePath = Path.Combine(protobufRoot, "include");
        PublicSystemIncludePaths.Add(protobufIncludePath);

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            string protobufLibPath = Path.Combine(protobufRoot, "lib", "Win64", "libprotobuf-lite.lib");
            PublicAdditionalLibraries.Add(protobufLibPath);
        }
    }
}