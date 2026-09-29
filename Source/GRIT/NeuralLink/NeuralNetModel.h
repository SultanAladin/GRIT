#pragma once
// NeuralNetModel.h — Tiny in-process MLP for runtime inference.
//
// Format: human-readable .md with YAML frontmatter + matrix blocks.
// Designed for 2-4 layer nets, <64k weights. Loads once, runs on CPU.
// No dependency on CortexServer — shippable.
//
// File layout (see Docs/NeuralNetFormat.md for full spec):
//
//   ---
//   name: scorpion_v1
//   type: mlp
//   inputs: 24
//   outputs: 6
//   activation: relu         # relu | tanh | none  (applied between hidden layers)
//   output_activation: tanh  # relu | tanh | sigmoid | none
//   trained_steps: 2048000
//   reward_mean: 12.4
//   ---
//
//   # Layer 0: 24 -> 64
//   ## weights [64x24]
//   0.234 -0.117 0.045 ...
//   ## biases [64]
//   0.012 -0.003 ...
//
//   # Layer 1: 64 -> 32
//   ...
//
//   # Layer 2: 32 -> 6
//   ...

#include "CoreMinimal.h"

enum class ENNActivation : uint8
{
	None,
	ReLU,
	Tanh,
	Sigmoid,
};

struct FNNLayer
{
	int32 InDim = 0;
	int32 OutDim = 0;
	TArray<float> Weights;  // row-major, OutDim * InDim
	TArray<float> Biases;   // OutDim
};

class GRIT_API FNeuralNetModel
{
public:
	// Metadata (populated from frontmatter)
	FString Name;
	int32 InputDim  = 0;
	int32 OutputDim = 0;
	ENNActivation HiddenActivation = ENNActivation::ReLU;
	ENNActivation OutputActivation = ENNActivation::Tanh;
	int32 TrainedSteps = 0;
	float RewardMean  = 0.0f;

	TArray<FNNLayer> Layers;

	bool IsValid() const { return Layers.Num() > 0 && InputDim > 0 && OutputDim > 0; }

	// --- I/O ---------------------------------------------------------------
	// Returns true on success. OutError is human-readable.
	bool LoadFromFile(const FString& FilePath, FString* OutError = nullptr);
	bool LoadFromString(const FString& Contents, FString* OutError = nullptr);
	bool SaveToFile(const FString& FilePath) const;
	FString ToString() const;

	// --- Inference ---------------------------------------------------------
	// In size must equal InputDim, Out is resized to OutputDim.
	// Thread-safe as long as no other thread is mutating the model.
	void Forward(const TArray<float>& In, TArray<float>& Out) const;

	// Raw pointer form — hot path; caller guarantees sizes.
	void ForwardRaw(const float* In, float* Out) const;

private:
	// Scratch buffers, reused across calls. Mutable so Forward() can stay const.
	mutable TArray<float> ScratchA;
	mutable TArray<float> ScratchB;

	static ENNActivation ParseActivation(const FString& S);
	static const TCHAR* ActivationName(ENNActivation A);
	static void ApplyActivation(float* X, int32 N, ENNActivation A);
};
