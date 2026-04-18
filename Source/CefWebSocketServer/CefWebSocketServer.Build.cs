using UnrealBuildTool;

public class CefWebSocketServer : ModuleRules
{
    public CefWebSocketServer(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "WebSockets",
                "Sockets"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "NetCore",
                "EngineSettings",
                "ImageCore",
                "PacketHandler",
                "Json",
                "JsonUtilities",
                "Projects",
                "OpenSSL",
                "libWebSockets",
                "zlib"
            }
        );

        PublicDefinitions.Add("USE_LIBWEBSOCKET=1");
    }
}
