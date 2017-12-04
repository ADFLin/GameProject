#ifndef Dependence_h__
#define Dependence_h__

#define USE_SFML 0
#include <gl/glew.h>
#pragma comment (lib,"OpenGL32.lib")
#pragma comment (lib,"GLu32.lib")

#if USE_SFML
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#endif

#include <vector>
#include "WindowsHeader.h"

#endif // Dependence_h__