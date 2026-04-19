// Copyright 2024 BlackHoleGame. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GraphicsQualityManager.generated.h"

/**
 * GPU vendor category detected at runtime.
 */
UENUM(BlueprintType)
enum class EGPUVendor : uint8
{
	Unknown      UMETA(DisplayName = "Unknown"),
	NVIDIA_RTX   UMETA(DisplayName = "NVIDIA RTX (HW Ray Tracing)"),
	NVIDIA_GTX   UMETA(DisplayName = "NVIDIA GTX (No HW RT)"),
	AMD_RDNA2    UMETA(DisplayName = "AMD RDNA2+ (Software RT)"),
	AMD_RDNA1    UMETA(DisplayName = "AMD RDNA1 (Reduced Lumen)"),
	AMD_Legacy   UMETA(DisplayName = "AMD Legacy DX12"),
	Intel_Arc    UMETA(DisplayName = "Intel Arc"),
	Other        UMETA(DisplayName = "Other DX12")
};

/**
 * Quality preset driven by GPU detection.
 */
UENUM(BlueprintType)
enum class EQualityPreset : uint8
{
	Cinematic    UMETA(DisplayName = "Cinematic (NVIDIA RTX / HW RT)"),
	High         UMETA(DisplayName = "High (AMD RDNA2 / SW Lumen)"),
	Medium       UMETA(DisplayName = "Medium (AMD RDNA1 / No Lumen)"),
	Low          UMETA(DisplayName = "Low (Fallback DX12)")
};

/**
 * UGraphicsQualityManager
 *
 * Auto-detects GPU vendor at startup using RHI device information and applies
 * appropriate rendering CVars for the black hole game.
 *
 * Attach to ABlackHoleGameMode or place in the level's default actor.
 *
 * Rendering paths:
 *  NVIDIA RTX   → HW Ray Tracing + Full Lumen + Path Tracing available
 *  AMD RDNA2+   → Software Lumen RT + VSM Shadows + No HW RT
 *  AMD RDNA1    → Reduced Lumen (Screen Space fallback) + Standard Shadows
 *  Other DX12   → Conservative settings, no Lumen RT
 *
 * All shaders use standard HLSL only — no NvAPI, no vendor intrinsics.
 */
UCLASS(ClassGroup=(BlackHole), meta=(BlueprintSpawnableComponent),
	DisplayName="Graphics Quality Manager")
class BLACKHOLEGAME_API UGraphicsQualityManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UGraphicsQualityManager();

protected:
	virtual void BeginPlay() override;

public:
	// ── Configuration ────────────────────────────────────────────────────────

	/** Allow the manager to override quality settings automatically */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graphics|Auto")
	bool bAutoDetectAndApply = true;

	/** Force a specific preset regardless of detected GPU (None = auto) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graphics|Override")
	EQualityPreset ForcedPreset = EQualityPreset::Cinematic;

	/** Whether ForcedPreset is used (overrides auto-detection) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graphics|Override")
	bool bUseForcePreset = false;

	// ── Runtime State ─────────────────────────────────────────────────────────

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Graphics|Detected")
	EGPUVendor DetectedVendor = EGPUVendor::Unknown;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Graphics|Detected")
	EQualityPreset AppliedPreset = EQualityPreset::High;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Graphics|Detected")
	FString DetectedGPUDescription;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Graphics|Detected")
	bool bHardwareRayTracingAvailable = false;

	// ── Public API ────────────────────────────────────────────────────────────

	/** Re-run detection and apply settings */
	UFUNCTION(BlueprintCallable, Category = "Graphics")
	void DetectAndApply();

	/** Manually apply a preset */
	UFUNCTION(BlueprintCallable, Category = "Graphics")
	void ApplyPreset(EQualityPreset Preset);

	/** Returns a human-readable summary of applied settings */
	UFUNCTION(BlueprintCallable, Category = "Graphics")
	FString GetSettingsSummary() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Graphics")
	void OnPresetApplied(EQualityPreset Preset, EGPUVendor Vendor);

private:
	EGPUVendor DetectGPUVendor();
	EQualityPreset VendorToPreset(EGPUVendor Vendor) const;
	bool CheckHardwareRTSupport() const;

	void ApplyCinematicPreset();
	void ApplyHighPreset();
	void ApplyMediumPreset();
	void ApplyLowPreset();

	static void ExecConsoleCommand(const FString& Cmd);
};
