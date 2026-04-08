// Fill out your copyright notice in the Description page of Project Settings.


#include "Libs/CefWebUiBPLibrary.h"

#include "Interfaces/IPluginManager.h"


FString UCefWebUiBPLibrary::GetHostExePath()
{
#if WITH_EDITOR
	FString pluginDir = IPluginManager::Get().FindPlugin(TEXT("CefWebUi"))->GetBaseDir();
	return FPaths::ConvertRelativePathToFull(
		FPaths::Combine(pluginDir, TEXT("Source"), TEXT("ThirdParty"), TEXT("Cef"), TEXT("Host.exe"))
	);
#else
	return FPaths::ConvertRelativePathToFull(
		FPaths::Combine(FPlatformProcess::BaseDir(), TEXT("Cef"), TEXT("Host.exe"))
	);
#endif
}
