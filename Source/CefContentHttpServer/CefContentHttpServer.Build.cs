using UnrealBuildTool;

public class CefContentHttpServer : ModuleRules
{
    public CefContentHttpServer(ReadOnlyTargetRules InTarget) : base(InTarget)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "HttpServer",
                "ImageWrapper",
                "Json",
                "Slate",
                "SlateCore"
            }
        );
    }
}
