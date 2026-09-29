#include "AtlasFontTypes.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

bool FAtlasFontDescriptor::LoadFromJsonFile(const FString& AbsoluteJsonPath)
{
    FString Text;
    if (!FFileHelper::LoadFileToString(Text, *AbsoluteJsonPath))
    {
        UE_LOG(LogTemp, Error, TEXT("AtlasFont: failed to read %s"), *AbsoluteJsonPath);
        return false;
    }
    return LoadFromJsonString(Text);
}

namespace
{
    // Scan a JSON object body starting at brace-open Pos. Returns Pos of the matching '}' + 1, or INDEX_NONE.
    static int32 SkipJsonObject(const FString& S, int32 Pos)
    {
        int32 Depth = 0;
        bool bInStr = false;
        bool bEsc = false;
        for (int32 i = Pos; i < S.Len(); ++i)
        {
            const TCHAR C = S[i];
            if (bEsc) { bEsc = false; continue; }
            if (bInStr)
            {
                if (C == TEXT('\\')) { bEsc = true; }
                else if (C == TEXT('"')) { bInStr = false; }
                continue;
            }
            if (C == TEXT('"')) { bInStr = true; }
            else if (C == TEXT('{') || C == TEXT('[')) { ++Depth; }
            else if (C == TEXT('}') || C == TEXT(']'))
            {
                --Depth;
                if (Depth == 0) { return i + 1; }
            }
        }
        return INDEX_NONE;
    }

    static int32 SkipWhitespace(const FString& S, int32 Pos)
    {
        while (Pos < S.Len() && FChar::IsWhitespace(S[Pos])) { ++Pos; }
        return Pos;
    }
}

