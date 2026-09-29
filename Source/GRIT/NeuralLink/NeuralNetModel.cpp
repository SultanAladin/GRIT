#include "NeuralNetModel.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformMath.h"

// --- Activation ---------------------------------------------------------------

ENNActivation FNeuralNetModel::ParseActivation(const FString& S)
{
	const FString L = S.ToLower().TrimStartAndEnd();
	if (L == TEXT("relu"))    return ENNActivation::ReLU;
	if (L == TEXT("tanh"))    return ENNActivation::Tanh;
	if (L == TEXT("sigmoid")) return ENNActivation::Sigmoid;
	return ENNActivation::None;
}

const TCHAR* FNeuralNetModel::ActivationName(ENNActivation A)
{
	switch (A)
	{
	case ENNActivation::ReLU:    return TEXT("relu");
	case ENNActivation::Tanh:    return TEXT("tanh");
	case ENNActivation::Sigmoid: return TEXT("sigmoid");
	default:                     return TEXT("none");
	}
}

void FNeuralNetModel::ApplyActivation(float* X, int32 N, ENNActivation A)
{
	switch (A)
	{
	case ENNActivation::ReLU:
		for (int32 i = 0; i < N; ++i) X[i] = X[i] > 0.0f ? X[i] : 0.0f;
		break;
	case ENNActivation::Tanh:
		for (int32 i = 0; i < N; ++i) X[i] = FMath::Tanh(X[i]);
		break;
	case ENNActivation::Sigmoid:
		for (int32 i = 0; i < N; ++i) X[i] = 1.0f / (1.0f + FMath::Exp(-X[i]));
		break;
	default: break;
	}
}

// --- Inference ----------------------------------------------------------------

void FNeuralNetModel::ForwardRaw(const float* In, float* Out) const
{
	if (Layers.Num() == 0) return;

	// Determine max layer output to size scratch. Cheap — done once if sizes stable.
	int32 MaxW = 0;
	for (const FNNLayer& L : Layers) MaxW = FMath::Max(MaxW, L.OutDim);
	if (ScratchA.Num() < MaxW) ScratchA.SetNumUninitialized(MaxW, EAllowShrinking::No);
	if (ScratchB.Num() < MaxW) ScratchB.SetNumUninitialized(MaxW, EAllowShrinking::No);

	const float* Src = In;
	float* Dst = ScratchA.GetData();
	float* Alt = ScratchB.GetData();

	for (int32 Li = 0; Li < Layers.Num(); ++Li)
	{
		const FNNLayer& L = Layers[Li];
		const bool bLast = (Li == Layers.Num() - 1);
		float* Write = bLast ? Out : Dst;

		const float* W = L.Weights.GetData();
		const float* B = L.Biases.GetData();

		// Write[i] = sum_j W[i*InDim + j] * Src[j] + B[i]
		for (int32 i = 0; i < L.OutDim; ++i)
		{
			float Acc = B[i];
			const float* Wrow = W + i * L.InDim;
			for (int32 j = 0; j < L.InDim; ++j)
			{
				Acc += Wrow[j] * Src[j];
			}
			Write[i] = Acc;
		}

		ENNActivation Act = bLast ? OutputActivation : HiddenActivation;
		ApplyActivation(Write, L.OutDim, Act);

		if (!bLast)
		{
			Src = Dst;
			// Swap scratch for next layer
			float* Tmp = Dst; Dst = Alt; Alt = Tmp;
		}
	}
}

void FNeuralNetModel::Forward(const TArray<float>& In, TArray<float>& Out) const
{
	if (!IsValid() || In.Num() != InputDim)
	{
		Out.Reset();
		return;
	}
	Out.SetNumUninitialized(OutputDim, EAllowShrinking::No);
	ForwardRaw(In.GetData(), Out.GetData());
}

// --- Parser -------------------------------------------------------------------

// Small helper: parse whitespace-separated floats into Out, expecting exactly N.
static bool ParseFloats(const TArray<FString>& Tokens, int32 Expected, TArray<float>& Out, FString* Err)
{
	Out.Reset(Expected);
	for (const FString& T : Tokens)
	{
		if (T.IsEmpty()) continue;
		Out.Add(FCString::Atof(*T));
	}
	if (Out.Num() != Expected)
	{
		if (Err) *Err = FString::Printf(TEXT("expected %d floats, got %d"), Expected, Out.Num());
		return false;
	}
	return true;
}

bool FNeuralNetModel::LoadFromFile(const FString& FilePath, FString* OutError)
{
	FString Contents;
	if (!FFileHelper::LoadFileToString(Contents, *FilePath))
	{
		if (OutError) *OutError = FString::Printf(TEXT("cannot read %s"), *FilePath);
		return false;
	}
	return LoadFromString(Contents, OutError);
}

