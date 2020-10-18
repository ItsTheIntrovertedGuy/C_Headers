#pragma once

#define global_variable static
#define local_persist static
#define internal static

#include <stdint.h>
typedef   int8_t  i8;
typedef  int16_t i16;
typedef  int32_t i32;
typedef  int64_t i64;
typedef  uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef      i32 b32;
typedef    float f32;
typedef   double f64;

// TODO(Felix): Proper debug macro
#define Assert(expression) if (!(expression)) { *(i64 *)0 = 0; }

#define KIBIBYTES(n) ((n)*(u64)1024)
#define MEBIBYTES(n) ((u64)1024*KIBIBYTES(n))
#define GIBIBYTES(n) ((u64)1024*MEBIBYTES(n))
#define TEBIBYTES(n) ((u64)1024*GIBIBYTES(n))

#define ARRAYCOUNT(Array) (sizeof(Array) / sizeof((Array)[0]))

#define MIN(a,b) ((a) < (b) ? (a) :  (b))
#define MAX(a,b) ((a) > (b) ? (a) :  (b))
#define ABS(a)   ((a) >  0  ? (a) : -(a))
#define SIGN(a)  ((a) >  0  ?  1  :  ((a) < 0 ? -1 : 0))
#define CLAMP(a, min, max) MIN((max), MAX((min), (a)))

typedef struct
{
	u8 *Memory;
	u64 BlockSize;
	u64 BytesAllocated;
} memory_block;

#ifdef __linux__
#include <sys/mman.h>
#endif

internal memory_block
CreateLinearAllocator(u64 MemoryBlockSize)
{
	memory_block Result = { 0 };
	Result.BlockSize = MemoryBlockSize;
#ifdef __linux__
	Result.Memory = mmap(0, Result.BlockSize, PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);;
#endif
	return (Result);
}

internal void *
AllocateMemory(memory_block *Memory, u64 BytesToAllocate)
{
	Assert(Memory->BlockSize > Memory->BytesAllocated + BytesToAllocate);
	void *MemoryResult = Memory->Memory + Memory->BytesAllocated;
	Memory->BytesAllocated += BytesToAllocate;
	return (MemoryResult);
}

internal void
FreeMemoryBlock(memory_block *Memory)
{
#ifdef __linux__
	munmap(Memory->Memory, Memory->BlockSize);
#endif
	*Memory = (memory_block) { 0 };
}

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

internal b32
CharIsLetter(char Char)
{
	return ((Char >= 'A' && Char <= 'Z') ||
			(Char >= 'a' && Char <= 'z'));
}

internal b32
CharIsNumber(char Char)
{
	return (Char >= '0' && Char <= '9');
}

internal char
CharToUpperIfIsLetter(char Character)
{
	char Result = Character;
	if (Character >= 'a' && Character <= 'z')
	{
		Result = Character - 32;
	}
	return (Result);
}

internal char
CharToLowerIfIsLetter(char Character)
{
	char Result = Character;
	if (Character >= 'A' && Character <= 'Z')
	{
		Result = Character + 32;
	}
	return (Result);
}

internal b32
CharIsAsciiControlCharacter(char Character)
{
	u8 CharInDecimal = (u8)Character;
	b32 IsControlCharacter = (CharInDecimal <= 31) || (CharInDecimal == 127);
	return (IsControlCharacter);
}

internal u32
StringLength(char *String)
{
	u32 Length = 0;
	for (; String[Length] != 0; ++Length) { }
	return (Length);
}

internal b32
StringEqual(char *A, char *B)
{
	i32 Index = 0;
	for (; 
		 A[Index] == B[Index] && A[Index] != 0 && B[Index] != 0;
		++Index)
	{
		// noop
	}
	
	return (A[Index] == B[Index]);
}

internal void
StringAppend(char *Destination, char *ToAppend)
{
	while (*Destination != 0) { Destination++; }
	while (*ToAppend != 0) { *(Destination++) = *(ToAppend++); }
	*Destination = 0;
}

internal b32
StringStartsWith(char *String, char *StartsWith)
{
	i32 Index = 0;
	for (;
		 String[Index] == StartsWith[Index] && StartsWith[Index] != 0;
		 ++Index)
	{
		// noop
	}
	
	return (StartsWith[Index] == 0);
}

