#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>

int main() {
    static const int w = 1024, h = 1024;

    SDL_Window    *gWindow = NULL;
    SDL_GLContext *gContext = NULL;
    bool           gRenderQuad = true;
    float          angle = 0;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init() failed: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    if ((gWindow = SDL_CreateWindow("Cube",
                                    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h,
                                    SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN)) == NULL) {
        fprintf(stderr, "SDL_CreateWindow() failed: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    if ((gContext = SDL_GL_CreateContext(gWindow)) == NULL) {
        fprintf(stderr, "SDL_GL_CreateContext() failed: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    if (SDL_GL_SetSwapInterval(1) < 0) {
        fprintf(stderr, "SDL_GL_SetSwapInterval(1) failed: %s\n", SDL_GetError());
    }

    GLenum error = GL_NO_ERROR;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    if ((error = glGetError()) != GL_NO_ERROR) {
        fprintf(stderr, "GL init failed: %s\n", gluErrorString(error));
        return EXIT_FAILURE;
    }

    bool has_quit_event = false;
    SDL_Event event;
    SDL_StartTextInput();

    while (!has_quit_event) {
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT
                || (event.type == SDL_KEYDOWN
                    && event.key.keysym.sym == SDLK_ESCAPE)) {
                has_quit_event = true;
            }
        }
        angle = angle + 1 % 360;

        usleep(10000);

        glLoadIdentity();
        glRotatef(angle, 1.0f, 0.5f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        if (gRenderQuad) {
            glBegin(GL_QUADS);

            glColor3f(0.5f, 0.0f, 0.5f);
            glVertex3f(-0.5f, -0.5f, -0.5f);
            glVertex3f(0.5f, -0.5f, -0.5f);
            glVertex3f(0.5f, 0.5f, -0.5f);
            glVertex3f(-0.5f, 0.5f, -0.5f);

            glColor3f(0.4f, 0.0f, 0.4f);
            glVertex3f(-0.5f, -0.5f, 0.5f);
            glVertex3f(0.5f, -0.5f, 0.5f);
            glVertex3f(0.5f, 0.5f, 0.5f);
            glVertex3f(-0.5f, 0.5f, 0.5f);

            glColor3f(0.45f, 0.0f, 0.45f);
            glVertex3f(-0.5f, 0.5f, -0.5f);
            glVertex3f(-0.5f, 0.5f, 0.5f);
            glVertex3f(0.5f, 0.5f, 0.5f);
            glVertex3f(0.5f, 0.5f, -0.5f);

            glColor3f(0.65f, 0.0f, 0.65f);
            glVertex3f(-0.5f, -0.5f, -0.5f);
            glVertex3f(-0.5f, -0.5f, 0.5f);
            glVertex3f(0.5f, -0.5f, 0.5f);
            glVertex3f(0.5f, -0.5f, -0.5f);

            glColor3f(0.55f, 0.0f, 0.55f);
            glVertex3f(0.5f, -0.5f, -0.5f);
            glVertex3f(0.5f, 0.5f, -0.5f);
            glVertex3f(0.5f, 0.5f, 0.5f);
            glVertex3f(0.5f, -0.5f, 0.5f);

            glColor3f(0.6f, 0.0f, 0.6f);
            glVertex3f(-0.5f, -0.5f, -0.5f);
            glVertex3f(-0.5f, 0.5f, -0.5f);
            glVertex3f(-0.5f, 0.5f, 0.5f);
            glVertex3f(-0.5f, -0.5f, 0.5f);

            glEnd();

            glPopMatrix();

            glEnable(GL_DEPTH_TEST);
        }

        SDL_GL_SwapWindow(gWindow);
    }

    SDL_StopTextInput();

    if (gContext) {
        SDL_GL_DeleteContext(gContext);
        gContext = NULL;
    }

    if (gWindow) {
        SDL_DestroyWindow(gWindow);
        gWindow = NULL;
    }

    SDL_Quit();

    return EXIT_SUCCESS;
}