bool FNeuralNetModel::LoadFromString(const FString& Contents, FString* OutError)
{
	Layers.Reset();
	Name.Reset();
	InputDim = OutputDim = 0;
	TrainedSteps = 0;
	RewardMean = 0.0f;
	HiddenActivation = ENNActivation::ReLU;
	OutputActivation = ENNActivation::Tanh;

	TArray<FString> Lines;
	Contents.ParseIntoArrayLines(Lines, /*InCullEmpty=*/false);

	int32 Idx = 0;
	auto TrimCopy = [](const FString& S) { return S.TrimStartAndEnd(); };

	// ---- frontmatter ------------------------------------------------------
	while (Idx < Lines.Num() && Lines[Idx].TrimStartAndEnd().IsEmpty()) ++Idx;
	if (Idx >= Lines.Num() || TrimCopy(Lines[Idx]) != TEXT("---"))
	{
		if (OutError) *OutError = TEXT("missing '---' frontmatter opener");
		return false;
	}
	++Idx;
	while (Idx < Lines.Num() && TrimCopy(Lines[Idx]) != TEXT("---"))
	{
		FString Line = TrimCopy(Lines[Idx++]);
		if (Line.IsEmpty() || Line.StartsWith(TEXT("#"))) continue;
		int32 ColonAt;
		if (!Line.FindChar(':', ColonAt)) continue;
		FString Key = TrimCopy(Line.Left(ColonAt)).ToLower();
		FString Val = TrimCopy(Line.Mid(ColonAt + 1));

		if      (Key == TEXT("name"))              Name = Val;
		else if (Key == TEXT("inputs"))            InputDim  = FCString::Atoi(*Val);
		else if (Key == TEXT("outputs"))           OutputDim = FCString::Atoi(*Val);
		else if (Key == TEXT("activation"))        HiddenActivation = ParseActivation(Val);
		else if (Key == TEXT("output_activation")) OutputActivation = ParseActivation(Val);
		else if (Key == TEXT("trained_steps"))     TrainedSteps = FCString::Atoi(*Val);
		else if (Key == TEXT("reward_mean"))       RewardMean   = FCString::Atof(*Val);
	}
	if (Idx >= Lines.Num())
	{
		if (OutError) *OutError = TEXT("missing '---' frontmatter closer");
		return false;
	}
	++Idx; // consume closing ---

	if (InputDim <= 0 || OutputDim <= 0)
	{
		if (OutError) *OutError = TEXT("inputs/outputs must be > 0 in frontmatter");
		return false;
	}

	// ---- layers -----------------------------------------------------------
	// Each layer starts with `# Layer <i>` (may also contain '<in> -> <out>').
	// Expected blocks inside: `## weights [OUTxIN]` then float lines, then
	// `## biases [OUT]` then one float line.
	FNNLayer* Current = nullptr;
	enum class EBlock { None, Weights, Biases } Block = EBlock::None;
	TArray<FString> PendingTokens;
	int32 ExpectedFloats = 0;

	auto FinalizeBlock = [&](FString* Err) -> bool
	{
		if (!Current || Block == EBlock::None) return true;
		TArray<float> Parsed;
		if (!ParseFloats(PendingTokens, ExpectedFloats, Parsed, Err)) return false;
		if (Block == EBlock::Weights) Current->Weights = MoveTemp(Parsed);
		else                          Current->Biases  = MoveTemp(Parsed);
		PendingTokens.Reset();
		Block = EBlock::None;
		ExpectedFloats = 0;
		return true;
	};

	for (; Idx < Lines.Num(); ++Idx)
	{
		FString Raw = Lines[Idx];
		FString Line = TrimCopy(Raw);
		if (Line.IsEmpty()) continue;

		if (Line.StartsWith(TEXT("## weights")) || Line.StartsWith(TEXT("## biases")))
		{
			FString BlockErr;
			if (!FinalizeBlock(&BlockErr))
			{
				if (OutError) *OutError = FString::Printf(TEXT("layer %d block parse: %s"),
					Layers.Num() - 1, *BlockErr);
				return false;
			}
			if (!Current)
			{
				if (OutError) *OutError = TEXT("weights/biases block before any '# Layer' header");
				return false;
			}
			Block = Line.StartsWith(TEXT("## weights")) ? EBlock::Weights : EBlock::Biases;
			ExpectedFloats = (Block == EBlock::Weights) ? (Current->InDim * Current->OutDim) : Current->OutDim;
			continue;
		}

		if (Line.StartsWith(TEXT("# Layer")) || Line.StartsWith(TEXT("# layer")))
		{
			FString BlockErr;
			if (!FinalizeBlock(&BlockErr))
			{
				if (OutError) *OutError = FString::Printf(TEXT("layer %d: %s"),
					Layers.Num() - 1, *BlockErr);
				return false;
			}

			// Parse "# Layer N: IN -> OUT"
			int32 Colon;
			int32 ParsedIn = 0, ParsedOut = 0;
			if (Line.FindChar(':', Colon))
			{
				FString After = TrimCopy(Line.Mid(Colon + 1));
				FString L, R;
				if (After.Split(TEXT("->"), &L, &R))
				{
					ParsedIn  = FCString::Atoi(*TrimCopy(L));
					ParsedOut = FCString::Atoi(*TrimCopy(R));
				}
			}
			if (ParsedIn <= 0 || ParsedOut <= 0)
			{
				// Infer from previous layer / frontmatter
				ParsedIn  = (Layers.Num() == 0) ? InputDim : Layers.Last().OutDim;
				ParsedOut = OutputDim;
			}

			FNNLayer NewLayer;
			NewLayer.InDim  = ParsedIn;
			NewLayer.OutDim = ParsedOut;
			Layers.Add(MoveTemp(NewLayer));
			Current = &Layers.Last();
			Block = EBlock::None;
			continue;
		}

		if (Line.StartsWith(TEXT("#"))) continue; // other comments

		// Otherwise: tokens for current block
		if (Block != EBlock::None)
		{
			TArray<FString> Toks;
			Line.ParseIntoArray(Toks, TEXT(" "), /*InCullEmpty=*/true);
			for (const FString& T : Toks)
			{
				FString Tt = TrimCopy(T);
				if (!Tt.IsEmpty()) PendingTokens.Add(Tt);
			}
		}
	}

	FString BlockErr;
	if (!FinalizeBlock(&BlockErr))
	{
		if (OutError) *OutError = FString::Printf(TEXT("final layer: %s"), *BlockErr);
		return false;
	}

	// ---- validation -------------------------------------------------------
	if (Layers.Num() == 0)
	{
		if (OutError) *OutError = TEXT("no layers parsed");
		return false;
	}
	if (Layers[0].InDim != InputDim)
	{
		if (OutError) *OutError = FString::Printf(TEXT("layer 0 InDim=%d != inputs=%d"),
			Layers[0].InDim, InputDim);
		return false;
	}
	if (Layers.Last().OutDim != OutputDim)
	{
		if (OutError) *OutError = FString::Printf(TEXT("last layer OutDim=%d != outputs=%d"),
			Layers.Last().OutDim, OutputDim);
		return false;
	}
	for (int32 i = 1; i < Layers.Num(); ++i)
	{
		if (Layers[i].InDim != Layers[i - 1].OutDim)
		{
			if (OutError) *OutError = FString::Printf(TEXT("layer %d InDim=%d != prev OutDim=%d"),
				i, Layers[i].InDim, Layers[i - 1].OutDim);
			return false;
		}
		if (Layers[i].Weights.Num() != Layers[i].InDim * Layers[i].OutDim ||
		    Layers[i].Biases.Num()  != Layers[i].OutDim)
		{
			if (OutError) *OutError = FString::Printf(TEXT("layer %d weight/bias size mismatch"), i);
			return false;
		}
	}

	return true;
}

