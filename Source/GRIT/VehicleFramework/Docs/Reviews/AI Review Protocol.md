AI Code Review Protocol for SolvePowertrain
Document Purpose
This protocol defines the standardized format and methodology for AI-assisted code reviews of physics simulation code. Multiple AI models can reference this document to produce consistent, comparable reviews of the same codebase.

📁 Directory Structure
Projects/GRIT/Source/GRIT/VehicleFramework/Docs/
│
├── CoreReview/                          # Main review repository
│   ├── SolvePowertrain_Review.md       # Primary comprehensive review
│   ├── ReviewProtocol.md               # This document
│   └── ReviewHistory.md                # Changelog of all reviews
│
├── ModelReviews/                        # Individual AI model reviews
│   ├── Claude_Sonnet_4.5_20251129.md
│   ├── GPT4_20251130.md
│   └── [ModelName]_[Date].md
│
└── Verification/                        # Mathematical verification
    ├── EquationVerification.md
    ├── UnitAnalysis.md
    └── NumericalTests.md

🎯 Review Objectives

Mathematical Correctness: Verify all equations against established literature
Unit Consistency: Ensure dimensional analysis is correct throughout
Physics Validation: Confirm physical principles are correctly applied
Code Quality: Assess implementation quality, magic numbers, and maintainability
Performance: Evaluate computational efficiency and numerical stability


📋 Review Format Template
File Header
markdown# [Function/Pass Name] Engineering Review

**File**: `VehicleSolver.cpp` (Lines [START]–[END])
**Reviewer**: [AI Model Name] ([Version])
**Review Date**: [YYYY-MM-DD]
**Review ID**: [UNIQUE_ID]
**Previous Review**: [Link to previous review if updating]
**Scope**: [Brief description of code section]

---

## Executive Summary

| Metric | Score | Status |
|--------|-------|--------|
| Mathematical Accuracy | [0-100%] | [✅/⚠️/🔴] |
| Unit Consistency | [0-100%] | [✅/⚠️/🔴] |
| Physics Validity | [0-100%] | [✅/⚠️/🔴] |
| Code Quality | [0-100%] | [✅/⚠️/🔴] |
| Numerical Stability | [0-100%] | [✅/⚠️/🔴] |
| **OVERALL RATING** | **[0-100%]** | **[⭐⭐⭐⭐⭐]** |

**Key Findings**: [2-3 sentence summary]

---
Pass Review Structure
For each logical code section, use this format:
markdown## PASS [N]: [PASS_NAME] (Lines [START]–[END])

### Code Section Identifier
```cpp
/*====================================================================================================================================================
                                                         [PASS_NAME] PASS
====================================================================================================================================================*/
```

### Function Rating
**Overall Score**: [0-100%]  
**Star Rating**: [⭐⭐⭐⭐⭐] (out of 5)  
**Reviewed By**: [AI Model] on [Date]  
**Status**: [✅ Verified | ⚠️ Concerns | 🔴 Critical Issues]

---

### Code Snippet
```cpp
// Key code excerpt (5-15 lines showing critical logic)
```

---

### Mathematical Analysis

#### Equation [N.1]: [Equation Name]
**Code Implementation**:
```cpp
[Relevant code lines]
```

**Analytical Form**:
```
[Mathematical equation in LaTeX-style notation]
```

**Literature Reference**:
- **Source**: [Author, "Title", Year, Chapter/Page]
- **Equation Number**: [If applicable]
- **Citation**: [Full reference]

**Unit Analysis**:
| Variable | Units | Dimensional Formula |
|----------|-------|---------------------|
| [var1] | [unit] | [M^a L^b T^c] |
| [var2] | [unit] | [M^a L^b T^c] |
| **Result** | [unit] | [M^a L^b T^c] |

**Verification**:
```
[Step-by-step dimensional analysis]
✅ Units consistent: [final units]
```

**Physics Validation**:
- ✅/❌ **Principle**: [Physical law/principle being applied]
- ✅/❌ **Assumptions**: [List key assumptions]
- ✅/❌ **Limitations**: [Known simplifications or edge cases]
- ✅/❌ **Accuracy Range**: [Where this model is valid]

**Numerical Test**:
```
Given: [input values with units]
Expected: [calculated result]
Code Output: [actual result]
Error: [percentage or absolute difference]
Status: [✅ Pass | ❌ Fail]
```

