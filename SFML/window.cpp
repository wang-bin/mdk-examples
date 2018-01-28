/*
 * Copyright (c) 2016-2018 WangBin <wbsecg1 at gmail.com>
 * MDK SDK + SFML example
 */
#include <SFML/Window.hpp>
#include <SFML/OpenGL.hpp>
#include "mdk/Player.h"
using namespace MDK_NS;

int main(int argc, char** argv)
{
    // Request a 24-bits depth buffer when creating the window
    sf::ContextSettings contextSettings;
    contextSettings.depthBits = 24;
    // Create the main window
    sf::Window window(sf::VideoMode(640, 480), "MDK + SFML window with OpenGL", sf::Style::Default, contextSettings);
    // Make it the active window for OpenGL calls
    window.setActive();

    Player player;
    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "-c:v") == 0) {
            i++;
            player.setVideoDecoders({argv[i]});
        }
    }
    player.setMedia(argv[argc-1]);
    player.setVideoSurfaceSize(window.getSize().x, window.getSize().y);
    //player.setRenderCallback();
    player.setState(State::Playing);

    // Start the game loop
    while (window.isOpen()) {
        // Process events
        sf::Event event;
        while (window.pollEvent(event)) {
            // Close window: exit
            if (event.type == sf::Event::Closed)
                window.close();
            // Escape key: exit
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Space)
                    player.setState(player.state() == State::Playing ? State::Paused : State::Playing);
                else if (event.key.code == sf::Keyboard::Right)
                    player.seek(player.position()+10000);
                else if (event.key.code == sf::Keyboard::Left)
                    player.seek(player.position()-10000);
            }
            // Resize event: adjust the viewport
            if (event.type == sf::Event::Resized)
                player.setVideoSurfaceSize(event.size.width, event.size.height);
        }
        player.renderVideo();
        // Finally, display the rendered frame on screen
        window.display();
    }
    player.destroyRenderer();
    return EXIT_SUCCESS;
}