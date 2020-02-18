/*
 * Copyright (c) 2020 WangBin <wbsecg1 at gmail.com>
 * MDK SDK + SFML example
 */
#include <SFML/Window.hpp>
#include <SFML/OpenGL.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include "mdk/Player.h"
using namespace MDK_NS;

int main(int argc, char** argv)
{
    // Request a 24-bits depth buffer when creating the window
    sf::ContextSettings contextSettings;
    contextSettings.depthBits = 24;
    // Create the main window
    sf::RenderWindow window(sf::VideoMode(640, 480), "MDK + SFML RenderTexture", sf::Style::Default, contextSettings);
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
    //player.setAspectRatio(IgnoreAspectRatio);
    player.setBackgroundColor(0, 0, 0, -1); // an invalid background can skip glClearColor(), thus we can draw on the scene already has background painted

    sf::RenderTexture texture;
    if (!texture.create(500, 500)) // or use video size, see ../Native/offscreen.cpp
        return -1;
    player.setVideoSurfaceSize(texture.getSize().x, texture.getSize().y);
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
        }
        texture.setActive(true);
        texture.clear(sf::Color::Red);
        player.renderVideo(); // render to current context (texture)
        texture.display();
        window.clear();
        sf::Sprite sprite(texture.getTexture());
        window.draw(sprite);
        // Finally, display the rendered frame on screen
        window.display();
    }
    //player.setVideoSurfaceSize(-1, -1); // it's better to cleanup gl renderer resources
    return EXIT_SUCCESS;
}