bool FAtlasFontDescriptor::LoadFromJsonString(const FString& JsonText)
{
    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("AtlasFont: invalid JSON"));
        return false;
    }

    Root->TryGetStringField(TEXT("fontName"),  FontName);
    Root->TryGetStringField(TEXT("atlasType"), AtlasType);
    Root->TryGetNumberField(TEXT("atlasSize"),  AtlasSize);
    Root->TryGetNumberField(TEXT("pxRange"),    PxRange);
    Root->TryGetNumberField(TEXT("cellPx"),     CellPx);
    Root->TryGetNumberField(TEXT("cellWidth"),  CellWidth);
    Root->TryGetNumberField(TEXT("cellHeight"), CellHeight);
    Root->TryGetNumberField(TEXT("glyphSize"),  GlyphSize);

    Glyphs.Reset();

    // Hand-roll the glyphs map. UE's FJsonObject::Values is TMap<FString, ...> with case-insensitive
    // FString hashing/equality, so 'A' and 'a' collide on insert and one of each pair is lost during
    // FJsonSerializer::Deserialize. We scan the raw JSON ourselves to keep both cases.
    const FString GlyphsTag(TEXT("\"glyphs\""));
    const int32 TagPos = JsonText.Find(GlyphsTag);
    if (TagPos == INDEX_NONE)
    {
        UE_LOG(LogTemp, Error, TEXT("AtlasFont: missing 'glyphs' object"));
        return false;
    }

    int32 P = TagPos + GlyphsTag.Len();
    P = SkipWhitespace(JsonText, P);
    if (P >= JsonText.Len() || JsonText[P] != TEXT(':'))
    {
        UE_LOG(LogTemp, Error, TEXT("AtlasFont: malformed 'glyphs' (no colon)"));
        return false;
    }
    ++P;
    P = SkipWhitespace(JsonText, P);
    if (P >= JsonText.Len() || JsonText[P] != TEXT('{'))
    {
        UE_LOG(LogTemp, Error, TEXT("AtlasFont: 'glyphs' is not an object"));
        return false;
    }
    ++P; // step past the opening '{' of the glyphs object

    // Walk key/value pairs.
    while (true)
    {
        P = SkipWhitespace(JsonText, P);
        if (P >= JsonText.Len()) { break; }
        const TCHAR C = JsonText[P];
        if (C == TEXT('}')) { break; }
        if (C == TEXT(','))   { ++P; continue; }
        if (C != TEXT('"'))
        {
            // Unexpected token; bail to avoid infinite loop.
            UE_LOG(LogTemp, Warning, TEXT("AtlasFont: unexpected char in glyphs map at offset %d"), P);
            break;
        }

        // Parse the key string. Single-character keys for ASCII glyphs; handle escapes for safety.
        ++P;
        FString Key;
        bool bEsc = false;
        while (P < JsonText.Len())
        {
            const TCHAR K = JsonText[P++];
            if (bEsc)
            {
                switch (K)
                {
                    case TEXT('"'):  Key.AppendChar(TEXT('"')); break;
                    case TEXT('\\'): Key.AppendChar(TEXT('\\')); break;
                    case TEXT('/'):  Key.AppendChar(TEXT('/')); break;
                    case TEXT('n'):  Key.AppendChar(TEXT('\n')); break;
                    case TEXT('t'):  Key.AppendChar(TEXT('\t')); break;
                    case TEXT('r'):  Key.AppendChar(TEXT('\r')); break;
                    case TEXT('b'):  Key.AppendChar(TEXT('\b')); break;
                    case TEXT('f'):  Key.AppendChar(TEXT('\f')); break;
                    case TEXT('u'):
                    {
                        if (P + 4 <= JsonText.Len())
                        {
                            const FString Hex = JsonText.Mid(P, 4);
                            const int32 Cp = FCString::Strtoi(*Hex, nullptr, 16);
                            Key.AppendChar(static_cast<TCHAR>(Cp));
                            P += 4;
                        }
                        break;
                    }
                    default: Key.AppendChar(K); break;
                }
                bEsc = false;
                continue;
            }
            if (K == TEXT('\\')) { bEsc = true; continue; }
            if (K == TEXT('"'))  { break; }
            Key.AppendChar(K);
        }

        P = SkipWhitespace(JsonText, P);
        if (P >= JsonText.Len() || JsonText[P] != TEXT(':')) { break; }
        ++P;
        P = SkipWhitespace(JsonText, P);
        if (P >= JsonText.Len() || JsonText[P] != TEXT('{')) { break; }

        const int32 ObjStart = P;
        const int32 ObjEnd = SkipJsonObject(JsonText, P);
        if (ObjEnd == INDEX_NONE) { break; }

        const FString ObjText = JsonText.Mid(ObjStart, ObjEnd - ObjStart);
        TSharedPtr<FJsonObject> G;
        TSharedRef<TJsonReader<>> ObjReader = TJsonReaderFactory<>::Create(ObjText);
        if (FJsonSerializer::Deserialize(ObjReader, G) && G.IsValid() && Key.Len() > 0)
        {
            FAtlasGlyph Out;
            G->TryGetNumberField(TEXT("u0"), Out.U0);
            G->TryGetNumberField(TEXT("v0"), Out.V0);
            G->TryGetNumberField(TEXT("u1"), Out.U1);
            G->TryGetNumberField(TEXT("v1"), Out.V1);
            G->TryGetNumberField(TEXT("contentLeft"),   Out.ContentLeft);
            G->TryGetNumberField(TEXT("contentTop"),    Out.ContentTop);
            G->TryGetNumberField(TEXT("contentWidth"),  Out.ContentWidth);
            G->TryGetNumberField(TEXT("contentHeight"), Out.ContentHeight);
            G->TryGetNumberField(TEXT("advance"),       Out.Advance);

            const TCHAR Ch = Key[0];
            Glyphs.Add(static_cast<int32>(Ch), Out);
        }

        P = ObjEnd;
    }

    UE_LOG(LogTemp, Log, TEXT("AtlasFont: loaded %d glyphs from descriptor"), Glyphs.Num());
    return true;
}
