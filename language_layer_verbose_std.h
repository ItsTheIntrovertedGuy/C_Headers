#pragma once

#include <stdio.h>

#define global_variable static
#define local_persist static
#define internal static

#include <inttypes.h>
typedef   int8_t    i8;
typedef  int16_t   i16;
typedef  int32_t   i32;
typedef  int64_t   i64;
typedef  uint8_t    u8;
typedef uint16_t   u16;
typedef uint32_t   u32;
typedef uint64_t   u64;
typedef      i32   b32;
typedef    float   f32;
typedef   double   f64;
typedef size_t     memory_index;
typedef uintptr_t  umm;
typedef intptr_t   imm;

#define PFi8  PRIi8
#define PFi16 PRIi16
#define PFi32 PRIi32
#define PFi64 PRIi64
#define PFimm PRIiPTR

#define PFu8  PRIu8
#define PFu16 PRIu16
#define PFu32 PRIu32
#define PFu64 PRIu64
#define PFumm PRIuPTR

#define SFi8  SCNi8
#define SFi16 SCNi16
#define SFi32 SCNi32
#define SFi64 SCNi64
#define SFimm SCNiPTR

#define SFu8  SCNu8
#define SFu16 SCNu16
#define SFu32 SCNu32
#define SFu64 SCNu64
#define SFumm SCNuPTR


#define KIBIBYTES(n) ((n)*(u64)1024)
#define MEBIBYTES(n) ((u64)1024*KIBIBYTES(n))
#define GIBIBYTES(n) ((u64)1024*MEBIBYTES(n))
#define TEBIBYTES(n) ((u64)1024*GIBIBYTES(n))

#define PI (3.14159265358979323846)
#define ARRAYCOUNT(Array) (sizeof(Array) / sizeof((Array)[0]))

#define MIN(a,b) ((a) < (b) ? (a) :  (b))
#define MAX(a,b) ((a) > (b) ? (a) :  (b))
#define ABS(a)   ((a) >  0  ? (a) : -(a))
#define SIGN(a)  ((a) >  0  ? (1) :  ((a) < 0 ? (-1) : (0)))
#define CLAMP(min,a,max) MIN((max), MAX((min), (a)))
#define CLAMP_TOP(a,max) MIN((a),(max))
#define CLAMP_BOTTOM(min,a) MAX((a),(min))


#include <assert.h>
#define Assert assert

#include <ctype.h>
#define CharIsDigitOrLetter          (b32)isalnum
#define CharIsLetter                 (b32)isalpha
#define CharIsAscii                  (b32)isascii
#define CharIsAsciiControlCharacter  (b32)iscntrl
#define CharIsDigit                  (b32)isdigit
#define CharIsVisible                (b32)isgraph
#define CharIsLowerCase              (b32)islower
#define CharIsUpperCase              (b32)Isupper
#define CharIsSpacing                (b32)isspace
#define CharToLowerCase              (char)tolower
#define CharToUpperCase              (char)toupper

#include <math.h>
#define F32ExpBaseE  expf
#define F32ExpBase2  exp2f
#define F32LogBaseE  logf
#define F32LogBase2  log2f
#define F32LogBase10 log10f
#define F32LogBaseE  logf
#define F32Sqrt      sqrtf
#define F32Power     powf
#define F32Sin       sinf
#define F32Cos       cosf
#define F32Tan       tanf
#define F32ArcSin    asinf
#define F32ArcCos    acosf
#define F32ArcTan    atanf
#define F32ArcTan2   atan2f
#define F32Ceil      ceilf
#define F32Floor     floorf
#define F32Round     roundf
#define F64ExpBaseE  exp
#define F64ExpBase2  exp2
#define F64LogBaseE  log
#define F64LogBase2  log2
#define F64LogBase10 log10
#define F64LogBaseE  log
#define F64Sqrt      sqrt
#define F64Power     pow
#define F64Sin       sin
#define F64Cos       cos
#define F64Tan       tan
#define F64ArcSin    asin
#define F64ArcCos    acos
#define F64ArcTan    atan
#define F64ArcTan2   atan2
#define F64Ceil      ceil
#define F64Floor     floor
#define F64Round     round

#include <stdlib.h>
#define StringParseF64 (f64)atof
#define StringParseI64 (i64)atoll
#define BinarySearch   bsearch
#define Sort           qsort
#define Die            abort
// TODO(Felix): Create helper functions for bsearch and qsort

#include <string.h>
#define MemoryCopy                              memcpy
#define MemoryMove                              memmove
#define MemoryCompare                           memcmp
#define MemoryClear(Memory,Size)                memset(memory, 0, Size)
#define StringAppend                            strcat
#define StringCompare                           strcmp
#define StringCopyAll                           strcpy
#define StringCopyLength                        strncpy
#define StringLengthWithout0                    strlen
#define StringFindAnyForList                    strpbrk
#define StringFindFirstSubstringCaseSensitive   strstr
#define StringFindFirstSubstringCaseInSensitive strcasestr

#include <time.h>


