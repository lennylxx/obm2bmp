/*
  obm2bmp.cpp 2012/07/15
  by 7k
  email: blxode [at] gmail.com
	convert *.obm files to *.bmp bitmap files
	this program doesn't support this image type: "0x1009", aka imagetype=0x09 bitcount=0x10
	
  Supporting Games (iOS platform):
		The King of Fighters i 2012
		Street Fighter IV Volt
		Double Dragon
*/

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <sys/stat.h>
using namespace std;

typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned int   DWORD;
typedef long           LONG;

#pragma pack(1)
typedef struct tagBITMAPFILEHEADER 
{ 
	WORD bfType;    
	DWORD bfSize; 
	WORD bfReserved1; 
	WORD bfReserved2; 
	DWORD bfOffBits;
} BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER
{
	DWORD biSize;
	LONG biWidth;
	LONG biHeight;
	WORD biPlanes;
	WORD biBitCount;
	DWORD biCompression;
	DWORD biSizeImage;
	LONG biXPelsPerMeter;
	LONG biYPelsPerMeter;
	DWORD biClrUsed;
	DWORD biClrImportant;
} BITMAPINFOHEADER;

typedef struct tagRGBQUAD {
	BYTE blue; 
	BYTE green; 
	BYTE red; 
	BYTE reserved;
} RGBQUAD;

typedef struct tagRGB {
	BYTE blue; 
	BYTE green; 
	BYTE red; 
} RGB;

typedef struct tagOBMINFOHEADER
{
	BYTE magic[2];//"OI"
	BYTE imagetype;
	BYTE bitcount;
	WORD width;
	WORD height;
} OBMINFOHEADER;

string ChangeExt(string filename, string newExt){
	int pt = filename.rfind('.');
	string changed = filename.substr(0, pt) + newExt;
	return changed;
}

