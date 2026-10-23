
#include <stdexcept>

#include "SDLGL.h"
#include "App.h"

#ifdef WOLFENRPG_PSP
#include <pspctrl.h>
#include <psppower.h>
#endif


SDLResVidModes sdlResVideoModes[18] = {
	{480, 320},
	{640, 480},
	{720, 400},
	{720, 480},
	{720, 576},
	{800, 600},
	{832, 624},
	{960, 640},
	{1024, 768},
	{1152, 864},
	{1152, 872},
	{1280, 720},
	{1280, 800},
	{1280, 1024},
	{1440, 900},
	{1600, 1000},
	{1680, 1050},
	{1920, 1080}
};

SDLGL::SDLGL() {
	this->initialized = false;
	this->window = nullptr;
	this->glcontext = nullptr;
#ifdef __linux__
	this->gpSdlWindowIcon = nullptr;
#endif
}

SDLGL::~SDLGL() {
	this->Shutdown();
}

void SDLGL::Shutdown() {
	if (this->glcontext) {
		SDL_GL_DeleteContext(this->glcontext);
		this->glcontext = nullptr;
	}
	if (this->window) {
		SDL_SetWindowFullscreen(this->window, 0);
		SDL_DestroyWindow(this->window);
		this->window = nullptr;
	}
#ifdef __linux__
	if (this->gpSdlWindowIcon) {
		SDL_FreeSurface(this->gpSdlWindowIcon);
		this->gpSdlWindowIcon = nullptr;
 	}
#endif
	if (this->initialized) {
		SDL_Quit();
		this->initialized = false;
	}
}

bool SDLGL::Initialize() {
	Uint32 flags;

	if (!this->initialized) {
		
		SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
		if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
			printf("Could not initialize SDL: %s", SDL_GetError());
			return false;
		}
		this->initialized = true;

		flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN /* | SDL_WINDOW_RESIZABLE*/;
#ifdef WOLFENRPG_PSP
		flags |= SDL_WINDOW_FULLSCREEN;
		scePowerSetClockFrequency(333, 333, 166);
		sceCtrlSetSamplingCycle(0);
		sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
#endif
		// Set the highdpi flags - this makes a big difference on Macs with
		// retina displays, especially when using small window sizes.
#ifndef WOLFENRPG_PSP
		flags |= SDL_WINDOW_ALLOW_HIGHDPI;
#endif

		this->oldResolutionIndex = -1;
		this->resolutionIndex = 0;
#ifdef WOLFENRPG_PSP
		this->winVidWidth = 480;
		this->winVidHeight = 272;
#else
		this->winVidWidth = sdlResVideoModes[this->resolutionIndex].width;//Applet::IOS_WIDTH*2;
		this->winVidHeight = sdlResVideoModes[this->resolutionIndex].height;//Applet::IOS_HEIGHT*2;
#endif

		//this->winVidWidth = 1440;
		//this->winVidHeight = 900;

		// Linux: create the icon for the window
#ifdef __linux__
		this->gpSdlWindowIcon = SDL_CreateRGBSurfaceWithFormatFrom((void*)Applet::gIcon_64_raw_rgb888, 64, 64, 24, 64 * 3, SDL_PIXELFORMAT_RGB24);
#endif

		this->window = SDL_CreateWindow("Wolfenstein RPG By [GEC] Version 0.1", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, winVidWidth, winVidHeight, flags);
		if (!this->window) {
			printf("Could not set %dx%d video mode: %s", winVidWidth, winVidHeight, SDL_GetError());
			this->Shutdown();
			return false;
		}

		this->vidWidth = Applet::IOS_WIDTH;
		this->vidHeight = Applet::IOS_HEIGHT;
		this->oldWindowMode = -1;
		this->windowMode = 0;
		this->oldVSync = false;
		this->vSync = true;

		//SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengles2");
		//SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
		//SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");//sdlVideo.vSync ? "1" : "0");
		// Linux: set the icon for the window
#ifdef __linux__
		if (this->gpSdlWindowIcon) {
			SDL_SetWindowIcon(this->window, this->gpSdlWindowIcon);
		}
#endif

		this->glcontext = SDL_GL_CreateContext(window);
		if (!this->glcontext) {
			printf("Could not create OpenGL context: %s", SDL_GetError());
			this->Shutdown();
			return false;
		}

		// Swap interval and other GL settings require a current context.
		this->updateVideo();

		// now you can make GL calls.
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		SDL_GL_SwapWindow(this->window);

#ifdef _WIN32
		// Check for joysticks
		SDL_SetHint(SDL_HINT_JOYSTICK_RAWINPUT, "0");
#endif
	}

	return this->initialized;
}


#include <stdarg.h> //va_list|va_start|va_end

