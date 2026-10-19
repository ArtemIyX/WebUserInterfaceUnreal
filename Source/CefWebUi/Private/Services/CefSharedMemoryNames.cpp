#include "Services/CefSharedMemoryNames.h"

namespace
{
FString MakeScopedName(const TCHAR* InBase, const FString& InScope)
{
	if (InScope.IsEmpty())
	{
		return FString(InBase);
	}
	return FString::Printf(TEXT("%s_%s"), InBase, *InScope);
}
}

FString BuildCefSessionScope(const FString& InSessionId)
{
	FString Scope;
	Scope.Reserve(InSessionId.Len());
	for (const TCHAR Char : InSessionId)
	{
		if (FChar::IsAlnum(Char) || Char == TEXT('_') || Char == TEXT('-'))
		{
			Scope.AppendChar(Char);
		}
	}
	return Scope;
}

FCefSharedMemoryNames BuildCefSharedMemoryNames(const FString& InSessionScope)
{
	FCefSharedMemoryNames Names;
	Names.FrameMapName = MakeScopedName(TEXT("CEFHost_Frame"), InSessionScope);
	Names.InputMapName = MakeScopedName(TEXT("CEFHost_Input"), InSessionScope);
	Names.ControlMapName = MakeScopedName(TEXT("CEFHost_Control"), InSessionScope);
	Names.ConsoleMapName = MakeScopedName(TEXT("CEFHost_Console"), InSessionScope);
	Names.FrameReadyEventName = MakeScopedName(TEXT("CEFHost_FrameReady"), InSessionScope);
	Names.InputReadyEventName = MakeScopedName(TEXT("CEFHost_InputReady"), InSessionScope);
	Names.ControlReadyEventName = MakeScopedName(TEXT("CEFHost_ControlReady"), InSessionScope);
	Names.ConsoleReadyEventName = MakeScopedName(TEXT("CEFHost_ConsoleReady"), InSessionScope);
	Names.SharedTextureNamePrefix = MakeScopedName(TEXT("Global\\CEFHost_SharedTex"), InSessionScope);
	Names.SharedPopupTextureName = MakeScopedName(TEXT("Global\\CEFHost_SharedPopupTex"), InSessionScope);
	Names.SharedGpuFenceName = MakeScopedName(TEXT("Global\\CEFHost_SharedFence"), InSessionScope);
	return Names;
}