internal u64
U64ChangeEndianess(u64 Value)
{
	u64 Result = 0;
	Result = 
		((Value & 0xFF00000000000000) >> 56) |
		((Value & 0x00FF000000000000) >> 40) |
		((Value & 0x0000FF0000000000) >> 24) |
		((Value & 0x000000FF00000000) >>  8) |
		((Value & 0x00000000FF000000) <<  8) |
		((Value & 0x0000000000FF0000) << 24) |
		((Value & 0x000000000000FF00) << 40) |
		((Value & 0x00000000000000FF) << 56);
	return (Result);
}

internal u32
U32ChangeEndianess(u32 Value)
{
	u32 Result = 0;
	Result = 
		((Value & 0xFF000000) >> 24) |
		((Value & 0x00FF0000) >>  8) |
		((Value & 0x0000FF00) <<  8) |
		((Value & 0x000000FF) << 24);
	return (Result);
}

internal u16
U16ChangeEndianess(u16 Value)
{
	u16 Result = 0;
	Result = 
		(u16)((Value & 0xFF00) >> 8) |
		(u16)((Value & 0x00FF) << 8);
	return (Result);
}

internal i64
I64ChangeEndianess(i64 Value)
{
	u64 ChangedEndianess = U64ChangeEndianess(*(u64 *)&Value);
	return (*(i64 *)&ChangedEndianess);
}

internal i32
I32ChangeEndianess(i32 Value)
{
	u32 ChangedEndianess = U32ChangeEndianess(*(u32 *)&Value);
	return (*(i32 *)&ChangedEndianess);
}

internal i16
I16ChangeEndianess(i16 Value)
{
	u16 ChangedEndianess = U16ChangeEndianess(*(u16 *)&Value);
	return (*(i16 *)&ChangedEndianess);
}



internal b32
StringEqual(char *A, char *B)
{
	return (0 == StringCompare(A, B));
}

internal b32
StringStartsWith(char *String, char *StartsWith)
{
	return (0 == MemoryCompare(String, StartsWith, StringLengthWithout0(StartsWith)));
}

internal u64
StringParseUnsignedHexadecimal(char *Buffer)
{
	Assert(Buffer);
	u64 Result = 0;
	for (;;)
	{
		char Character = CharToLowerCase(*(Buffer++));
		if (CharIsDigit(Character))
		{
			Result <<= 4;
			Result += (u64)(Character - '0');
		}
		else if (Character >= 'a' && Character <= 'f')
		{
			Result <<= 4;
			Result += 10 + (u64)(Character - 'a');
		}
		else
		{
			break;
		}
	}
	return (Result);
}


internal u32
ByteCountToBase64CharCount(u32 ByteCount)
{
	// NOTE(Felix): For every pack of three bytes (even if only one byte is used)
	// we get 4 base64 Chars
	//           v-----------------v: Equivalent to ceil(ByteCount/3)
	u32 Result = ((ByteCount+2) / 3) * 4;
	return (Result);
}

internal void
StringConvertToBase64(char *ResultBuffer, u8 *Bytes, u32 ByteCount)
{
	u32 CurrentByte = 0;
	u32 ResultBufferLength = ByteCountToBase64CharCount(ByteCount);
	u32 PaddingCharCount = 0;

	for (u32 ResultBufferIndex = 0; 
		 ResultBufferIndex < ResultBufferLength; 
		 ResultBufferIndex += 4)
	{
		// NOTE(Felix): Read three bytes into a small buffer
		u32 BytesToConvert = 0;
		for (u32 ByteIndex = 0; ByteIndex < 3; ++ByteIndex)
		{
			BytesToConvert <<= 8;

			u8 Byte = 0;
			if (CurrentByte < ByteCount)
			{
				Byte = Bytes[CurrentByte];
			}
			else
			{
				PaddingCharCount++;
			}
			BytesToConvert |= Byte;
			CurrentByte++;
		}

		// NOTE(Felix): Retreive 6 bits at a time from that buffer and translate that value
		for (u32 CharIndex = 0; 
			 CharIndex < (4 - PaddingCharCount); 
			 ++CharIndex)
		{
			// NOTE(Felix): The first output char is stored at the msb end of the u32
			u8 ValueToTranslate = (BytesToConvert >> (18 - 6*CharIndex)) & 0x3F;

			char ResultChar = 0;
			Assert(ValueToTranslate < 64);
			if (ValueToTranslate <= 25)
			{
				// NOTE(Felix): Uppercase alphabet
				ResultChar = (char)('A' + ValueToTranslate);
			}
			else if (ValueToTranslate >= 26 && ValueToTranslate <= 51)
			{
				// NOTE(Felix): Lowercase alphabet
				ResultChar = (char)('a' + (ValueToTranslate - 26));
			}
			else if (ValueToTranslate >= 52 && ValueToTranslate <= 61)
			{
				// NOTE(Felix): Digit 0-9
				ResultChar = (char)('0' + (ValueToTranslate - 52));
			}
			else if (ValueToTranslate == 62)
			{
				ResultChar = '+';
			}
			else if (ValueToTranslate == 63)
			{
				ResultChar = '/';
			}

			ResultBuffer[ResultBufferIndex+CharIndex] = ResultChar;
		}
	}
	
	// NOTE(Felix): Fill padding
	for (u32 PaddingIndex = 0; PaddingIndex < PaddingCharCount; ++PaddingIndex)
	{
		ResultBuffer[ResultBufferLength-1-PaddingIndex] = '=';
	}
}

