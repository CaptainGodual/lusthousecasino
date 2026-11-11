#include "CasinoDisplayLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/WidgetComponent.h"
#include "Slate/WidgetRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Widgets/SWidget.h"
#include "RenderingThread.h"
FString UCasinoDisplayLibrary::DescribeDisplay(UUserWidget* Widget,UWidgetComponent* Display)
{
 if(!Widget)return TEXT("No widget");
 UWidget* Root=Widget->GetRootWidget();
 return FString::Printf(TEXT("Root=%s RootSize=%s Desired=%s SlateVisible=%d DisplayWidget=%s CurrentSize=%s"),*GetNameSafe(Root),Root?*Root->GetCachedGeometry().GetLocalSize().ToString():TEXT("none"),*Widget->GetDesiredSize().ToString(),Widget->TakeWidget()->GetVisibility().IsVisible(),Display?*GetNameSafe(Display->GetWidget()):TEXT("none"),Display?*Display->GetCurrentDrawSize().ToString():TEXT("none"));
}
UTextureRenderTarget2D* UCasinoDisplayLibrary::RenderWidgetPreview(UUserWidget* Widget)
{
 if(!Widget)return nullptr;
 FWidgetRenderer* Renderer=new FWidgetRenderer(true);
 UTextureRenderTarget2D* Target=Renderer->DrawWidget(Widget->TakeWidget(),FVector2D(1920,1080));
 FlushRenderingCommands();
 BeginCleanup(Renderer);
 return Target;
}

#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Serialization/BufferArchive.h"
bool UCasinoDisplayLibrary::SaveDisplayPreview(UTextureRenderTarget2D* Target,const FString& Filename)
{
 if(!Target)return false;
 FBufferArchive Bytes;
 return FImageUtils::ExportRenderTarget2DAsPNG(Target,Bytes)&&FFileHelper::SaveArrayToFile(Bytes,*Filename);
}
