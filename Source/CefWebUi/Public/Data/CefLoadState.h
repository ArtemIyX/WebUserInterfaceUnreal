// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

UENUM(BlueprintType, Blueprintable)
enum class ECefLoadState : uint8
{
	Idle = 0,
	Loading = 1,
	Ready = 2,
	Error = 3,
};
