#pragma once

// NOTE(Felix): Relevant functions
// as_pcf_context asPcfHeader_InitializeFromBuffer(void *FileBuffer);
// asPcfHeader_DrawCharacterToBuffer(as_pcf_context Context, i32 Character,
//                                   void *ToFillTopLeft, u32 BufferRowStrideInBytes,
//                                   void *BufferPerPixel, u32 BufferPerPixelSize, u32 PixelStrideInBytes)


// NOTE(Felix): Info:
// https://fontforge.org/docs/techref/pcf-format.html

// TODO(Felix): 
// - Header guards for re-usability (single compilation unit vs multiple units)
// - Prefix all of this shit with "asPcfHeader"
// - Make this more portable by getting rid of our custom i32 stuff and use int32_t instead
// - Make the user provide endianess conversion functions
// 
// At the moment this depends on
// - our language layer (to be replaced with <stdint.h>)
// - endian.h (make the user supply functions, if not have fallback simple function)
// - memcpy (=> string.h) - maybe just use our own function for that, idk


typedef struct
{
	i32 Type;
	i32 Format;
	i32 SizeInBytes;
	i32 OffsetFromStartOfFile;
} toc_entry;

typedef enum
{
	TOC_ENTRY_TYPE_PCF_PROPERTIES      = (1 << 0),
	TOC_ENTRY_TYPE_PCF_ACCELERATORS    = (1 << 1),
	TOC_ENTRY_TYPE_PCF_METRICS         = (1 << 2),
	TOC_ENTRY_TYPE_PCF_BITMAPS         = (1 << 3),
	TOC_ENTRY_TYPE_PCF_INK_METRICS     = (1 << 4),
	TOC_ENTRY_TYPE_PCF_BDF_ENCODINGS   = (1 << 5),
	TOC_ENTRY_TYPE_PCF_SWIDTHS         = (1 << 6),
	TOC_ENTRY_TYPE_PCF_GLYPH_NAMES     = (1 << 7),
	TOC_ENTRY_TYPE_PCF_BDF_ACCELRATORS = (1 << 8),
} toc_entry_type;

typedef enum
{
	TOC_ENTRY_FORMAT_PCF_DEFAULT_FORMAT     = 0x000,
	TOC_ENTRY_FORMAT_PCF_INKBOUNDS          = 0x200,
	TOC_ENTRY_FORMAT_PCF_ACCEL_W_INKBOUNDS  = 0x100,
	TOC_ENTRY_FORMAT_PCF_COMPRESSED_METRICS = 0x100,
	
	TOC_ENTRY_FORMAT_PCF_GLYPH_PAD_MASK = (3 << 0),
	TOC_ENTRY_FORMAT_PCF_BYTE_MASK      = (1 << 2),
	TOC_ENTRY_FORMAT_PCF_BIT_MASK       = (1 << 3),
	TOC_ENTRY_FORMAT_PCF_SCAN_UNIT_MASK = (3 << 4),
} toc_entry_format;

typedef struct
{
	void      *FileBuffer;
	toc_entry *HeaderTables;
	i32        HeaderTableCount;
	u32        FontWidth;
	u32        FontHeight;
	i32 BitmapFormat;
	i32 *Offsets;
	i32 *BitmapSizes;
	 u8 *BitmapData;
	i16 *GlyphIndeces;
	
	i16 MinCharOrByte2;
	i16 MaxCharOrByte2;
	i16 MinByte1;
	i16 MaxByte1;
	i16 DefaultChar;
	
	b32 BitmapTableIsBigEndian;
	b32 EncodingTableIsBigEndian;
} as_pcf_context;


internal i32
I32ConvertByteOrderToHost(i32 Value, b32 IsBigEndian) 
{
	if (IsBigEndian) 
	{
		return ((i32)be32toh((u32)Value));
	}
	else
	{
		return ((i32)le32toh((u32)Value));
	}
}

internal i16
I16ConvertByteOrderToHost(i16 Value, b32 IsBigEndian) 
{
	if (IsBigEndian) 
	{
		return ((i16)be16toh((u16)Value));
	}
	else
	{
		return ((i16)le16toh((u16)Value));
	}
}

internal toc_entry *
GetTable(as_pcf_context Context, toc_entry_type Type)
{
	toc_entry *ToReturn = 0;
	for (i32 Index = 0; Index < Context.HeaderTableCount; ++Index)
	{
		toc_entry *Entry = Context.HeaderTables+Index;
		if ((u32)Entry->Type & Type)
		{
			ToReturn = Entry;
			break;
		}
	}
	assert(ToReturn);
	return (ToReturn);
}

