// Copyright 2024 BlackHoleGame. All Rights Reserved.
#include "GraphicsQualityManager.h"
#include "RHI.h"
#include "RHIDefinitions.h"
#include "Misc/ConfigCacheIni.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"

UGraphicsQualityManager::UGraphicsQualityManager()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGraphicsQualityManager::BeginPlay()
{
	Super::BeginPlay();
	if (bAutoDetectAndApply)
		DetectAndApply();
}

// ─── Detection ────────────────────────────────────────────────────────────────

void UGraphicsQualityManager::DetectAndApply()
{
	if (bUseForcePreset)
	{
		AppliedPreset = ForcedPreset;
		ApplyPreset(AppliedPreset);
		return;
	}

	DetectedVendor = DetectGPUVendor();
	bHardwareRayTracingAvailable = CheckHardwareRTSupport();

	AppliedPreset = VendorToPreset(DetectedVendor);
	ApplyPreset(AppliedPreset);

	UE_LOG(LogTemp, Log,
		TEXT("GraphicsQualityManager: GPU='%s' Vendor=%d HW-RT=%s Preset=%d"),
		*DetectedGPUDescription,
		static_cast<int32>(DetectedVendor),
		bHardwareRayTracingAvailable ? TEXT("YES") : TEXT("NO"),
		static_cast<int32>(AppliedPreset));

	OnPresetApplied(AppliedPreset, DetectedVendor);
}

EGPUVendor UGraphicsQualityManager::DetectGPUVendor()
{
	// RHI exposes the adapter description string
	FString AdapterName;

#if PLATFORM_WINDOWS || PLATFORM_LINUX
	if (GRHIAdapterName.Len() > 0)
	{
		DetectedGPUDescription = GRHIAdapterName;
		AdapterName = GRHIAdapterName.ToUpper();
	}
	else
	{
		DetectedGPUDescription = TEXT("Unknown GPU");
		return EGPUVendor::Unknown;
	}
#else
	DetectedGPUDescription = TEXT("Platform GPU");
	return EGPUVendor::Unknown;
#endif

	// ── NVIDIA ────────────────────────────────────────────────────────────────
	if (AdapterName.Contains(TEXT("NVIDIA")) || AdapterName.Contains(TEXT("GEFORCE")))
	{
		// RTX series: RTX 20xx, 30xx, 40xx (hardware RT support)
		if (AdapterName.Contains(TEXT("RTX")))
			return EGPUVendor::NVIDIA_RTX;

		// GTX 10xx/16xx: DX12 capable but no hardware RT
		return EGPUVendor::NVIDIA_GTX;
	}

	// ── AMD ───────────────────────────────────────────────────────────────────
	if (AdapterName.Contains(TEXT("AMD")) || AdapterName.Contains(TEXT("RADEON"))
		|| AdapterName.Contains(TEXT("ADVANCED MICRO")))
	{
		// RDNA2: RX 6xxx series — supports hardware RT + mesh shaders
		if (AdapterName.Contains(TEXT("RX 6")) || AdapterName.Contains(TEXT("RX6"))
			|| AdapterName.Contains(TEXT("RX 7")) || AdapterName.Contains(TEXT("RX7")))
		{
			return EGPUVendor::AMD_RDNA2;
		}

		// RDNA1: RX 5xxx series — DX12, no hardware RT, limited mesh shaders
		if (AdapterName.Contains(TEXT("RX 5")) || AdapterName.Contains(TEXT("RX5")))
			return EGPUVendor::AMD_RDNA1;

		// Older AMD (Vega, Polaris) — DX12 but legacy
		return EGPUVendor::AMD_Legacy;
	}

	// ── Intel Arc ─────────────────────────────────────────────────────────────
	if (AdapterName.Contains(TEXT("INTEL")) && AdapterName.Contains(TEXT("ARC")))
		return EGPUVendor::Intel_Arc;

	return EGPUVendor::Other;
}

bool UGraphicsQualityManager::CheckHardwareRTSupport() const
{
#if PLATFORM_WINDOWS
	// UE5 exposes whether the current RHI supports ray tracing
	return GRHISupportsRayTracing && GRHISupportsRayTracingShaders;
#else
	return false;
#endif
}

EQualityPreset UGraphicsQualityManager::VendorToPreset(EGPUVendor Vendor) const
{
	switch (Vendor)
	{
		case EGPUVendor::NVIDIA_RTX:
			return bHardwareRayTracingAvailable
				? EQualityPreset::Cinematic
				: EQualityPreset::High;

		case EGPUVendor::AMD_RDNA2:
		case EGPUVendor::Intel_Arc:
			return EQualityPreset::High;

		case EGPUVendor::AMD_RDNA1:
		case EGPUVendor::NVIDIA_GTX:
			return EQualityPreset::Medium;

		case EGPUVendor::AMD_Legacy:
		case EGPUVendor::Other:
		default:
			return EQualityPreset::Low;
	}
}

// ─── Preset Application ───────────────────────────────────────────────────────

