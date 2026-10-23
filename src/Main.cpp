#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <SDL.h>
#include "SDLGL.h"
#include "ZipFile.h"

#include "CAppContainer.h"
#include "App.h"
#include "Image.h"
#include "Resource.h"
#include "Render.h"
#include "GLES.h"

#include "Canvas.h"
#include "Graphics.h"
#include "Player.h"
#include "Game.h"
#include "Graphics.h"
#include "Utils.h"
#include "TinyGL.h"
#include "Input.h"

void drawView(SDLGL* sdlGL);

static bool fileExists(const char* path) {
    SDL_RWops* file = SDL_RWFromFile(path, "rb");
    if (file == nullptr) {
        return false;
    }
    SDL_RWclose(file);
    return true;
}

static void useExecutableDirectory(int argc, char* args[]) {
    if (argc <= 0 || args[0] == nullptr) {
        return;
    }

    const char* slash = strrchr(args[0], '/');
    if (slash == nullptr) {
        return;
    }

    char directory[2048];
    const size_t directoryLength = static_cast<size_t>(slash - args[0]);
    if (directoryLength == 0 || directoryLength >= sizeof(directory)) {
        return;
    }

    memcpy(directory, args[0], directoryLength);
    directory[directoryLength] = '\0';
    chdir(directory);
}

static const char* findIpaPath(int argc, char* args[]) {
    static char path[2048];
    const char* ipaNames[] = {
        "WolfensteinRPG.ipa",
        "Wolfenstein RPG.ipa",
    };

    for (const char* ipaName : ipaNames) {
        if (fileExists(ipaName)) {
            return ipaName;
        }
    }

    char* basePath = SDL_GetBasePath();
    if (basePath != nullptr) {
        for (const char* ipaName : ipaNames) {
            snprintf(path, sizeof(path), "%s%s", basePath, ipaName);
            if (fileExists(path)) {
                SDL_free(basePath);
                return path;
            }
        }
        SDL_free(basePath);
    }

    if (argc > 0 && args[0] != nullptr) {
        const char* slash = strrchr(args[0], '/');
        if (slash != nullptr) {
            const size_t directoryLength = static_cast<size_t>(slash - args[0] + 1);
            for (const char* ipaName : ipaNames) {
                if (directoryLength + strlen(ipaName) < sizeof(path)) {
                    memcpy(path, args[0], directoryLength);
                    strcpy(path + directoryLength, ipaName);
                    if (fileExists(path)) {
                        return path;
                    }
                }
            }
        }
    }

#ifdef WOLFENRPG_PSP
    const char* pspDirectories[] = {
        "ms0:/",
        "ms0:/PSP/GAME/WolfensteinRPG/",
    };
    for (const char* directory : pspDirectories) {
        for (const char* ipaName : ipaNames) {
            snprintf(path, sizeof(path), "%s%s", directory, ipaName);
            if (fileExists(path)) {
                return path;
            }
        }
    }
#endif

    return ipaNames[0];
}

int main(int argc, char* args[]) {
    int		UpTime = 0;

    // SDL's PSP backend does not consistently start in the EBOOT directory.
    // Keep relative assets and saves rooted beside EBOOT.PBP, as DoomRPG does.
    useExecutableDirectory(argc, args);

	SDLGL sdlGL;
	if (!sdlGL.Initialize()) {
		return 1;
	}

    // DoomRPG initializes the platform before opening game data. Do the same
    // so archive failures have a valid video/error-reporting environment.
    ZipFile zipFile;
    const char* ipaPath = findIpaPath(argc, args);
    printf("Using IPA: %s\n", ipaPath);
    zipFile.openZipFile(ipaPath);

    Input input;
    input.init(); // [GEC] Port: set default Binds

    if (!CAppContainer::getInstance()->Construct(&sdlGL, &zipFile)) {
        printf("Failed to initialize Wolfenstein RPG\n");
        zipFile.closeZipFile();
        return 1;
    }
    sdlGL.updateVideo(); // [GEC]
	// SDL's PSP backend resets its tick origin during video initialization.
	// Seed the frame deadline only after initialization so the first frame is
	// not delayed until the 32-bit timer wraps around.
	UpTime = CAppContainer::getInstance()->getTimeMS();

    while (CAppContainer::getInstance()->app->closeApplet != true) {
        int currentTimeMillis = CAppContainer::getInstance()->getTimeMS();
        if (currentTimeMillis > UpTime) {
            input.handleEvents();
            UpTime = currentTimeMillis + 15;
            drawView(&sdlGL);
            input.consumeEvents();
        }
    }

    printf("APP_QUIT\n");
    CAppContainer::getInstance()->Shutdown();
    zipFile.closeZipFile();
	return 0;
}


static uint32_t lastTimems = 0;

void drawView(SDLGL *sdlGL) {
    if (lastTimems == 0) {
        lastTimems = CAppContainer::getInstance()->getTimeMS();
    }

    sdlGL->applyGameViewport();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_ALPHA_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, Applet::IOS_WIDTH, Applet::IOS_HEIGHT, 0.0, -1.0, 1.0);
    //glRotatef(90.0, 0.0, 0.0, 1.0);
    //glTranslatef(0.0, -320.0, 0.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    

    uint32_t startTime = CAppContainer::getInstance()->getTimeMS();
    uint32_t passedTime = startTime - lastTimems;
    lastTimems = startTime;

    if (passedTime >= 125) {
        passedTime = 125;
    }
    //printf("passedTime %d\n", passedTime);

    CAppContainer::getInstance()->DoLoop(passedTime);

#ifdef WOLFENRPG_PSP
	// PSPGL queues commands in a display list; its EGL swap only exchanges
	// buffers and does not submit that list for us.
	glFlush();
	glFinish();
#endif
    SDL_GL_SwapWindow(sdlGL->window);  // Swap the window/pBmp to display the result.
}
