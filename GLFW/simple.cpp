
/*
 * Copyright (c) 2016-2018 WangBin <wbsecg1 at gmail.com>
 * MDK SDK + GLFW example
 */
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>
#include "mdk/Player.h"
using namespace MDK_NS;

Player player;

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action != GLFW_PRESS)
        return;
    switch (key) {
        case GLFW_KEY_SPACE:
            player.setState(player.state() == State::Playing ? State::Paused : State::Playing);
            break;  
        case GLFW_KEY_RIGHT:
            player.seek(player.position()+10000);
            break;
        case GLFW_KEY_LEFT:
            player.seek(player.position()-10000);
            break;
    }
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        printf("usage: %s [-c:v decoder] video_url\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "-c:v") == 0) {
            i++;
            player.setVideoDecoders({argv[i]});
        }
    }
    player.setMedia(argv[argc-1]);
    player.setRenderCallback([](void*){
        glfwPostEmptyEvent();
    });
    player.setState(State::Playing);

    glfwSetErrorCallback([](int error, const char* description) {
        fprintf(stderr, "Error: %s\n", description);
    });

    if (!glfwInit())
        exit(EXIT_FAILURE);

    const int w = 640, h = 480;
    player.setVideoSurfaceSize(w, h);
    GLFWwindow *win = glfwCreateWindow(w, h, "MDK + GLFW", nullptr, nullptr);
    if (!win) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwSetKeyCallback(win, key_callback);
    glfwSetWindowSizeCallback(win, [](GLFWwindow*, int w,int h){
        player.setVideoSurfaceSize(w, h);
    });
    glfwShowWindow(win);

    while (!glfwWindowShouldClose(win)) {
        glfwMakeContextCurrent(win);
        player.renderVideo();
        glfwSwapBuffers(win);
        //glfwWaitEventsTimeout(0.04); // if player.setRenderCallback() is not called, render at 25fps
        glfwWaitEvents();
    }
    player.destroyRenderer();
    glfwTerminate();
    exit(EXIT_SUCCESS);
}

