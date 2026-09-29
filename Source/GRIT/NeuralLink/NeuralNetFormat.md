# GRIT Neural Net File Format

Tiny human-readable format for shipping MLP weights. Used by
`FNeuralNetModel::LoadFromFile` / `SaveToFile` for in-process inference on
the CPU. Designed for 2-4 layer nets with a few thousand weights.

## Why a custom format

ONNX / SafeTensors / GGUF are all built for large networks with rich layer
graphs. A GRIT creature policy is a 24→64→32→6 MLP — roughly 3,648 weights.
A readable text format is small, diff-friendly, hand-editable for priors,
and needs no runtime dependency.

## File layout

```markdown
---
name: scorpion_v1
type: mlp
inputs: 24
outputs: 6
activation: relu              # relu | tanh | sigmoid | none — between hidden layers
output_activation: tanh       # applied to the final layer
trained_steps: 2048000
reward_mean: 12.4
---

# Layer 0: 24 -> 64
## weights [64x24]
0.234 -0.117 0.045 ... (24 floats per row, 64 rows — may span lines)
## biases [64]
0.012 -0.003 ... (64 floats)

# Layer 1: 64 -> 32
## weights [32x64]
...
## biases [32]
...

# Layer 2: 32 -> 6
## weights [6x32]
...
## biases [6]
...
```

## Rules

- Frontmatter between `---` lines. YAML-ish `key: value`.
- `activation` applies between hidden layers. `output_activation` applies only
  to the final layer output.
- Each layer starts with `# Layer N: IN -> OUT`. Sizes are inferred from the
  previous layer if omitted, but being explicit is safer.
- `## weights [OUTxIN]` is row-major: each row is one output neuron's weights
  across all inputs. Whitespace-separated floats; newlines are not significant
  within a block.
- `## biases [OUT]` is one-dimensional.
- Lines starting with `#` that aren't headers are treated as comments.

## Error handling

The loader validates:
- first layer InDim matches `inputs`
- last layer OutDim matches `outputs`
- adjacent layers are shape-compatible
- weight/bias counts equal declared shapes

Any mismatch fails the load with a descriptive error — the caller falls back
to the server path or a scripted FSM.

## Writing from a trainer

CortexServer (external trainer) should export using this same layout. Keeping
the format text-based means weight files can be hand-edited to inject priors
or bias a creature toward a behavior before RL refinement.
