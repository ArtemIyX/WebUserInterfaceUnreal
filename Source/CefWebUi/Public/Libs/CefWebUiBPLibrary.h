// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CefWebUiBPLibrary.generated.h"

UCLASS()
class CEFWEBUI_API UCefWebUiBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * @brief Returns absolute path to Host.exe.
	 */
	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	static FString GetHostExePath();
};