int main(int argc, char *argv[])
{
	if(argc<2) return 1;
	string ofname = ChangeExt(argv[1], ".bmp");
	//open the file
	ifstream infile(argv[1],ios::binary);
	ofstream outfile(ofname.c_str(),ios::binary);
	//read .obm file
	struct _stat buf;
	_stat(argv[1], &buf);
	BYTE *in_buffer = new BYTE[buf.st_size];
	BYTE *in_pt = in_buffer;
	infile.read((char *)in_buffer, buf.st_size);

	OBMINFOHEADER obminfohdr;
	memcpy(&obminfohdr, in_buffer, sizeof(obminfohdr));
	in_pt += sizeof(obminfohdr);

	BYTE image_type = obminfohdr.imagetype;
	BYTE bit_count = obminfohdr.bitcount;
	WORD width = obminfohdr.width;
	WORD height = obminfohdr.height;
	
	WORD palette_size;
	DWORD data_size;
	switch (image_type+(bit_count<<8)){
	case 0x0404:
	case 0x0801:
	case 0x0803:
	case 0x0804:
		{
			palette_size = (1<<bit_count)*4;
			if(bit_count==0x04)
			{ data_size = width*height/2; }
			else if(bit_count==0x08)
			{ data_size = width*height; }
			break;
		}
	case 0x1800:
	case 0x2000:
	case 0x1802:
	case 0x1804:
		{
			palette_size = 0;
			data_size = width*height*3;
			break;
		}
	case 0x1801:
	case 0x2001:
	case 0x2003:
	case 0x2004:
		{
			palette_size = 0;
			data_size = width*height*4;
			break;
		}
	case 0x1009:
		{
			//I just don't know how to decode it
			//Info: 16bit true color
			return 1;
			break;
		}
	}
	
	DWORD out_buffer_size = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + palette_size + data_size;
	BYTE *out_buffer = new BYTE[out_buffer_size];
	BYTE *out_pt = out_buffer;
	
	//bitmap header
	BITMAPFILEHEADER bmpfilehdr;
	bmpfilehdr.bfType = 'B'+('M'<<8);
	bmpfilehdr.bfReserved1 = 0;
	bmpfilehdr.bfReserved2 = 0;
	bmpfilehdr.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + palette_size;
	bmpfilehdr.bfSize = bmpfilehdr.bfOffBits + data_size;
	BITMAPINFOHEADER bmpinfohdr;
	bmpinfohdr.biSize = sizeof(BITMAPINFOHEADER);
	bmpinfohdr.biWidth = width;
	bmpinfohdr.biHeight = -height;
	bmpinfohdr.biPlanes = 1;
	bmpinfohdr.biBitCount = bit_count;
	bmpinfohdr.biCompression = 0;
	bmpinfohdr.biSizeImage = 0;
	bmpinfohdr.biXPelsPerMeter = 0;
	bmpinfohdr.biYPelsPerMeter = 0;
	bmpinfohdr.biClrUsed = 0;
	bmpinfohdr.biClrImportant = 0;
	
	memcpy(out_pt, &bmpfilehdr, sizeof(bmpfilehdr));
	out_pt += sizeof(bmpfilehdr);
	memcpy(out_pt, &bmpinfohdr, sizeof(bmpinfohdr));
	out_pt += sizeof(bmpinfohdr);
	
	switch (image_type+(bit_count<<8)){
	case 0x0404:
	case 0x0804:
	case 0x2004:
		{
			//palette(4bit 8bit) data(32bit)
			for(int i=0; i<((bit_count<=0x08)?(1<<bit_count):(width*height)); i++,in_pt+=2,out_pt+=4)
			{
				WORD value = (*in_pt) + (*(in_pt+1)<<8);
				int red = (value&0xF800)>>8;
				int green = (value&0x07C0)>>3;
				int blue = (value&0x003E)<<2;
				int alpha = (value&0x0001)<<7;
				if (alpha != 0) alpha = 0x100;
				*out_pt = blue;
				*(out_pt+1) = green;
				*(out_pt+2) = red;
				*(out_pt+3) = alpha;
			}
			//data(4bit 8bit)
			if(bit_count==0x04 || bit_count==0x08)
			{ memcpy(out_pt, in_pt, data_size); }
			break;
		}
	case 0x0803:
	case 0x2003:
		{
			//palette(8bit) data(32bit)
			for(int i=0; i<((bit_count<=0x08)?(1<<bit_count):(width*height)); i++,in_pt+=2,out_pt+=4)
			{
				WORD value = (*in_pt) + (*(in_pt+1)<<8);
				int red = (value&0xF000)>>8;
				int green = (value&0x0F00)>>4;
				int blue = (value&0x00F0);
				int alpha = (value&0x000F)<<4;
				*out_pt = blue;
				*(out_pt+1) = green;
				*(out_pt+2) = red;
				*(out_pt+3) = alpha;
			}
			//data(8bit)
			if(bit_count==0x08)
			{ memcpy(out_pt, in_pt, data_size); }
			break;
		}
	case 0x1802:
	case 0x1804:
		{
			//data
			for(int i=0; i<(width*height); i++,in_pt+=2,out_pt+=3)
			{
				WORD data_value = (*in_pt) + (*(in_pt+1)<<8);
				int red = (data_value&0xF800)>>8;
				int green = (data_value&0x07E0)>>3;
				int blue = (data_value&0x001F)<<3;
				*out_pt = blue;
				*(out_pt+1) = green;
				*(out_pt+2) = red;
			}
			break;
		}
	case 0x0801:
		{
			memcpy(out_pt, in_pt, palette_size);
			in_pt += palette_size; out_pt += palette_size;
			memcpy(out_pt, in_pt, data_size);
			break;
		}
	case 0x1800:
	case 0x2000:
	case 0x1801:
	case 0x2001:
		{
			if(image_type==0x00)
			{ out_buffer[0x1C] = 0x18;}
			else if(image_type==0x01)
			{ out_buffer[0x1C] = 0x20;}
			memcpy(out_pt, in_pt, data_size);
			break;
		}
	}
	//write the data(palette)
	outfile.write((const char*)out_buffer, out_buffer_size);
	
	delete []in_buffer;
	delete []out_buffer;
	infile.close();
	outfile.close();
	return 0;
}