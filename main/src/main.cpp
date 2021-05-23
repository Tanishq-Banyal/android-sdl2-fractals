#include "SDL.h"

#define _USE_MATH_DEFINES
#define _CRT_SECURE_NO_WARNINGS

#include <cmath>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <complex>

using namespace std;
using namespace complex_literals;

#define WIDTH			720
#define HEIGHT			1280

#define START_POS_X		0
#define START_POS_Y		0
#define START_ZOOM		(WIDTH * 0.2f)

#define BAIL_OUT		5
#define STRIPS			30

#define ZOOM_FACTOR		1.5
#define DETAILS			0.1

#define SEED			2
#define STRIP_BY_STRIP

complex<double> J(0.0, 1.0);
int SKIPS = HEIGHT / STRIPS;

char* GetAbsCwdPath()
{
	#ifndef WIN32
	// Linux And Android
	char abs_cwd_path[PATH_MAX];
	realpath("./", abs_cwd_path);
	#else
	// Windows (idk why)
	//char abs_cwd_path[MAX_PATH];
	#endif
	
	return abs_cwd_path;
}

char* GetUniqueName()
{
	time_t raw_time = time(NULL);
	struct tm* time_now = localtime(&raw_time);

	char unik_name[50] = "";
	strcat(unik_name, "./Shots/");
	strcat(unik_name, "Shot ");
	strcat(unik_name, asctime(time_now)); unik_name[strlen(unik_name) - 6] = '\0';
	strcat(unik_name, ".bmp");

	for (int i = 0; unik_name[i]; i++)
	{
		if (unik_name[i] == ' ')
		{
			unik_name[i] = '_';
		}
		else if (unik_name[i] == ':')
		{
			unik_name[i] = '-';
		}
	}
	
	/*
	// Checking if File Already Exists
	if (FILE* f = fopen(unik_name, "rb"))
	{
		fclose(f);
		strcat(unik_name, "_");
	}
	else
	{
		strcat(unik_name, "_a");
	}
	*/
	return unik_name;
}

void sdl_draw_mandelbrot(SDL_Window* window, SDL_Surface* surface, int CenterX, int CenterY, double zoom)
{
	int f, x, y, n;
	int maxiter = (WIDTH / 2) * DETAILS * log10(zoom);
	complex<double> z, c;
	float C;

	fprintf(stderr, "zoom: %f\n", zoom);
	fprintf(stderr, "center point: %d, %d\n", CenterX, CenterY);
	fprintf(stderr, "iterations: %d\n", maxiter);

	for (f = 0; f < SKIPS; f++)
	{
		#ifdef STRIP_BY_STRIP
		for (y = SKIPS*f; y < SKIPS*(f+1); y++)		// Top to Bottom
		#else
		for (y = f; y < HEIGHT; y += SKIPS)		// Top to Bottom
		#endif
		{
			for (x = 0; x < WIDTH; x++)			// Left to Right
			{
				// Get the complex point on gauss space to be calculate
				z = c = CenterX + ((x - (WIDTH / 2)) / zoom) +
					((CenterY + ((y - (HEIGHT / 2)) / zoom)) * J);

				#define X real(z)
				#define Y imag(z)
				
				// Check if point lies within the main cardiod or in the period-2 buld
				if ((pow(X-0.25, 2)+pow(Y, 2))*(pow(X, 2)+(X / 2)+pow(Y, 2)-0.1875) < pow(Y, 2)/4 || pow(X+1, 2)+pow(Y, 2) < 0.0625)
					n = maxiter;
				else
					/* Applies the actual mandelbrot formula on that point */
					for (n = 0; n <= maxiter && abs(z) < BAIL_OUT; n++)
						z = pow(z, SEED) + c;

				C = n - log2f(logf(abs(z)) / M_LN2);

				// Paint the pixel calculated depending on the number of iterations found
				((Uint32*)surface->pixels)[(y * surface->w) + x] = (n >= maxiter) ? 0 :
					SDL_MapRGB(surface->format,
						(1 + sin(C * 0.27 + 5)) * 127., (1 + cos(C * 0.85)) * 127., (1 + sin(C * 0.15)) * 127.);
			}
		}

		SDL_UpdateWindowSurface(window);
	}
}

int main()
{
	#ifdef _unix_
	freopen("output.txt", "w", stderr);
	#endif
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		fprintf(stderr, "Could not initialize SDL: %s\n", SDL_GetError());
		exit(1);
	}
	atexit(SDL_Quit);

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");
	SDL_Window* window;

	window = SDL_CreateWindow("SDL Mandelbrot",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		WIDTH,
		HEIGHT,
		SDL_WINDOW_OPENGL
	);

	SDL_Surface* surface = SDL_GetWindowSurface(window);

	/* Initialize variables */
	int CenterX = START_POS_X;
	int CenterY = START_POS_Y;
	double zoom = START_ZOOM;

	sdl_draw_mandelbrot(window, surface, CenterX, CenterY, zoom);

	SDL_Event event;
	while (1)
	{
		SDL_PollEvent(&event);
		switch (event.type)
		{
		case SDL_QUIT:
			exit(0);
			break;

		case SDL_KEYDOWN:
			if (event.key.keysym.sym == SDLK_TAB)
			{
				CenterX = START_POS_X;
				CenterY = START_POS_Y;
				zoom = START_ZOOM;
				sdl_draw_mandelbrot(window, surface, CenterX, CenterY, zoom);
			}
			else if (event.key.keysym.sym == SDLK_ESCAPE)
			{
				exit(0);
			}
			else if (event.key.keysym.sym == SDLK_SPACE)
			{
				//SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_ARGB8888, surface->pixels, surface->pitch);
				if (SDL_SaveBMP(surface, GetUniqueName()) != 0)
					fprintf(stderr, "Save BMP Failed : %s\n", SDL_GetError());
				SDL_FreeSurface(surface);
			}
			break;

		case SDL_MOUSEBUTTONDOWN:

			CenterX = (CenterX + (event.button.x - (WIDTH / 2))) / (zoom/ZOOM_FACTOR);
			CenterY = (CenterY + (event.button.y - (HEIGHT / 2))) / (zoom/ZOOM_FACTOR);

			fprintf(stderr, "\nMouse Click : %d, %d\n", event.button.x, event.button.y);
			fprintf(stderr, "Mouse Diff : %d, %d\n\n", (WIDTH / 2) - event.button.x, (HEIGHT / 2) - event.button.y);

			if (event.button.button == 1)
				zoom *= ZOOM_FACTOR;
			else if (event.button.button == 3)
				zoom /= ZOOM_FACTOR;
				
			if (SDL_SaveBMP(surface, GetUniqueName()) != 0)
				fprintf(stderr, "Save BMP Failed : %s\n", SDL_GetError());
			SDL_FreeSurface(surface);

			sdl_draw_mandelbrot(window, surface, CenterX, CenterY, zoom);
			break;
		}
	}

	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