void SDLGL::Error(const char* fmt, ...)
{
	char errMsg[256];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(errMsg, sizeof(errMsg), fmt, ap);
	va_end(ap);

	printf("%s", errMsg);

	const SDL_MessageBoxButtonData buttons[] = {
		{ /* .flags, .buttonid, .text */        0, 0, "Ok" },
	};
	const SDL_MessageBoxColorScheme colorScheme = {
		{ /* .colors (.r, .g, .b) */
			/* [SDL_MESSAGEBOX_COLOR_BACKGROUND] */
			{ 255,   0,   0 },
			/* [SDL_MESSAGEBOX_COLOR_TEXT] */
			{   0, 255,   0 },
			/* [SDL_MESSAGEBOX_COLOR_BUTTON_BORDER] */
			{ 255, 255,   0 },
			/* [SDL_MESSAGEBOX_COLOR_BUTTON_BACKGROUND] */
			{   0,   0, 255 },
			/* [SDL_MESSAGEBOX_COLOR_BUTTON_SELECTED] */
			{ 255,   0, 255 }
		}
	};
	const SDL_MessageBoxData messageboxdata = {
		SDL_MESSAGEBOX_ERROR, /* .flags */
		NULL, /* .window */
		"Wolfenstein RPG Error", /* .title */
		errMsg, /* .message */
		SDL_arraysize(buttons), /* .numbuttons */
		buttons, /* .buttons */
		&colorScheme /* .colorScheme */
	};

	SDL_ShowMessageBox(&messageboxdata, NULL);
	this->Shutdown();
	exit(0);
}


void SDLGL::transformCoord2f(float* x, float* y) {
	#ifdef WOLFENRPG_PSP
	*x = 36.0f + (*x * (408.0f / (float)Applet::IOS_WIDTH));
	*y *= 272.0f / (float)Applet::IOS_HEIGHT;
	#else
	int w, h;
	SDL_GetWindowSize(this->window, &w, &h);
	*x *= (float)w / (float)Applet::IOS_WIDTH;
	*y *= (float)h / (float)Applet::IOS_HEIGHT;
	#endif
}

void SDLGL::transformViewportRect(int* x, int* y, int* w, int* h) {
#ifdef WOLFENRPG_PSP
	*x = 36 + (*x * 408) / Applet::IOS_WIDTH;
	*y = (*y * 272) / Applet::IOS_HEIGHT;
	*w = (*w * 408) / Applet::IOS_WIDTH;
	*h = (*h * 272) / Applet::IOS_HEIGHT;
#else
	int windowWidth, windowHeight;
	SDL_GetWindowSize(this->window, &windowWidth, &windowHeight);
	*x = (*x * windowWidth) / Applet::IOS_WIDTH;
	*y = (*y * windowHeight) / Applet::IOS_HEIGHT;
	*w = (*w * windowWidth) / Applet::IOS_WIDTH;
	*h = (*h * windowHeight) / Applet::IOS_HEIGHT;
#endif
}

void SDLGL::applyGameViewport() {
#ifdef WOLFENRPG_PSP
	glViewport(36, 0, 408, 272);
	glScissor(36, 0, 408, 272);
#else
	int width, height;
	SDL_GetWindowSize(this->window, &width, &height);
	glViewport(0, 0, width, height);
	glScissor(0, 0, width, height);
#endif
}

void SDLGL::centerMouse(int x, int y) {
	float X = (float)((Applet::IOS_WIDTH / 2) + x);
	float Y = (float)((Applet::IOS_HEIGHT / 2) + y);
	this->transformCoord2f(&X, &Y);
	SDL_WarpMouseInWindow(this->window, (int)X, (int)Y);
}

void SDLGL::updateWinVid(int w, int h) {
	this->winVidWidth = w;
	this->winVidHeight = h;
}

void SDLGL::updateVideo() {
#ifdef WOLFENRPG_PSP
	this->updateWinVid(480, 272);
	return;
#endif
	if (this->vSync != this->oldVSync) {
		SDL_GL_SetSwapInterval(this->vSync ? 1 : 0);
		this->oldVSync = this->vSync;
	}

	if (this->windowMode != this->oldWindowMode) {
		SDL_SetWindowFullscreen(this->window, 0);
		SDL_SetWindowBordered(this->window, SDL_TRUE);

		if (this->windowMode == 1) {
			SDL_SetWindowBordered(this->window, SDL_FALSE);
		}
		else if (this->windowMode == 2) {
			SDL_SetWindowFullscreen(this->window, SDL_WINDOW_FULLSCREEN);
		}
		this->oldWindowMode = this->windowMode;
	}

	if (this->resolutionIndex != this->oldResolutionIndex) {
		SDL_SetWindowSize(this->window, sdlResVideoModes[this->resolutionIndex].width, sdlResVideoModes[this->resolutionIndex].height);
		SDL_SetWindowPosition(this->window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
		this->updateWinVid(sdlResVideoModes[this->resolutionIndex].width, sdlResVideoModes[this->resolutionIndex].height);
		this->oldResolutionIndex = this->resolutionIndex;
	}

	int winVidWidth, winVidHeight;
	SDL_GetWindowSize(this->window, &winVidWidth, &winVidHeight);
	this->updateWinVid(winVidWidth, winVidHeight);
}

void SDLGL::restore() {
	if (this->vSync != this->oldVSync) {
		this->vSync = this->oldVSync;
	}

	if (this->windowMode != this->oldWindowMode) {
		this->windowMode = this->oldWindowMode;
	}

	if (this->resolutionIndex != this->oldResolutionIndex) {
		this->resolutionIndex = this->oldResolutionIndex;
	}
}