---

### Issues & Concerns

#### 🔴 Critical Issues (P0 - Ship Blocker)
1. **[Issue Title]** (Line [N])
   - **Problem**: [Description]
   - **Impact**: [Severity and consequences]
   - **Root Cause**: [Why this is wrong]
   - **Fix**: 
```cpp
   // Current (incorrect):
   [current code]
   
   // Corrected:
   [proposed fix]
```
   - **Verification**: [How to test the fix]

#### ⚠️ High Priority Issues (P1 - Accuracy)
[Same format as above]

#### 💡 Medium Priority Issues (P2 - Polish)
[Same format as above]

#### 📝 Low Priority Issues (P3 - Nice-to-Have)
[Same format as above]

---

### Magic Numbers Audit
| Line | Value | Current Usage | Recommended Action |
|------|-------|---------------|-------------------|
| [N] | `[value]` | [description] | [Make configurable as...] |

---

### Code Quality Assessment

**Readability**: [Score/10]
- ✅ Well-commented
- ✅ Clear variable names
- ⚠️ [Specific concerns]

**Maintainability**: [Score/10]
- ✅ Modular structure
- ❌ [Specific concerns]

**Performance**: [Score/10]
- ✅ [Strengths]
- ⚠️ [Optimization opportunities]

---

### Comparative Analysis
If this pass has been reviewed before, compare findings:

| Metric | Previous Review | Current Review | Change |
|--------|----------------|----------------|--------|
| Accuracy | [%] | [%] | [↑/↓/→] |
| Issues Found | [N] | [N] | [↑/↓/→] |
| Rating | [⭐] | [⭐] | [↑/↓/→] |

**Changes Since Last Review**: [Summary of code updates]

---

🔬 Mathematical Verification Protocol
Step 1: Equation Identification
For each equation in the code:

Extract the implemented formula
Identify the corresponding analytical form
Locate the source reference

Step 2: Literature Cross-Reference
Required reference categories:

Primary Sources: Peer-reviewed papers, textbooks, standards (SAE, ISO)
Implementation Guides: Technical manuals, API documentation
Validation Data: Experimental results, benchmark comparisons

Reference Format:
markdown**[Reference ID]**: [Author(s)], "[Title]", [Publication], [Year]
- **Type**: [Textbook/Paper/Standard/Manual]
- **Relevance**: [Which equations/principles]
- **Key Contributions**: [What this reference validates]
- **Equation Reference**: [Specific equation numbers if applicable]
- **Accessibility**: [DOI/ISBN/URL]
Step 3: Dimensional Analysis
For EVERY equation:

List all variables with units
Perform dimensional analysis using M-L-T-Θ system
Verify left-hand side = right-hand side dimensionally
Flag any inconsistencies immediately

Step 4: Physical Principle Validation
Ask:

Does this equation follow known physical laws (Newton, thermodynamics, etc.)?
Are sign conventions correct?
Are limits/edge cases handled?
Does behavior match physical intuition?