internal as_pcf_context
asPcfHeader_InitializeFromBuffer(void *FontFileBuffer)
{
	as_pcf_context Result = { 0 };

	// NOTE(Felix): Save Buffer
	Result.FileBuffer = FontFileBuffer;
	
	// NOTE(Felix): Check for magic numbers, indicating that 
	// the given buffer is indeed a pcf buffer/file
	{
		char *MagicHeader = FontFileBuffer;
		assert(MagicHeader[0] == 0x01 &&
		       MagicHeader[1] == 'f'  &&
		       MagicHeader[2] == 'c'  &&
		       MagicHeader[3] == 'p');
	}
	
	// NOTE(Felix): Get Table count
	{
		u8 *StartOfTableCount = (u8 *)FontFileBuffer + 4;
		Result.HeaderTableCount = (i32)le32toh(*(u32 *)StartOfTableCount);
	}
	
	// NOTE(Felix): Get Tables
	{
		u8 *StartOfTables = (u8 *)FontFileBuffer + 8;
		Result.HeaderTables = (toc_entry *)StartOfTables;
	}
	
	// NOTE(Felix): Accelerator Table
	{
		toc_entry *HeaderAcceleratorTableEntry = GetTable(Result, TOC_ENTRY_TYPE_PCF_ACCELERATORS);
		u8 *BeginAcceleratorTable = (u8 *)Result.FileBuffer + I32ConvertByteOrderToHost(HeaderAcceleratorTableEntry->OffsetFromStartOfFile, 0);
		
		i32 AcceleratorTableFormat = I32ConvertByteOrderToHost(*(i32 *)BeginAcceleratorTable, 0);
		b32 IsBigEndian = (AcceleratorTableFormat & TOC_ENTRY_FORMAT_PCF_BYTE_MASK);
		
		// NOTE(Felix): I'm ignoring lots of data here, so lots of magic numbers
		u8  ConstantWidth = *(BeginAcceleratorTable + 7);
		i32 FontAscent  = I32ConvertByteOrderToHost(*(i32 *)(BeginAcceleratorTable + 12), IsBigEndian);
		i32 FontDescent = I32ConvertByteOrderToHost(*(i32 *)(BeginAcceleratorTable + 16), IsBigEndian);
		i16 FontMaxWidth = I16ConvertByteOrderToHost(*(i16 *)(BeginAcceleratorTable + 12 + 4*3 + 6*2 + 4), IsBigEndian);
		
		Result.FontWidth = (u32)FontMaxWidth;
		Result.FontHeight = (u32)(FontAscent + FontDescent);
	}
	
	// NOTE(Felix): Encoding Table
	{
		toc_entry *HeaderEncodingTableEntry = GetTable(Result, TOC_ENTRY_TYPE_PCF_BDF_ENCODINGS);
		u8 *BeginEncodingTable = (u8 *)Result.FileBuffer + I32ConvertByteOrderToHost(HeaderEncodingTableEntry->OffsetFromStartOfFile, 0);
		
		i32 EncodingTableFormat = I32ConvertByteOrderToHost(*(i32 *)BeginEncodingTable, 0);
		Result.EncodingTableIsBigEndian = (EncodingTableFormat & TOC_ENTRY_FORMAT_PCF_BYTE_MASK);
		
		Result.MinCharOrByte2  = I16ConvertByteOrderToHost(*(i16 *)(BeginEncodingTable +  4), Result.EncodingTableIsBigEndian);
		Result.MaxCharOrByte2  = I16ConvertByteOrderToHost(*(i16 *)(BeginEncodingTable +  6), Result.EncodingTableIsBigEndian);
		Result.MinByte1        = I16ConvertByteOrderToHost(*(i16 *)(BeginEncodingTable +  8), Result.EncodingTableIsBigEndian);
		Result.MaxByte1        = I16ConvertByteOrderToHost(*(i16 *)(BeginEncodingTable + 10), Result.EncodingTableIsBigEndian);
		Result.DefaultChar     = I16ConvertByteOrderToHost(*(i16 *)(BeginEncodingTable + 12), Result.EncodingTableIsBigEndian);
		Result.GlyphIndeces =  (i16 *)(BeginEncodingTable + 14);
	}

	// NOTE(Felix): Bitmap table
	{
		toc_entry *HeaderBitmapTableEntry = GetTable(Result, TOC_ENTRY_TYPE_PCF_BITMAPS);
		u8 *BeginBitmapTable = (u8 *)Result.FileBuffer + I32ConvertByteOrderToHost(HeaderBitmapTableEntry->OffsetFromStartOfFile, 0);
		
		Result.BitmapFormat = I32ConvertByteOrderToHost(*(i32 *)BeginBitmapTable, 0);
		Result.BitmapTableIsBigEndian = (Result.BitmapFormat & TOC_ENTRY_FORMAT_PCF_BYTE_MASK);
		
		i32 GlyphCount = I32ConvertByteOrderToHost(*(i32 *)(BeginBitmapTable + 4), Result.BitmapTableIsBigEndian);
		Result.Offsets = (i32 *)(BeginBitmapTable + 4 + 4);
		Result.BitmapSizes = (i32 *)(Result.Offsets + GlyphCount);
		Result.BitmapData = (u8 *)(Result.BitmapSizes+4);
	}


	// NOTE(Felix): Test character print
#if 0
	{
		i16 Index = (i16)be16toh((u16)Result.GlyphIndeces['M']);
		i32 Offset = (i32)be32toh((u32)Result.Offsets[Index]);

		{
			u32 BytesPerRow = 0;
			switch (Result.BitmapFormat & 3)
			{
				case 0: { BytesPerRow = 1; } break;
				case 1: { BytesPerRow = 2; } break;
				case 2: { BytesPerRow = 4; } break;
				default: { assert(0); } break;
			}

			u32 BitsPerRow = FONT_WIDTH;

			u8 *Data = Result.BitmapData + Offset;
			for (u32 Y = 0; Y < FONT_HEIGHT; ++Y)
			{
				u32 BitsPrinted = 0;
				for (u32 XByte = 0; XByte < BytesPerRow; ++XByte)
				{
					u8 Byte = Data[Y*BytesPerRow + XByte];
					for (u32 Bit = 0; Bit < 8 && BitsPrinted < BitsPerRow; ++Bit)
					{
						printf("%s", (Byte & (1 << (7-Bit))) ? "#" : ".");
						++BitsPrinted;
					}
				}
				printf("\n");
			}
		}
	}
#endif

	return (Result);
}