void UGraphicsQualityManager::ApplyPreset(EQualityPreset Preset)
{
	switch (Preset)
	{
		case EQualityPreset::Cinematic: ApplyCinematicPreset(); break;
		case EQualityPreset::High:      ApplyHighPreset();      break;
		case EQualityPreset::Medium:    ApplyMediumPreset();    break;
		case EQualityPreset::Low:       ApplyLowPreset();       break;
	}
}

// NVIDIA RTX — full hardware ray tracing, Lumen HW path, path tracing
void UGraphicsQualityManager::ApplyCinematicPreset()
{
	// Hardware Ray Tracing
	ExecConsoleCommand(TEXT("r.RayTracing 1"));
	ExecConsoleCommand(TEXT("r.RayTracing.ForceAllRayTracingEffects 1"));

	// Lumen — hardware RT mode
	ExecConsoleCommand(TEXT("r.Lumen.HardwareRayTracing 1"));
	ExecConsoleCommand(TEXT("r.Lumen.Reflections.Allow 1"));
	ExecConsoleCommand(TEXT("r.Lumen.GI.Allow 1"));
	ExecConsoleCommand(TEXT("r.Lumen.Reflections.UseSoftwareRayTracing 0"));
	ExecConsoleCommand(TEXT("r.Lumen.GI.UseSoftwareRayTracing 0"));

	// Shadows — Virtual Shadow Maps
	ExecConsoleCommand(TEXT("r.Shadow.Virtual.Enable 1"));
	ExecConsoleCommand(TEXT("r.ShadowQuality 5"));

	// Nanite
	ExecConsoleCommand(TEXT("r.Nanite 1"));

	// Post process
	ExecConsoleCommand(TEXT("r.MotionBlurQuality 4"));
	ExecConsoleCommand(TEXT("r.BloomQuality 5"));
	ExecConsoleCommand(TEXT("r.DepthOfFieldQuality 4"));
	ExecConsoleCommand(TEXT("r.AmbientOcclusionLevels 3"));

	// Texture streaming
	ExecConsoleCommand(TEXT("r.Streaming.PoolSize 4096"));

	// Anti-aliasing — TSR (Temporal Super Resolution) for best quality
	ExecConsoleCommand(TEXT("r.AntiAliasingMethod 4")); // TSR

	// HDR
	ExecConsoleCommand(TEXT("r.HDR.EnableHDROutput 1"));

	UE_LOG(LogTemp, Log, TEXT("GraphicsQualityManager: Applied CINEMATIC preset (NVIDIA RTX HW-RT)"));
}

// AMD RDNA2+ — Software Lumen ray tracing, no hardware RT CVars
void UGraphicsQualityManager::ApplyHighPreset()
{
	// Disable hardware ray tracing (not reliable on AMD pre-RDNA3 in UE5)
	ExecConsoleCommand(TEXT("r.RayTracing 0"));

	// Lumen — software ray tracing path (works well on RDNA2+)
	ExecConsoleCommand(TEXT("r.Lumen.HardwareRayTracing 0"));
	ExecConsoleCommand(TEXT("r.Lumen.Reflections.Allow 1"));
	ExecConsoleCommand(TEXT("r.Lumen.GI.Allow 1"));
	ExecConsoleCommand(TEXT("r.Lumen.Reflections.UseSoftwareRayTracing 1"));
	ExecConsoleCommand(TEXT("r.Lumen.GI.UseSoftwareRayTracing 1"));

	// Software Lumen quality settings
	ExecConsoleCommand(TEXT("r.Lumen.ScreenProbeGather.RadianceCache.NumMipmaps 4"));
	ExecConsoleCommand(TEXT("r.Lumen.Reflections.ScreenSpaceReconstruction 1"));

	// Shadows — Virtual Shadow Maps (supported on RDNA2+)
	ExecConsoleCommand(TEXT("r.Shadow.Virtual.Enable 1"));
	ExecConsoleCommand(TEXT("r.ShadowQuality 4"));

	// Nanite
	ExecConsoleCommand(TEXT("r.Nanite 1"));

	// Post process
	ExecConsoleCommand(TEXT("r.MotionBlurQuality 3"));
	ExecConsoleCommand(TEXT("r.BloomQuality 4"));
	ExecConsoleCommand(TEXT("r.DepthOfFieldQuality 3"));

	// Anti-aliasing — TSR works on DX12 without HW-RT
	ExecConsoleCommand(TEXT("r.AntiAliasingMethod 4")); // TSR

	// Texture streaming
	ExecConsoleCommand(TEXT("r.Streaming.PoolSize 2048"));

	UE_LOG(LogTemp, Log, TEXT("GraphicsQualityManager: Applied HIGH preset (AMD RDNA2 Software Lumen)"));
}

