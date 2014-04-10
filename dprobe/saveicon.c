#define WIN32_LEAN_AND_MEAN
#define STRICT

#include <windows.h>
#include "sdk.h"

// turn off boring warnings
#pragma warning(disable: 4244)	// conversion (long->byte, loss of data)
#pragma warning(disable: 4013)	// undefined function, assuming...

#pragma pack(push, 1)

//
//	ICONS (.ICO type 1) are structured like this:
//
//	ICONHEADER											 (just 1)
//	ICONDIR										[1...n]  (an array, 1 for each image)
//	[BITMAPINFOHEADER+COLOR_BITS+MASK_BITS]		[1...n]	 (1 after the other, for each image)
//
//	CURSORS (.ICO type 2) are identical in structure, but use
//	two monochrome bitmaps (real XOR and AND masks, this time).
//

typedef struct
{
	WORD	idReserved;		// must be 0
	WORD	idType;			// 1 = ICON, 2 = CURSOR
	WORD	idCount;		// number of images (and ICONDIRs)

	// ICONDIR   [1...n] 
	// ICONIMAGE [1...n]

} ICONHEADER;

//
//	An array of ICONDIRs immediately follow the ICONHEADER
//
typedef struct
{
	BYTE	bWidth;
	BYTE	bHeight;
	BYTE	bColorCount;
	BYTE	bReserved;
	WORD	wPlanes;		// for cursors, this field = wXHotSpot
	WORD	wBitCount;		// for cursors, this field = wYHotSpot
	DWORD	dwBytesInRes;
	DWORD	dwImageOffset;	// file-offset to the start of ICONIMAGE
	
} ICONDIR;

//
//	After the ICONDIRs follow the ICONIMAGE structures - 
//	consisting of a BITMAPINFOHEADER, (optional) RGBQUAD array, then
//	the color and mask bitmap bits (all packed together
//
typedef struct
{
	BITMAPINFOHEADER	biHeader;      // header for color bitmap (no mask header)
	//RGBQUAD			rgbColors[1...n];
	//BYTE				bXOR[1];      // DIB bits for color bitmap
	//BYTE				bAND[1];      // DIB bits for mask bitmap
	
} ICONIMAGE;

#pragma pack(pop)

//
//	Write the ICO header to disk
//
static UINT WriteIconHeader(HANDLE hFile, int nImages)
{
	ICONHEADER	iconheader;
	UINT		nWritten;	

	// Setup the icon header
	iconheader.idReserved		= 0;		// Must be 0
	iconheader.idType			= 1;		// Type 1 = ICON  (type 2 = CURSOR)
	iconheader.idCount			= nImages;	// number of ICONDIRs 
	
	// Write the header to disk
	WriteFile(hFile, &iconheader, sizeof(iconheader), &nWritten, 0);

	// following ICONHEADER is a series of ICONDIR structures (idCount of them, in fact)
	return nWritten;
}

//
//	Return the number of BYTES the bitmap will take ON DISK
//
static UINT NumBitmapBytes(BITMAP *pBitmap)
{
	int nWidthBytes = pBitmap->bmWidthBytes;

	// bitmap scanlines MUST be a multiple of 4 bytes when stored
	// inside a bitmap resource, so round up if necessary
	if(nWidthBytes & 3)
		nWidthBytes = (nWidthBytes + 4) & ~3;

	return nWidthBytes * pBitmap->bmHeight;
}

//
//	Return number of bytes written
//
static UINT WriteIconImageHeader(HANDLE hFile, BITMAP *pbmpColor, BITMAP *pbmpMask)
{
	BITMAPINFOHEADER biHeader;
	UINT	nWritten;
	UINT    nImageBytes;

	// calculate how much space the COLOR and MASK bitmaps take
	nImageBytes = NumBitmapBytes(pbmpColor) + NumBitmapBytes(pbmpMask);

	// write the ICONIMAGE to disk (first the BITMAPINFOHEADER)
	ZeroMemory(&biHeader, sizeof(biHeader));

	// Fill in only those fields that are necessary
	biHeader.biSize				= sizeof(biHeader);
	biHeader.biWidth			= pbmpColor->bmWidth;
	biHeader.biHeight			= pbmpColor->bmHeight * 2;		// height of color+mono
	biHeader.biPlanes			= pbmpColor->bmPlanes;
	biHeader.biBitCount			= pbmpColor->bmBitsPixel;
	biHeader.biSizeImage		= nImageBytes;

	// write the BITMAPINFOHEADER
	WriteFile(hFile, &biHeader, sizeof(biHeader), &nWritten, 0);

	// write the RGBQUAD color table (for 16 and 256 colour icons)
	if(pbmpColor->bmBitsPixel == 2 || pbmpColor->bmBitsPixel == 8)
	{
		
	}

	return nWritten;
}

//
//	Wrapper around GetIconInfo and GetObject(BITMAP)
//
static BOOL GetIconBitmapInfo(HICON hIcon, ICONINFO *pIconInfo, BITMAP *pbmpColor, BITMAP *pbmpMask)
{
	if(!GetIconInfo(hIcon, pIconInfo))
		return FALSE;

	if(!GetObject(pIconInfo->hbmColor, sizeof(BITMAP), pbmpColor))
		return FALSE;

	if(!GetObject(pIconInfo->hbmMask,  sizeof(BITMAP), pbmpMask))
		return FALSE;

	return TRUE;
}