internal void
asPcfHeader_DrawCharacterToBuffer(as_pcf_context Context, i32 Character,
								  void *ToFillTopLeft, u32 BufferRowStrideInBytes,
								  void *BufferPerPixel, u32 BufferPerPixelSize, u32 PixelStrideInBytes)
{
	i32 GlyphIndeces = (Context.MaxCharOrByte2 - Context.MinCharOrByte2 + 1) * (Context.MaxByte1 - Context.MinByte1 + 1);
	if (Character < 0 || Character >= GlyphIndeces)
	{
		Character = (i32)Context.DefaultChar;
	}

	i16 GlyphIndex = I16ConvertByteOrderToHost(Context.GlyphIndeces[Character], Context.EncodingTableIsBigEndian);
	if ((u16)GlyphIndex == 0xFFFF)
	{
		Character = (i32)Context.DefaultChar;
		GlyphIndex = I16ConvertByteOrderToHost(Context.GlyphIndeces[Character], Context.EncodingTableIsBigEndian);
	}

	i32 Offset = I32ConvertByteOrderToHost(Context.Offsets[GlyphIndex], Context.BitmapTableIsBigEndian);
	u32 BytesPerRow = 0;
	switch (Context.BitmapFormat & 3)
	{
		case 0: { BytesPerRow = 1; } break;
		case 1: { BytesPerRow = 2; } break;
		case 2: { BytesPerRow = 4; } break;
		default: { assert(0); } break;
	}
	
	// TODO(Felix): Bit Order
	u32 BitsPerRow = Context.FontWidth;
	u8 *Data = Context.BitmapData + Offset;
	for (u32 Y = 0; Y < Context.FontHeight; ++Y)
	{
		u32 BitsPrinted = 0;
		for (u32 XByte = 0; XByte < BytesPerRow; ++XByte)
		{
			u8 Byte = Data[Y*BytesPerRow + XByte];
			for (u32 Bit = 0; Bit < 8 && BitsPrinted < BitsPerRow; ++Bit)
			{
				if (Byte & (1 << (7-Bit)))
				{
					memcpy((u8 *)ToFillTopLeft + Y*BufferRowStrideInBytes + BitsPrinted*PixelStrideInBytes,
						   BufferPerPixel, BufferPerPixelSize);
				}
				++BitsPrinted;
			}
		}
	}
}
