#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CefWebUiSlateHostWidget.generated.h"

class SCefBrowserSurface;
class UCefWebUiBrowserSession;

UCLASS(BlueprintType, Blueprintable)
class CEFWEBUI_API UCefWebUiSlateHostWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	void SetBrowserSession(UCefWebUiBrowserSession* InBrowserSession);

	UFUNCTION(BlueprintPure, Category="CefWebUi")
	UCefWebUiBrowserSession* GetBrowserSession() const { return BrowserSession; }

	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	void SetBrowserSize(int32 InBrowserWidth, int32 InBrowserHeight);

	UFUNCTION(BlueprintPure, Category="CefWebUi")
	int32 GetBrowserWidth() const { return BrowserWidth; }

	UFUNCTION(BlueprintPure, Category="CefWebUi")
	int32 GetBrowserHeight() const { return BrowserHeight; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	virtual void SynchronizeProperties() override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CefWebUi")
	TObjectPtr<UCefWebUiBrowserSession> BrowserSession = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CefWebUi")
	int32 BrowserWidth = 1920;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CefWebUi")
	int32 BrowserHeight = 1080;

private:
	TSharedPtr<SCefBrowserSurface> BrowserSurfaceWidget;
};
