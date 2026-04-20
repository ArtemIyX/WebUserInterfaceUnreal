/**
 * @file CefWebUi\Public\Debug\WBP_RenderTest.h
 * @brief Declares WBP_RenderTest for module CefWebUi\Public\Debug\WBP_RenderTest.h.
 * @details Contains types and APIs used by the plugin runtime and gameplay-facing systems.
 */
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "WBP_RenderTest.generated.h"

/**
 * 
 */
UCLASS()
class CEFWEBUI_API UWBP_RenderTest : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * @brief Sets the texture on the image brush.
	 * @param InTexture Texture to display.
	 */
	void SetDisplayTexture(UTexture2D* InTexture);

protected:
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Display")
	TObjectPtr<class UImage> DisplayImage;
};