Step 5: Numerical Verification
Create test cases:
markdown**Test Case [N]**: [Descriptive name]
- **Scenario**: [What we're testing]
- **Inputs**: 
  - `[var1]` = [value] [unit]
  - `[var2]` = [value] [unit]
- **Expected Output**: [calculated value] [unit]
- **Code Output**: [actual value] [unit]
- **Relative Error**: [%]
- **Tolerance**: [acceptable %]
- **Result**: [✅ Pass | ❌ Fail]
- **Notes**: [Any observations]

🤖 AI Reviewer Instructions
When Starting a Review:

Identify yourself:

markdown   Reviewed by: [Model Name] ([Version String])
   Review Date: [ISO 8601 Date]
   Review Session ID: [Unique identifier]

Check for previous reviews:

Read the most recent review in CoreReview/
Note any open issues or pending verifications
Identify code changes since last review (if version control available)


Declare your scope:

markdown   **Review Scope**: [Full audit | Update | Targeted analysis of [specific area]]
   **Lines Reviewed**: [START]–[END]
   **Focus Areas**: [List specific concerns to investigate]
During Review:
For Each Pass/Section:

Read the code block thoroughly
Extract equations - identify every mathematical operation
Verify each equation using the 5-step protocol above
Check units - create a unit analysis table
Find references - search for source material (you may use web_search)
Test numerically - create at least one test case
Rate the section using the scoring rubric below
Document findings in the standard format

Critical Rules:

❌ NEVER skip dimensional analysis
❌ NEVER accept equations without literature references
❌ NEVER assume code is correct because it "looks reasonable"
✅ ALWAYS verify sign conventions
✅ ALWAYS check edge cases (zero, negative, infinity)
✅ ALWAYS note magic numbers

Scoring Rubric:
ScoreCriteria95-100%Perfect implementation, peer-reviewed references, all units verified90-94%Correct physics, minor style issues, solid references85-89%Functionally correct, some simplifications documented, good references80-84%Mostly correct, acceptable simplifications, adequate references70-79%Works but has documented limitations, some references missing60-69%Questionable accuracy, weak references, needs improvement<60%Incorrect physics or math, missing references, critical issues
Star Rating Conversion:

⭐⭐⭐⭐⭐ (95-100%): Publication-quality
⭐⭐⭐⭐☆ (85-94%): Production-ready
⭐⭐⭐☆☆ (70-84%): Acceptable with caveats
⭐⭐☆☆☆ (60-69%): Needs work
⭐☆☆☆☆ (<60%): Requires major revision


📝 Updating an Existing Review
When to Update:

Code has been modified
New references discovered
Previous review had gaps
Different AI model providing second opinion

Update Protocol:

Create a new file: ModelReviews/[YourModel]_[Date].md
Reference the previous review:

markdown   **Previous Review**: [Link to file]
   **Previous Reviewer**: [AI Model]
   **Previous Date**: [Date]
   **Previous Rating**: [Score/Stars]

Note code changes:

markdown   ## Changes Since Last Review
   - [Line N]: [Description of change]
   - [Line M]: [Description of change]
   
   **Impact Assessment**: [How these changes affect the review]

Re-evaluate affected sections:

If code unchanged: May reference previous findings
If code changed: Must perform full re-verification
If new insights: Document even if code unchanged


Update CoreReview:

If your findings supersede previous review, update CoreReview/SolvePowertrain_Review.md
Add changelog entry:



markdown   ### [Date] - [AI Model]
   - **Section**: [Pass Name]
   - **Change**: [What was updated]
   - **Reason**: [Why this update was needed]
   - **Rating Change**: [Old] → [New]

🔍 Code Section Markers
All code reviews should reference these standardized section markers:
cpp/*====================================================================================================================================================
                                                         [SECTION_NAME] PASS
====================================================================================================================================================*/
Standard Section Names:

CONSTANTS PASS - Physical constants and configuration
PERSISTENT STATE PASS - Static/member variables initialization
ENVIRONMENT PASS - Ambient conditions, external inputs
TURBOCHARGER PASS - Forced induction modeling
ENGINE TORQUE PASS - ICE output and losses
INERTIA REFLECTION PASS - Gear ratio transforms
RK4 INTEGRATION PASS - Numerical integration
CLUTCH DYNAMICS PASS - Clutch slip and lockup
TRANSMISSION PASS - Gear selection and ratios
DIFFERENTIAL PASS - Torque distribution
AERODYNAMICS PASS - Drag and downforce
WHEEL DYNAMICS PASS - Tire forces and slip
THERMAL PASS - Temperature modeling
FUEL CONSUMPTION PASS - Energy usage
TELEMETRY PASS - Data output


📚 Required References Library
Maintain a master reference list in CoreReview/References.md:
markdown# SolvePowertrain References Library

## Textbooks
1. **Heywood1988**: Heywood, J.B., "Internal Combustion Engine Fundamentals", McGraw-Hill, 1988
   - Used for: Engine thermodynamics, BSFC, turbocharger modeling
   
2. **Gillespie1992**: Gillespie, T.D., "Fundamentals of Vehicle Dynamics", SAE International, 1992
   - Used for: Inertia reflection, load transfer, tire models

[... continue for all references ...]

## Papers
[List peer-reviewed papers with DOI]

## Standards
[List SAE, ISO standards]

## Online Resources
[List validated technical resources]

✅ Review Completion Checklist
Before submitting a review, verify:

 File header complete with all metadata
 Executive summary table filled
 Each pass has function rating and stars
 All equations have literature references
 All equations have unit analysis tables
 At least one numerical test per critical equation
 All issues categorized by priority (P0-P3)
 Magic numbers documented
 Code quality assessment completed
 Comparison to previous review (if applicable)
 References added to master library
 AI model identifier and date in header
 Review added to ReviewHistory.md


🔄 Review Workflow
mermaidgraph TD
    A[New Code/Update] --> B[AI Reviewer Assigned]
    B --> C[Read Previous Reviews]
    C --> D[Identify Changed Sections]
    D --> E[For Each Pass]
    E --> F[Extract Equations]
    F --> G[Verify Math]
    G --> H[Check Units]
    H --> I[Find References]
    I --> J[Test Numerically]
    J --> K[Rate Section]
    K --> L{More Passes?}
    L -->|Yes| E
    L -->|No| M[Compile Review]
    M --> N[Compare to Previous]
    N --> O[Update CoreReview]
    O --> P[Add to ReviewHistory]
    P --> Q[Submit Review]

🎯 Example: Minimal Valid Review Entry
markdown## PASS 3: ENGINE TORQUE (Lines 2143–2173)

### Function Rating
**Overall Score**: 92%  
**Star Rating**: ⭐⭐⭐⭐⭐  
**Reviewed By**: Claude Sonnet 4.5 on 2025-12-02  
**Status**: ✅ Verified

---

### Code Snippet
```cpp
float MaxTorqueAtRPM = DrivetrainSpecs_PT.Engine.SampleTorque(EngineRPM);
MaxTorqueAtRPM *= TorqueMultiplier;
float ThrottleTorque = MaxTorqueAtRPM * EffectiveThrottle;
```

---

### Mathematical Analysis

#### Equation 3.1: Drive Torque
**Analytical Form**: `τ_drive = τ_max(ω) × k_boost × θ`

**Literature Reference**: Standard approach from Heywood1988, Ch. 9

**Unit Analysis**:
| Variable | Units | Check |
|----------|-------|-------|
| MaxTorqueAtRPM | N·m | ✅ |
| TorqueMultiplier | - | ✅ |
| EffectiveThrottle | - | ✅ |
| **ThrottleTorque** | N·m | ✅ |

**Physics Validation**:
- ✅ Torque scales linearly with throttle (simplified but acceptable)
- ✅ Boost multiplier correctly amplifies output

**Numerical Test**:
```
Given: MaxTorqueAtRPM = 500 N·m, TorqueMultiplier = 1.2, Throttle = 0.8
Expected: 500 × 1.2 × 0.8 = 480 N·m
Status: ✅ Verified
```

---

### Issues
No critical issues found. Throttle as linear multiplier is a simplification but acceptable for real-time simulation.

---

📊 Review History Template
Maintain in CoreReview/ReviewHistory.md:
markdown# SolvePowertrain Review History

## 2025-12-02: Claude Sonnet 4.5
- **Action**: Full re-review of turbocharger pass
- **Key Changes**: Updated compressor model to include efficiency losses
- **Rating Change**: 78% → 95%
- **Files**: `ModelReviews/Claude_Sonnet_4.5_20251202.md`

## 2025-11-30: GPT-4 Turbo
- **Action**: Independent verification of inertia reflection
- **Key Findings**: Confirmed correctness, suggested minor comment improvements
- **Rating**: 97% (agrees with previous review)
- **Files**: `ModelReviews/GPT4_Turbo_20251130.md`

## 2025-11-29: Claude Sonnet 4.5
- **Action**: Initial comprehensive review
- **Scope**: All passes (lines 2045-3435)
- **Overall Rating**: 89%
- **Critical Issues**: 1 (static variables)
- **Files**: `CoreReview/SolvePowertrain_Review.md`

🚀 Quick Start for New AI Reviewer

Read this protocol document completely
Review the latest entry in CoreReview/ReviewHistory.md
Read the current CoreReview/SolvePowertrain_Review.md
Identify your scope (full review vs. targeted analysis)
Create your review file using the template
Follow the pass-by-pass format for each section
Verify all math using the 5-step protocol
Submit your review and update the history


End of Protocol Document
Last Updated: 2025-12-02
Protocol Version: 1.0
Maintained by: Engineering Review Team