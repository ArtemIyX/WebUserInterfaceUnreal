#pragma once

#include "CoreMinimal.h"
#include "CefConsoleLogLevel.generated.h"

UENUM(BlueprintType)
enum class ECefConsoleLogLevel : uint8
{
	Log = 0,
	Warning = 1,
	Error = 2,
};