internal char *
StringContains(char *StringToSearchWithin, char *SearchTerm, b32 IsCaseSensitive)
{
	Assert(StringToSearchWithin);
	Assert(SearchTerm);

	u32 StringToSearchWithinLength = StringLength(StringToSearchWithin);
	u32 SearchTermLength = StringLength(SearchTerm);
	char *SearchTermWithinStringToSearch = 0;

	for (u32 CharIndex = 0; 
		 CharIndex+SearchTermLength < StringToSearchWithinLength+1; 
		 ++CharIndex)
	{
		u32 MatchingCharsCount = 0;
		for (; MatchingCharsCount < SearchTermLength; ++MatchingCharsCount)
		{
			char StringToSearchCharToCompare = StringToSearchWithin[CharIndex + MatchingCharsCount];
			char SearchTermCharToCompare = SearchTerm[MatchingCharsCount];
			if (0 == IsCaseSensitive)
			{
				StringToSearchCharToCompare = CharToLowerIfIsLetter(StringToSearchCharToCompare);
				SearchTermCharToCompare = CharToLowerIfIsLetter(SearchTermCharToCompare);
			}

			if (StringToSearchCharToCompare != SearchTermCharToCompare)
			{
				break;
			}
		}

		if (MatchingCharsCount == SearchTermLength)
		{
			SearchTermWithinStringToSearch = &StringToSearchWithin[CharIndex];
			break;
		}
	}

	return (SearchTermWithinStringToSearch);
}

internal char *
StringContainsCaseSensitive(char *String, char *SearchTerm)
{
	return (StringContains(String, SearchTerm, 1));
}

internal char *
StringContainsCaseInsensitive(char *String, char *SearchTerm)
{
	return (StringContains(String, SearchTerm, 0));
}

internal char *
StringContainsButReturnAfterFind(char *Buffer, u32 BufferLength, char *CompareString)
{
#if 0
	// TODO(Felix): This implementation is straight up crashing the program
	u32 CompareStringLength = StringLength(CompareString);

	for (u32 BufferIndex = 0; BufferIndex+CompareStringLength < BufferLength; ++BufferIndex)
	{
		for (u32 CompareStringIndex = 0; CompareStringIndex < CompareStringLength; ++CompareStringIndex)
		{
			if (Buffer[BufferIndex+CompareStringIndex] == CompareString[CompareStringIndex])
			{
				if (CompareStringIndex+1 == CompareStringLength)
				{
					return (&Buffer[BufferIndex+CompareStringIndex+1]);
				}
			}
			else
			{
				break;
			}
		}
	}
	
	return (0);
#else
	// TODO(Felix): THIS IMPLEMENTATION IS BUGGY!
	// But so _seems_ to be the above (for some reason)
	// e.g.
	// "rre" with search string "re"
	// will "swallow the first string"
	u32 CompareStringLength = StringLength(CompareString);
	u32 CompareStringIndex = 0;

	for (u32 BufferIndex = 0; BufferIndex < BufferLength; ++BufferIndex)
	{
		if (Buffer[BufferIndex] == CompareString[CompareStringIndex])
		{
			++CompareStringIndex;
			if (CompareStringIndex == CompareStringLength)
			{
				return (&Buffer[BufferIndex+1]);
			}
		}
		else
		{
			CompareStringIndex = 0;
		}
	}

	return (0);
#endif
}

internal i32
StringParseToNumber(char *Buffer)
{
	b32 InvertResult = 0;
	if (Buffer[0] == '-')
	{   
		InvertResult = 1;
		++Buffer;
	}
	else if (Buffer[0] == '+')
	{
		++Buffer;
	}

	i32 Result = 0;
	i32 Digit = 0;
	while ((Digit = (i32)*Buffer - (i32)'0') >= 0 && Digit <= 9)
	{
		Result *= 10;
		Result += Digit;
		++Buffer;
	}
	
	if (InvertResult)
	{
		Result *= -1;
	}
	
	return (Result);
}

internal u64
StringParseUnsignedHexadecimal(char *Buffer)
{
	Assert(Buffer);
	u64 Result = 0;
	for (;;)
	{
		char Character = CharToLowerIfIsLetter(*(Buffer++));
		if (CharIsNumber(Character))
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


internal void
MemoryCopy(void *Destination, void *Source, u64 BytesToCopy)
{
	Assert(Destination);
	Assert(Source);
	Assert(BytesToCopy != (u64)-1);

	u8 *OutputBuffer = Destination;
	u8 *InputBuffer = Source;
	for (u64 ByteIndex = 0; ByteIndex < BytesToCopy; ++ByteIndex)
	{
		OutputBuffer[ByteIndex] = InputBuffer[ByteIndex];
	}
}

internal void
MemoryClear(void *Destination, u64 BytesToClear)
{
	Assert(Destination);
	Assert(BytesToClear != (u64)-1);

	u8 *Buffer = Destination;
	for (u64 ByteIndex = 0; ByteIndex < BytesToClear; ++ByteIndex)
	{
		Buffer[ByteIndex] = 0;
	}
}


