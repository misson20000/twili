//libpsf v2
//
//This library is used for loading a PC Screen Font and converting it 
//to an abritrary pixel format for storage in arbitrary memory. It is 
//designed to be as light as humanly possible and to be included 
//directly into other projects.
//
//v2 adds support for loading multiple fonts, as well as neccessary API 
//changes. The new API also looks much nicer as it removed camel cased 
//function names.

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "psf.h"

#define NO_ZLIB

#ifndef NO_ZLIB
	#ifdef NO_CLIB
		#warning You have requested zlib support and disabled libc support
		#warning this can not work unless you have rewritten zlib without 
		#warning a dependency on libc. Either disable NO_CLIB or enable NO_ZLIB
	#else
		#include <zlib.h>
		#define OPEN(a)		gzopen(a,"rb")
		#define READ(a,b,c)	gzread(a,b,c)
		#define CLOSE(a)	gzclose(a)
		#define SEEK(a,b)	gzseek(a,b,SEEK_SET)
		typedef gzFile psf_file;
	#endif
#else
	#ifdef NO_CLIB
		#include <sys/syscall.h>
		#include <unistd.h>
		#include <fcntl.h>
		long syscall0(int);
		long syscall1(int,...);
		long syscall2(int,...);
		long syscall3(int,...);
		#define OPEN(a)		syscall2(SYS_open,a,O_RDWR)
		#define READ(a,b,c)	syscall3(SYS_read,a,b,c)
		#define CLOSE(a)	syscall1(SYS_close,a)
		#define SEEK(a,b)	syscall3(SYS_lseek,a,b,SEEK_SET)
		typedef int psf_file;
	#else
		#define OPEN(a)		fopen(a,"rb")
		#define READ(a,b,c)	fread(b,1,c,a)
		#define CLOSE(a)	fclose(a)
		#define SEEK(a,b)	fseek(a,b,SEEK_SET)
		typedef FILE *psf_file;
	#endif
#endif

void psf_open_font(struct psf_font *font, const char *fname)
{
	font->psf_fd=(void *)OPEN(fname);

	return;
}

int psf_get_glyph_size(struct psf_font *font)
{
	return (font->psf_type==PSF_TYPE_1)?font->psf_charsize:font->psf2_charsize;
}

int psf_get_glyph_height(struct psf_font *font)
{
	return (font->psf_type==PSF_TYPE_1)?font->psf_charsize:font->psf2_height;
}

int psf_get_glyph_width(struct psf_font *font)
{
	return (font->psf_type==PSF_TYPE_1)?8:font->psf2_width;
}

int psf_get_glyph_total(struct psf_font *font)
{
	return (font->psf_type==PSF_TYPE_1)?((font->psf_mode&PSF_MODE_512)?512:256):font->psf2_length;
}

void psf_read_header(struct psf_font *font)
{
	READ((psf_file)font->psf_fd,font->psf_magic,2);

	//Check if PSF version 1 format
	if (font->psf_magic[0]==0x36 && font->psf_magic[1]==0x04)
	{
		font->psf_type=PSF_TYPE_1;

		READ((psf_file)font->psf_fd,&font->psf_mode,1);
		READ((psf_file)font->psf_fd,&font->psf_charsize,1);
	}
	else
	{
		//Check if PSF version 2 format
		if (font->psf_magic[0]==0x72 && font->psf_magic[1]==0xB5)
		{
			font->psf2_magic[0]=font->psf_magic[0]; 
			font->psf2_magic[1]=font->psf_magic[1];
			READ((psf_file)font->psf_fd,&font->psf2_magic[2],2);
			if (font->psf2_magic[2]==0x4A && font->psf2_magic[3]==0x86)
			{
				font->psf_type=PSF_TYPE_2;

				READ((psf_file)font->psf_fd,&font->psf2_version,4);
				READ((psf_file)font->psf_fd,&font->psf2_headersize,4);
				READ((psf_file)font->psf_fd,&font->psf2_flags,4);
				READ((psf_file)font->psf_fd,&font->psf2_length,4);
				READ((psf_file)font->psf_fd,&font->psf2_charsize,4);
				READ((psf_file)font->psf_fd,&font->psf2_height,4);
				READ((psf_file)font->psf_fd,&font->psf2_width,4);
				SEEK((psf_file)font->psf_fd,font->psf2_headersize);
			}
			else
				font->psf_type=PSF_TYPE_UNKNOWN;
		}
		else
			font->psf_type=PSF_TYPE_UNKNOWN;
	}

	return;
}

void psf_read_glyph(struct psf_font *font, void *mem, int size, int fill, int clear)
{
	int tmp;
	char tmpglyph[psf_get_glyph_size(font)];

	READ((psf_file)font->psf_fd,tmpglyph,psf_get_glyph_size(font));

	int i,j;
	for (i=0;i<psf_get_glyph_size(font);i++)
	{
		for (j=0;j<8;j++)
		{
			if ((i%(psf_get_glyph_size(font)/psf_get_glyph_height(font))*8)+j<psf_get_glyph_width(font))
			{
				if (tmpglyph[i]&(0x80))
					memcpy(mem,&fill,size);
				else
					memcpy(mem,&clear,size);
				mem+=size;
			}

			tmpglyph[i]=tmpglyph[i]<<1;

		}
	}
}

void psf_close_font(struct psf_font *font)
{
	CLOSE(font->psf_fd);

	return;
}
