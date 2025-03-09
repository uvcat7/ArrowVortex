#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_render.h>

/*
 * sorry for macro abuse - sjplus
 */

#define ARROWVORTEX_TITLE "ArrowVortex"
#define ARROWVORTEX_DIMENSIONS 900, 900

int main(int argc, char** argv)
{
	SDL_Window* window;
	SDL_Renderer* renderer;

	bool finished = false;

	SDL_Init(SDL_INIT_VIDEO);

	window = SDL_CreateWindow(
			ARROWVORTEX_TITLE,
			ARROWVORTEX_DIMENSIONS,
			SDL_WINDOW_OPENGL
		);
	renderer = SDL_CreateRenderer(window, NULL);
	
	if (!window)
	{
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Window couldn't be created, %s\n", SDL_GetError());
		return 1;
	}

	while (!finished)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_EVENT_QUIT:
				{
					finished = true;
				}
				break;
			}

			SDL_RenderClear(renderer);
			SDL_RenderPresent(renderer);
		}
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();
	return 0;
}