//
//	Write one icon directory entry - specify the index of the image
//
static UINT WriteIconDirectoryEntry(HANDLE hFile, int nIdx, HICON hIcon, UINT nImageOffset)
{
	ICONINFO	iconInfo;
	ICONDIR		iconDir;

	BITMAP		bmpColor;
	BITMAP		bmpMask;

	UINT		nWritten;
	UINT		nColorCount;
	UINT		nImageBytes;

	GetIconBitmapInfo(hIcon, &iconInfo, &bmpColor, &bmpMask);
		
	nImageBytes = NumBitmapBytes(&bmpColor) + NumBitmapBytes(&bmpMask);

	if(bmpColor.bmBitsPixel >= 8)
		nColorCount = 0;
	else
		nColorCount = 1 << (bmpColor.bmBitsPixel * bmpColor.bmPlanes);

	// Create the ICONDIR structure
	iconDir.bWidth				= bmpColor.bmWidth;
	iconDir.bHeight				= bmpColor.bmHeight;
	iconDir.bColorCount			= nColorCount;
	iconDir.bReserved			= 0;
	iconDir.wPlanes				= bmpColor.bmPlanes;
	iconDir.wBitCount			= bmpColor.bmBitsPixel;
	iconDir.dwBytesInRes		= sizeof(BITMAPINFOHEADER) + nImageBytes;
	iconDir.dwImageOffset		= nImageOffset;

	// Write to disk
	WriteFile(hFile, &iconDir, sizeof(iconDir), &nWritten, 0);

	// Free resources
	DeleteObject(iconInfo.hbmColor);
	DeleteObject(iconInfo.hbmMask);

	return nWritten;
}

static UINT WriteIconData(HANDLE hFile, HBITMAP hBitmap)
{
	BITMAP	bmp;
	int		i;
	BYTE *  pIconData;
	
	UINT	nBitmapBytes;
	UINT	nWritten;
	
	GetObject(hBitmap, sizeof(BITMAP), &bmp);

	nBitmapBytes = NumBitmapBytes(&bmp);

	pIconData = (BYTE *)SdkMalloc(nBitmapBytes);

	GetBitmapBits(hBitmap, nBitmapBytes, pIconData);

	// bitmaps are stored inverted (vertically) when on disk..
	// so write out each line in turn, starting at the bottom + working
	// towards the top of the bitmap. Also, the bitmaps are stored in packed
	// in memory - scanlines are NOT 32bit aligned, just 1-after-the-other
	for(i = bmp.bmHeight - 1; i >= 0; i--)
	{
		// Write the bitmap scanline
		WriteFile(
			hFile, 
			pIconData + (i * bmp.bmWidthBytes),		// calculate offset to the line 
			bmp.bmWidthBytes,						// 1 line of BYTES
			&nWritten,
			0);
		
		// extend to a 32bit boundary (in the file) if necessary
		if(bmp.bmWidthBytes & 3)
		{
			DWORD padding = 0;
			WriteFile(hFile, &padding, 4 - bmp.bmWidthBytes, &nWritten, 0);
		}
	}

	SdkFree(pIconData);

	return nBitmapBytes;
}

//
//	Create a .ICO file, using the specified array of HICON images
//
BOOL SaveIcon(TCHAR *szIconFile, HICON hIcon[], int nNumIcons)
{
	HANDLE	hFile;
	int		i;
	int	*	pImageOffset;
	
	if(hIcon == 0 || nNumIcons < 1)
		return FALSE;

	// Save icon to disk:
	hFile = CreateFile(szIconFile, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

	if(hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	//
	//	Write the iconheader first of all
	//
	WriteIconHeader(hFile, nNumIcons);

	//
	//	Leave space for the IconDir entries
	//
	SetFilePointer(hFile, sizeof(ICONDIR) * nNumIcons, 0, FILE_CURRENT);

	pImageOffset = (int *)SdkMalloc(nNumIcons * sizeof(int));

	//
	//	Now write the actual icon images!
	//
	for(i = 0; i < nNumIcons; i++)
	{
		ICONINFO	iconInfo;
		BITMAP		bmpColor,  bmpMask;
		
		GetIconBitmapInfo(hIcon[i], &iconInfo, &bmpColor, &bmpMask);

		// record the file-offset of the icon image for when we write the icon directories
		pImageOffset[i] = SetFilePointer(hFile, 0, 0, FILE_CURRENT);
		
		// bitmapinfoheader + colortable
		WriteIconImageHeader(hFile, &bmpColor, &bmpMask);
		
		// color and mask bitmaps
		WriteIconData(hFile, iconInfo.hbmColor);
		WriteIconData(hFile, iconInfo.hbmMask);

		DeleteObject(iconInfo.hbmColor);
		DeleteObject(iconInfo.hbmMask);
	}

	// 
	//	Lastly, skip back and write the icon directories.
	//
	SetFilePointer(hFile, sizeof(ICONHEADER), 0, FILE_BEGIN);

	for(i = 0; i < nNumIcons; i++)
	{
		WriteIconDirectoryEntry(hFile, i, hIcon[i], pImageOffset[i]);
	}

	SdkFree(pImageOffset);

	// finished!
	CloseHandle(hFile);

	return TRUE;
}

