#include <stdio.h>
#include <SDL/SDL.h>
#include "psf.h"

struct psf_font font;

SDL_Surface *screen,*glyph[512];

//This function creates a SDL_Surface in the same format as the screen
SDL_Surface *CreateSurface(Uint32 flags,int width,int height,const SDL_Surface* display)
{
	const SDL_PixelFormat fmt = *(display->format);
	return SDL_CreateRGBSurface(flags,width,height,fmt.BitsPerPixel,fmt.Rmask,fmt.Gmask,fmt.Bmask,fmt.Amask);
}

//This function displays one character at a specific location
void DisplayChar(unsigned char c,int x,int y)
{
	SDL_Rect dest,src;
	src.x=0;
	src.y=0;
	src.w=screen->w;
	src.h=screen->h;

	dest.x=x;
	dest.y=y;
	dest.w=glyph[c]->w;
	dest.h=glyph[c]->h;

	//All we have to do is blit the glyph to the screen
	SDL_BlitSurface(glyph[c],&src,screen,&dest);
}

//This function displays one entire string at a specific location
void DisplayStr(char *str,int x,int y)
{
	int i=0,cursorx=0,cursory=0;

	for (i=0;str[i]!='\0';i++)
	{
		if (str[i]==0x0D)
		{
			cursory++;
			cursorx=0;
		}
		else
			DisplayChar(str[i],x+(psf_get_glyph_width(&font)*cursorx++),y+(psf_get_glyph_height(&font)*cursory));
	}

	return;
}

//Entry point
int main(int argc, char **argv)
{
	//First, we must choose a font and open it.
	if (argc==1)
		psf_open_font(&font,"font.psf"); //Use ./font.psf if no font is specified
	else
		psf_open_font(&font,argv[1]); //Allow the user to choose a font

	//Once the font is open, we have to first read the font's header
	//This will allow us to get information about the font
	//which is required for rendering (and reading)
	psf_read_header(&font);

	//Setup SDL and create a window where we can display the characters
	SDL_Init(SDL_INIT_EVERYTHING);
	screen=SDL_SetVideoMode(320,240,32,SDL_DOUBLEBUF);
	//Unicode allows us to display exactly which character the user types
	SDL_EnableUNICODE(1);

	//psf_get_glyph_total tells us how many glyphs are in the font
	int i;
	for (i=0;i<psf_get_glyph_total(&font);i++)
	{
		//Create a surface of exactly the right size for each glyph
		glyph[i]=CreateSurface(0,psf_get_glyph_width(&font),psf_get_glyph_height(&font),screen);

		//Read the glyph directly into the surface's memory
		psf_read_glyph(&font,glyph[i]->pixels,4,0x00000000,0xFFFFFFFF);
	}

	//After reading all the glyphs, close the font
	//We already have all the glyphs stored in memory
	psf_close_font(&font);

	//If the program should end
	int quit = 0;

	//Buffer for characters to be rendered
	char buf[512];

	//Where in the string to add new characters
	int pos=0;

	//Standard SDL event loop
	SDL_Event event;
	while (!quit)
	{
		while(SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
			{
				quit = 1;
			}

			if (event.type == SDL_KEYDOWN)
			{
                            switch(event.key.keysym.sym) {
                            case SDLK_ESCAPE:
                                quit = 1;
                                break;
                            default:
                                if (event.key.keysym.unicode)                  
				{
					//Add the user's character to the string
					buf[pos++]=event.key.keysym.unicode;
					buf[pos]='\0';	//Make sure it's null terminated
				}
                            }
			}
		}

		//Clear the screen (the sloppy way)
		SDL_FillRect(screen,NULL,SDL_MapRGB(screen->format,0xFF,0xFF,0xFF));

		//Display the string
		DisplayStr(buf,50,50);

		SDL_Flip(screen); //Display everything
		usleep(100000); //Don't hog all the CPU
	}

        SDL_Quit();
}

