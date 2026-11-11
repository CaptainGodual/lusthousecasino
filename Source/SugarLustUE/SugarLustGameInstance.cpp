#include "SugarLustGameInstance.h"
#include "SugarLustUpdatePolicy.h"

#include "Engine/Texture2D.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Misc/ConfigCacheIni.h"
#include "HAL/PlatformProcess.h"
#include "MoviePlayer.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

void USugarLustGameInstance::Init()
{
	Super::Init();
	StartLoadingScreen();
	CheckForUpdates();
}

void USugarLustGameInstance::ShowLoadingScreen()
{
	if (!bLoadingScreenStarted)
	{
		StartLoadingScreen();
	}
}

void USugarLustGameInstance::StartLoadingScreen()
{
	if (IsRunningDedicatedServer() || !GetMoviePlayer())
	{
		return;
	}

	LoadingBackdrop = LoadObject<UTexture2D>(nullptr, TEXT("/Game/Art/sugarlust_backdrop.sugarlust_backdrop"));
	LoadingLogo = LoadObject<UTexture2D>(nullptr, TEXT("/Game/Art/logo.logo"));
	if (!LoadingUpdateStatus.IsValid())
	{
		LoadingUpdateStatus = MakeShared<FString, ESPMode::ThreadSafe>(TEXT("Checking for updates..."));
	}

	if (LoadingBackdrop)
	{
		BackdropBrush.SetResourceObject(LoadingBackdrop);
		BackdropBrush.ImageSize = FVector2D(LoadingBackdrop->GetSizeX(), LoadingBackdrop->GetSizeY());
	}
	if (LoadingLogo)
	{
		LogoBrush.SetResourceObject(LoadingLogo);
		LogoBrush.ImageSize = FVector2D(LoadingLogo->GetSizeX(), LoadingLogo->GetSizeY());
	}

	const TWeakObjectPtr<USugarLustGameInstance> WeakThis(this);
	TSharedRef<SWidget> BrandWidget = LoadingLogo
		? StaticCastSharedRef<SWidget>(SNew(SScaleBox)
			.Stretch(EStretch::ScaleToFit)
			[
				SNew(SImage).Image(&LogoBrush)
			])
		: StaticCastSharedRef<SWidget>(SNew(STextBlock)
			.Text(FText::FromString(TEXT("SUGAR LUST")))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 58))
			.ColorAndOpacity(FLinearColor(1.0f, 0.78f, 0.23f))
			.Justification(ETextJustify::Center));

	TSharedRef<SOverlay> LoadingWidget = SNew(SOverlay);

	if (LoadingBackdrop)
	{
		LoadingWidget->AddSlot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SScaleBox)
			.Stretch(EStretch::ScaleToFill)
			[
				SNew(SImage).Image(&BackdropBrush)
			]
		];
	}
	else
	{
		LoadingWidget->AddSlot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SColorBlock).Color(FLinearColor(0.12f, 0.015f, 0.16f, 1.0f))
		];
	}

	LoadingWidget->AddSlot()
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Fill)
	[
		SNew(SColorBlock).Color(FLinearColor(0.035f, 0.0f, 0.055f, 0.38f))
	];

	LoadingWidget->AddSlot()
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Center)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		.Padding(24.0f, 0.0f, 24.0f, 18.0f)
		[
			SNew(SBox)
			.WidthOverride(520.0f)
			.HeightOverride(220.0f)
			[
				BrandWidget
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		.Padding(20.0f, 0.0f, 20.0f, 16.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Sweet spins are loading.")))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 30))
			.ColorAndOpacity(FLinearColor(1.0f, 0.82f, 0.35f))
			.Justification(ETextJustify::Center)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		.Padding(8.0f)
		[
			SNew(SCircularThrobber)
			.NumPieces(8)
			.Period(1.1f)
			.Radius(20.0f)
			.ColorAndOpacity(FLinearColor(1.0f, 0.12f, 0.67f))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		.Padding(12.0f, 10.0f, 12.0f, 4.0f)
		[
			SNew(STextBlock)
			.Text_Lambda([WeakThis]()
			{
				const USugarLustGameInstance* Self=WeakThis.Get();
				return Self && Self->LoadingUpdateStatus.IsValid()
					? FText::FromString(*Self->LoadingUpdateStatus)
					: FText::FromString(TEXT("Checking for updates..."));
			})
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 18))
			.ColorAndOpacity(FLinearColor(1.0f, 0.91f, 0.98f))
			.Justification(ETextJustify::Center)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		.Padding(8.0f, 6.0f)
		[
			SNew(SButton)
			.Visibility_Lambda([WeakThis]() { return WeakThis.IsValid() && WeakThis->bUpdateAvailable ? EVisibility::Visible : EVisibility::Collapsed; })
			.OnClicked_Lambda([WeakThis]()
			{
				const USugarLustGameInstance* Self=WeakThis.Get();
				if (Self && SugarLustUpdatePolicy::IsAllowedDownloadUrl(Self->UpdateDownloadUrl))
				{
					FPlatformProcess::LaunchURL(*Self->UpdateDownloadUrl, nullptr, nullptr);
				}
				return FReply::Handled();
			})
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("DOWNLOAD UPDATE")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
				.ColorAndOpacity(FLinearColor(1.0f, 0.78f, 0.23f))
			]
		]
	];

	LoadingWidget->AddSlot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(18.f, 18.f, 18.f, 24.f)
    [
        SNew(STextBlock).Text(FText::FromString(TEXT("Online connection required. Guest login is saved on this device; clearing app data can lose account access.")))
        .WrapTextAt(780.f).Justification(ETextJustify::Center)
        .Font(FCoreStyle::GetDefaultFontStyle("Regular",16))
        .ColorAndOpacity(FLinearColor(1.f,.91f,.98f))
    ];
	FLoadingScreenAttributes Attributes;
	Attributes.WidgetLoadingScreen = LoadingWidget;
	Attributes.MinimumLoadingScreenDisplayTime = 2.0f;
	Attributes.bAutoCompleteWhenLoadingCompletes = false;
	Attributes.bWaitForManualStop = true;
	Attributes.bAllowEngineTick = true;

	GetMoviePlayer()->SetupLoadingScreen(Attributes);
	bLoadingScreenStarted = GetMoviePlayer()->PlayMovie();
}

