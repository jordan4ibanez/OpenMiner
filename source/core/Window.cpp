/*
 * =====================================================================================
 *
 *       Filename:  Window.cpp
 *
 *    Description:
 *
 *        Created:  20/12/2014 00:17:10
 *
 *         Author:  Quentin Bazin, <quent42340@gmail.com>
 *
 * =====================================================================================
 */
#include "OpenGL.hpp"
#include "Exception.hpp"
#include "Window.hpp"

void Window::open(const std::string &caption, u16 width, u16 height) {
	sf::ContextSettings settings;
	settings.majorVersion = 2;
	settings.minorVersion = 1;
	settings.depthBits = 24;
	settings.stencilBits = 8;
	settings.antialiasingLevel = 0;

	m_window.create(sf::VideoMode(width, height), caption, sf::Style::Titlebar | sf::Style::Close, settings);
	m_window.setPosition({int(sf::VideoMode::getDesktopMode().width / 2  - width / 2),
						  int(sf::VideoMode::getDesktopMode().height / 2 - height / 2)});

	m_width = width;
	m_height = height;

	m_isOpen = true;
}

void Window::clear() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Window::display() {
	m_window.display();
}

void Window::setVerticalSyncEnabled(bool enabled) {
	m_window.setVerticalSyncEnabled(enabled);
}

