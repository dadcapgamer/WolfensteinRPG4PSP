#ifndef __SDLGL_H__
#define __SDLGL_H__

#include <SDL.h>
#include <SDL_opengl.h>
#if !defined(WOLFENRPG_PSP) && !defined(__APPLE__)
#include <SDL_opengles2.h>
#endif
#include <SDL_gamecontroller.h>
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

typedef struct SDLResVidModes_s {
	int width, height;
} SDLResVidModes;
extern SDLResVidModes sdlResVideoModes[18];

class SDLGL
{
private:
	bool initialized;
	SDL_GLContext glcontext;
#ifdef __linux__
	// Linux only: an icon to use for the window (SDL)
	SDL_Surface* gpSdlWindowIcon;
#endif

public:
	SDL_Window* window;
	int resolutionIndex;
	int oldResolutionIndex;
	int winVidWidth, winVidHeight;
	int vidWidth, vidHeight;
	int windowMode, oldWindowMode;
	bool vSync, oldVSync;

	SDLGL();
	// Destructor
	~SDLGL();
	bool Initialize();
	void Shutdown();
	void Error(const char* fmt, ...);

	void transformCoord2f(float* x, float* y);
	void transformViewportRect(int* x, int* y, int* w, int* h);
	void applyGameViewport();
	void centerMouse(int x, int y);
	void updateWinVid(int w, int h);
	void updateVideo();
	void restore();
};

#endif