void USugarLustGameInstance::CheckForUpdates()
{
	if (!LoadingUpdateStatus.IsValid())
	{
		LoadingUpdateStatus = MakeShared<FString, ESPMode::ThreadSafe>(TEXT("Checking for updates..."));
	}

	FString ManifestUrl;
	GConfig->GetString(TEXT("SugarLust.Update"), TEXT("ManifestUrl"), ManifestUrl, GGameIni);
	ManifestUrl.TrimStartAndEndInline();
	if (ManifestUrl.IsEmpty())
	{
		*LoadingUpdateStatus = TEXT("Update check is not configured.");
		return;
	}

	bUpdateAvailable=false;
	UpdateDownloadUrl.Reset();
	FString CurrentVersion;
	GConfig->GetString(TEXT("/Script/EngineSettings.GeneralProjectSettings"), TEXT("ProjectVersion"), CurrentVersion, GGameIni);
	if (CurrentVersion.IsEmpty()) GConfig->GetString(TEXT("SugarLust.Update"), TEXT("CurrentVersion"), CurrentVersion, GGameIni);
	if (CurrentVersion.IsEmpty())
	{
		CurrentVersion = TEXT("0.0.2");
	}

	TArray<int32> CurrentParts;
	if (!ManifestUrl.StartsWith(TEXT("https://"), ESearchCase::IgnoreCase) || !SugarLustUpdatePolicy::ParseVersion(CurrentVersion, CurrentParts))
	{
		*LoadingUpdateStatus=TEXT("Update check is not configured correctly. Continuing to login.");
		return;
	}
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(ManifestUrl);
	Request->SetVerb(TEXT("GET"));
	Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
	Request->SetTimeout(8.0f);
	UpdateRequest=Request;
	Request->OnProcessRequestComplete().BindWeakLambda(this, [this, CurrentVersion](FHttpRequestPtr, FHttpResponsePtr Response, bool bConnectedSuccessfully)
	{
		if (!LoadingUpdateStatus.IsValid())
		{
			return;
		}
		if (!bConnectedSuccessfully || !Response.IsValid() || !EHttpResponseCodes::IsOk(Response->GetResponseCode()))
		{
			*LoadingUpdateStatus = TEXT("Could not check for updates. Continuing to login.");
			return;
		}

		if (Response->GetContentLength() > 65536 || Response->GetContent().Num() > 65536)
		{
			*LoadingUpdateStatus=TEXT("Update information is too large. Continuing to login.");
			return;
		}
		TSharedPtr<FJsonObject> Manifest;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
		if (!FJsonSerializer::Deserialize(Reader, Manifest) || !Manifest.IsValid())
		{
			*LoadingUpdateStatus = TEXT("Update information could not be read.");
			return;
		}

		FString LatestVersion;
		Manifest->TryGetStringField(TEXT("version"), LatestVersion);
		TArray<int32> LatestParts;
		if (!SugarLustUpdatePolicy::ParseVersion(LatestVersion, LatestParts))
		{
			*LoadingUpdateStatus = TEXT("Update information is incomplete.");
			return;
		}

		Manifest->TryGetStringField(TEXT("download_url"), UpdateDownloadUrl);
		if (SugarLustUpdatePolicy::IsNewerVersion(LatestVersion, CurrentVersion))
		{
			bUpdateAvailable = SugarLustUpdatePolicy::IsAllowedDownloadUrl(UpdateDownloadUrl);
			if (!bUpdateAvailable) UpdateDownloadUrl.Reset();
			*LoadingUpdateStatus = bUpdateAvailable ? FString::Printf(TEXT("Version %s is available."), *LatestVersion) : TEXT("Update link could not be verified. Continuing to login.");
		}
		else
		{
			bUpdateAvailable = false;
			*LoadingUpdateStatus = FString::Printf(TEXT("You are up to date (v%s)."), *CurrentVersion);
		}
	});
	if (!Request->ProcessRequest())
	{
		*LoadingUpdateStatus = TEXT("Could not start update check. Continuing to login.");
	}
}

void USugarLustGameInstance::StopLoadingScreen()
{
	if (bLoadingScreenStarted && GetMoviePlayer())
	{
		GetMoviePlayer()->StopMovie();
		bLoadingScreenStarted = false;
	}
}

void USugarLustGameInstance::Shutdown()
{
    if (UpdateRequest.IsValid())
    {
        UpdateRequest->OnProcessRequestComplete().Unbind();
        UpdateRequest->CancelRequest();
        UpdateRequest.Reset();
    }
    StopLoadingScreen();
    Super::Shutdown();
}