// --- Writer -------------------------------------------------------------------

FString FNeuralNetModel::ToString() const
{
	FString S;
	S.Reserve(Layers.Num() * 4096);

	S += TEXT("---\n");
	S += FString::Printf(TEXT("name: %s\n"),              Name.IsEmpty() ? TEXT("unnamed") : *Name);
	S += TEXT("type: mlp\n");
	S += FString::Printf(TEXT("inputs: %d\n"),            InputDim);
	S += FString::Printf(TEXT("outputs: %d\n"),           OutputDim);
	S += FString::Printf(TEXT("activation: %s\n"),        ActivationName(HiddenActivation));
	S += FString::Printf(TEXT("output_activation: %s\n"), ActivationName(OutputActivation));
	S += FString::Printf(TEXT("trained_steps: %d\n"),     TrainedSteps);
	S += FString::Printf(TEXT("reward_mean: %.4f\n"),     RewardMean);
	S += TEXT("---\n\n");

	for (int32 Li = 0; Li < Layers.Num(); ++Li)
	{
		const FNNLayer& L = Layers[Li];
		S += FString::Printf(TEXT("# Layer %d: %d -> %d\n"), Li, L.InDim, L.OutDim);

		S += FString::Printf(TEXT("## weights [%dx%d]\n"), L.OutDim, L.InDim);
		for (int32 r = 0; r < L.OutDim; ++r)
		{
			for (int32 c = 0; c < L.InDim; ++c)
			{
				S += FString::Printf(TEXT("%.6f"), L.Weights[r * L.InDim + c]);
				S += (c + 1 < L.InDim) ? TEXT(" ") : TEXT("\n");
			}
		}

		S += FString::Printf(TEXT("## biases [%d]\n"), L.OutDim);
		for (int32 i = 0; i < L.OutDim; ++i)
		{
			S += FString::Printf(TEXT("%.6f"), L.Biases[i]);
			S += (i + 1 < L.OutDim) ? TEXT(" ") : TEXT("\n");
		}
		S += TEXT("\n");
	}
	return S;
}

bool FNeuralNetModel::SaveToFile(const FString& FilePath) const
{
	return FFileHelper::SaveStringToFile(ToString(), *FilePath);
}
