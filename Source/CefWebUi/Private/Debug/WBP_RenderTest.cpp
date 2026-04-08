// Fill out your copyright notice in the Description page of Project Settings.


#include "Debug/WBP_RenderTest.h"

#include "Components/Image.h"

void UWBP_RenderTest::SetDisplayTexture(UTexture2D* InTexture)
{
	if (!DisplayImage || !InTexture)
		return;

	UMaterialInstanceDynamic* mid = DisplayImage->GetDynamicMaterial();
	if (!mid)
	{
		DisplayImage->SetBrushFromTexture(InTexture);
		return;
	}

	mid->SetTextureParameterValue(TEXT("Texture"), InTexture);
}