// AMD RDNA1 / NVIDIA GTX — Screen space GI fallback, standard shadows
void UGraphicsQualityManager::ApplyMediumPreset()
{
	ExecConsoleCommand(TEXT("r.RayTracing 0"));

	// Disable hardware Lumen RT; use screen space fallback
	ExecConsoleCommand(TEXT("r.Lumen.HardwareRayTracing 0"));
	ExecConsoleCommand(TEXT("r.Lumen.Reflections.Allow 1"));
	ExecConsoleCommand(TEXT("r.Lumen.GI.Allow 1"));
	ExecConsoleCommand(TEXT("r.Lumen.Reflections.UseSoftwareRayTracing 0")); // Screen space
	ExecConsoleCommand(TEXT("r.Lumen.GI.UseSoftwareRayTracing 0"));

	// Standard cascade shadows (VSM may have issues on RDNA1)
	ExecConsoleCommand(TEXT("r.Shadow.Virtual.Enable 0"));
	ExecConsoleCommand(TEXT("r.ShadowQuality 3"));

	// Nanite — RDNA1 supports it via DX12
	ExecConsoleCommand(TEXT("r.Nanite 1"));

	// Lower post-process quality
	ExecConsoleCommand(TEXT("r.MotionBlurQuality 2"));
	ExecConsoleCommand(TEXT("r.BloomQuality 3"));
	ExecConsoleCommand(TEXT("r.DepthOfFieldQuality 2"));

	// TAA instead of TSR for better compatibility
	ExecConsoleCommand(TEXT("r.AntiAliasingMethod 2")); // TAA

	ExecConsoleCommand(TEXT("r.Streaming.PoolSize 1024"));

	UE_LOG(LogTemp, Log, TEXT("GraphicsQualityManager: Applied MEDIUM preset (AMD RDNA1 / GTX)"));
}

// Legacy DX12 fallback — minimal settings
void UGraphicsQualityManager::ApplyLowPreset()
{
	ExecConsoleCommand(TEXT("r.RayTracing 0"));
	ExecConsoleCommand(TEXT("r.Lumen.HardwareRayTracing 0"));
	ExecConsoleCommand(TEXT("r.Lumen.Reflections.Allow 0")); // Disable Lumen entirely
	ExecConsoleCommand(TEXT("r.Lumen.GI.Allow 0"));

	// SSAO as GI fallback
	ExecConsoleCommand(TEXT("r.AmbientOcclusionLevels 2"));

	ExecConsoleCommand(TEXT("r.Shadow.Virtual.Enable 0"));
	ExecConsoleCommand(TEXT("r.ShadowQuality 2"));
	ExecConsoleCommand(TEXT("r.Nanite 0")); // Disable Nanite on legacy hardware

	ExecConsoleCommand(TEXT("r.MotionBlurQuality 1"));
	ExecConsoleCommand(TEXT("r.BloomQuality 2"));
	ExecConsoleCommand(TEXT("r.DepthOfFieldQuality 1"));
	ExecConsoleCommand(TEXT("r.AntiAliasingMethod 2")); // TAA

	ExecConsoleCommand(TEXT("r.Streaming.PoolSize 512"));

	UE_LOG(LogTemp, Log, TEXT("GraphicsQualityManager: Applied LOW preset (Legacy DX12 fallback)"));
}

// ─── Summary ─────────────────────────────────────────────────────────────────

FString UGraphicsQualityManager::GetSettingsSummary() const
{
	FString VendorStr;
	switch (DetectedVendor)
	{
		case EGPUVendor::NVIDIA_RTX:  VendorStr = TEXT("NVIDIA RTX"); break;
		case EGPUVendor::NVIDIA_GTX:  VendorStr = TEXT("NVIDIA GTX"); break;
		case EGPUVendor::AMD_RDNA2:   VendorStr = TEXT("AMD RDNA2+"); break;
		case EGPUVendor::AMD_RDNA1:   VendorStr = TEXT("AMD RDNA1");  break;
		case EGPUVendor::AMD_Legacy:  VendorStr = TEXT("AMD Legacy"); break;
		case EGPUVendor::Intel_Arc:   VendorStr = TEXT("Intel Arc");  break;
		default:                      VendorStr = TEXT("Unknown");    break;
	}

	FString PresetStr;
	switch (AppliedPreset)
	{
		case EQualityPreset::Cinematic: PresetStr = TEXT("Cinematic (HW RT)"); break;
		case EQualityPreset::High:      PresetStr = TEXT("High (SW Lumen)");   break;
		case EQualityPreset::Medium:    PresetStr = TEXT("Medium (SS GI)");    break;
		case EQualityPreset::Low:       PresetStr = TEXT("Low (SSAO)");        break;
	}

	return FString::Printf(
		TEXT("GPU: %s | Vendor: %s | HW-RT: %s | Preset: %s"),
		*DetectedGPUDescription, *VendorStr,
		bHardwareRayTracingAvailable ? TEXT("Yes") : TEXT("No"),
		*PresetStr);
}

void UGraphicsQualityManager::ExecConsoleCommand(const FString& Cmd)
{
	if (IConsoleManager::Get().FindConsoleObject(*Cmd.Left(Cmd.Find(TEXT(" ")))))
	{
		// Parse "cvar value" format
		GEngine->Exec(nullptr, *Cmd);
	}
	else
	{
		GEngine->Exec(nullptr, *Cmd);
	}
}
