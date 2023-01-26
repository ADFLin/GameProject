#include "PlatformSFML.h"


#if USE_SFML

static sf::Clock gClock;
int64 PlatformSFML::GetTickCount()
{
	return  gClock.getElapsedTime().asMilliseconds();
}

PlatformWindow* PlatformSFML::CreateWindow( char const* title , Vec2i const& size , int colorBit , bool bFullScreen )
{
	PlatformWindow* win = new PlatformWindow;
	if ( !win->create( title , size , colorBit , bFullScreen ) )
	{
		delete win;
		return NULL;
	}
	return win;
}

PlatformGLContext* PlatformSFML::CreateGLContext( PlatformWindow& window , GLConfig& config )
{
	PlatformGLContext* context = new PlatformGLContext;
	if ( !context->init( window , config ) )
	{
		delete context;
		return NULL;
	}
	return context;
}

sf::Keyboard::Key convertToSFKey( unsigned key )
{
	//FIXME
	switch( key )
	{
	case EKeyCode::Cancel : break;

	case EKeyCode::Back:           break;
	case EKeyCode::Tab:            break;
	case EKeyCode::Clear:          break;
	case EKeyCode::Return:         break;

	case EKeyCode::Shift:          break;
	case EKeyCode::Control:        break;
	case EKeyCode::Menu:           break;
	case EKeyCode::Pause:          break;
	case EKeyCode::Capital:        break;

	case EKeyCode::Kana:           break;
	case EKeyCode::Junja:          break; 
	case EKeyCode::Final:          break;
	case EKeyCode::Hanja:          break;

	case EKeyCode::Escape:         break;

	case EKeyCode::Convert:        break;
	case EKeyCode::NonConvert:     break;
	case EKeyCode::Accept:         break;
	case EKeyCode::ModeChange:     break;

	case EKeyCode::Space:          break;
	case EKeyCode::Prior:          break;
	case EKeyCode::Next:           break;
	case EKeyCode::End:            break;
	case EKeyCode::Home:           break;
	case EKeyCode::Left:           break;
	case EKeyCode::Up:             break;
	case EKeyCode::Right:          break;
	case EKeyCode::Down:           break;
	case EKeyCode::Select:         break;
	case EKeyCode::Print:          break;
	case EKeyCode::Execute:        break;
	case EKeyCode::Snapshot:       break;
	case EKeyCode::Insert:         break;
	case EKeyCode::Delete:         break;
	case EKeyCode::Help:           break;

	case EKeyCode::Num0: return sf::Keyboard::Num0; 
	case EKeyCode::Num1: return sf::Keyboard::Num1; 
	case EKeyCode::Num2: return sf::Keyboard::Num2; 
	case EKeyCode::Num3: return sf::Keyboard::Num3; 
	case EKeyCode::Num4: return sf::Keyboard::Num4; 
	case EKeyCode::Num5: return sf::Keyboard::Num5; 
	case EKeyCode::Num6: return sf::Keyboard::Num6; 
	case EKeyCode::Num7: return sf::Keyboard::Num7; 
	case EKeyCode::Num8: return sf::Keyboard::Num8; 
	case EKeyCode::Num9: return sf::Keyboard::Num9; 
	case EKeyCode::A: return sf::Keyboard::A;    
	case EKeyCode::B: return sf::Keyboard::B;  
	case EKeyCode::C: return sf::Keyboard::C;  
	case EKeyCode::D: return sf::Keyboard::D;  
	case EKeyCode::E: return sf::Keyboard::E;  
	case EKeyCode::F: return sf::Keyboard::F;  
	case EKeyCode::G: return sf::Keyboard::G;  
	case EKeyCode::H: return sf::Keyboard::H;  
	case EKeyCode::I: return sf::Keyboard::I;  
	case EKeyCode::J: return sf::Keyboard::J;  
	case EKeyCode::K: return sf::Keyboard::K;   
	case EKeyCode::L: return sf::Keyboard::L; 
	case EKeyCode::M: return sf::Keyboard::M; 
	case EKeyCode::N: return sf::Keyboard::N; 
	case EKeyCode::O: return sf::Keyboard::O; 
	case EKeyCode::P: return sf::Keyboard::P;
	case EKeyCode::Q: return sf::Keyboard::Q; 
	case EKeyCode::R: return sf::Keyboard::R; 
	case EKeyCode::S: return sf::Keyboard::S; 
	case EKeyCode::T: return sf::Keyboard::T; 
	case EKeyCode::U: return sf::Keyboard::U;
	case EKeyCode::V: return sf::Keyboard::V; 
	case EKeyCode::W: return sf::Keyboard::W; 
	case EKeyCode::X: return sf::Keyboard::X; 
	case EKeyCode::Y: return sf::Keyboard::Y; 
	case EKeyCode::Z: return sf::Keyboard::Z;

	case EKeyCode::Sleep:                    break;

	case EKeyCode::Numpad0: return sf::Keyboard::Numpad0;
	case EKeyCode::Numpad1: return sf::Keyboard::Numpad1;
	case EKeyCode::Numpad2: return sf::Keyboard::Numpad2;
	case EKeyCode::Numpad3: return sf::Keyboard::Numpad3;
	case EKeyCode::Numpad4: return sf::Keyboard::Numpad4;
	case EKeyCode::Numpad5: return sf::Keyboard::Numpad5;
	case EKeyCode::Numpad6: return sf::Keyboard::Numpad6;
	case EKeyCode::Numpad7: return sf::Keyboard::Numpad7;
	case EKeyCode::Numpad8: return sf::Keyboard::Numpad8;
	case EKeyCode::Numpad9: return sf::Keyboard::Numpad9;

	case EKeyCode::Multiply:                  break;
	case EKeyCode::Add:                       break;
	case EKeyCode::Separator:                 break;
	case EKeyCode::Subtract:                  break;
	case EKeyCode::Decimal:                   break;
	case EKeyCode::Divide:                    break;

	case EKeyCode::F1:  return sf::Keyboard::F1;
	case EKeyCode::F2:  return sf::Keyboard::F2;
	case EKeyCode::F3:  return sf::Keyboard::F3; 
	case EKeyCode::F4:  return sf::Keyboard::F4; 
	case EKeyCode::F5:  return sf::Keyboard::F5;
	case EKeyCode::F6:  return sf::Keyboard::F6;
	case EKeyCode::F7:  return sf::Keyboard::F7; 
	case EKeyCode::F8:  return sf::Keyboard::F8;
	case EKeyCode::F9:  return sf::Keyboard::F9;
	case EKeyCode::F10: return sf::Keyboard::F10;
	case EKeyCode::F11: return sf::Keyboard::F11;
	case EKeyCode::F12: return sf::Keyboard::F12; 
	case EKeyCode::F13: return sf::Keyboard::F13;
	case EKeyCode::F14: return sf::Keyboard::F14; 
	case EKeyCode::F15: return sf::Keyboard::F15;

	case EKeyCode::NumLock:  break;
	case EKeyCode::Scroll:   break;

	case EKeyCode::LShift:    return sf::Keyboard::LShift;
	case EKeyCode::RShift:    return sf::Keyboard::RShift;
	case EKeyCode::LControl:  return sf::Keyboard::LControl;
	case EKeyCode::RControl:  return sf::Keyboard::RControl;
	case EKeyCode::LMenu:     return sf::Keyboard::LAlt;
	case EKeyCode::RMenu:     return sf::Keyboard::RAlt;
	}

	return sf::Keyboard::Unknown;
}

bool PlatformSFML::IsKeyPressed( unsigned key )
{
	return sf::Keyboard::isKeyPressed( convertToSFKey( key ) );
}

bool PlatformSFML::IsButtonPressed( unsigned button )
{
	sf::Mouse::Button buttonSF;
	switch (button)
	{
	case EMouseKey::Left:  buttonSF = sf::Mouse::Left;     break;
	case EMouseKey::Right:  buttonSF = sf::Mouse::Right;    break;
	case EMouseKey::Middle:  buttonSF = sf::Mouse::Middle;   break;
	case EMouseKey::X1: buttonSF = sf::Mouse::XButton1; break;
	case EMouseKey::X2: buttonSF = sf::Mouse::XButton2; break;
	}
	return sf::Mouse::isButtonPressed( buttonSF );
}

GameWindowSFML::GameWindowSFML()
{

	mListener = NULL;
	mMouseState = 0;
}


bool GameWindowSFML::create( char const* title , Vec2i const& size , int colorBit , bool bFullScreen )
{
	int style = sf::Style::Close;		
	mImpl.create(sf::VideoMode( size.x , size.y , colorBit), title, style);
	return mImpl.isOpen();
}

void GameWindowSFML::close()
{
	mImpl.close();
}



void GameWindowSFML::showCursor( bool bShow )
{
	mImpl.setMouseCursorVisible( bShow );
}


GameWindowSFML::~GameWindowSFML()
{
	close();
}


int convertSFKey( sf::Keyboard::Key key )
{
	if ( key == sf::Keyboard::Unknown )
		return -1;
	if ( key <= sf::Keyboard::Z )
		return EKeyCode::A + ( key - sf::Keyboard::A );
	if ( key <= sf::Keyboard::Num9 )
		return EKeyCode::Num0 + ( key - sf::Keyboard::Num0 );

	//FIXME
	switch( key )
	{
	case sf::Keyboard::Escape: return EKeyCode::Escape;
	case sf::Keyboard::Return: return EKeyCode::Return;
	case sf::Keyboard::BackSpace: return EKeyCode::Back;
	case sf::Keyboard::Left:  return EKeyCode::Left;
	case sf::Keyboard::Right: return EKeyCode::Right;
	case sf::Keyboard::Up:    return EKeyCode::Up;
	case sf::Keyboard::Down:  return EKeyCode::Down;
	case sf::Keyboard::Pause: return EKeyCode::Pause;
	}
	if ( key <  sf::Keyboard::Numpad0 )
		return -1;
	if ( key <= sf::Keyboard::Numpad9 )
		return EKeyCode::Numpad0 + ( key - sf::Keyboard::Numpad0 );
	if ( key <= sf::Keyboard::F15 )
		return EKeyCode::F1 + ( key - sf::Keyboard::F1 );

	return -1;
}


void GameWindowSFML::procSystemMessage()
{
	sf::Event event;
	while( mImpl.pollEvent( event ) )
	{
		bool needSend = true;
		switch( event.type )
		{
		case sf::Event::Closed:
			mListener->onSystemEvent( SYS_WINDOW_CLOSE );
			break;
		case sf::Event::TextEntered:
			{
				uint16 out[2];
				uint16* end = sf::Utf32::toUtf16( &event.text.unicode , &event.text.unicode + 1 , out);
				for( uint16* it = out ; it != end ; ++it )
				{
					uint16 c = *it;
					if ( c & 0xff00 )
						mListener->onChar( c >> 8 );
					mListener->onChar( c & 0xff );
				}
			}
			break;
		case sf::Event::KeyPressed:
		case sf::Event::KeyReleased:
			{
				unsigned key = convertSFKey( event.key.code );
				if ( key != -1 )
					mListener->onKey( KeyMsg( EKeyCode::Type( key ) , event.type == sf::Event::KeyPressed ) );
			}
			break;
		case sf::Event::MouseButtonReleased:
		case sf::Event::MouseButtonPressed:
			{
				unsigned msg = 0;

				switch( event.mouseButton.button )
				{
				case sf::Mouse::Button::Left:   msg |= MBS_LEFT; break;
				case sf::Mouse::Button::Right:  msg |= MBS_RIGHT; break;
				case sf::Mouse::Button::Middle: msg |= MBS_MIDDLE; break;
				}

				if ( event.type == sf::Event::MouseButtonPressed )
				{
					mMouseState |= msg;
					msg |= MBS_DOWN;
				}
				else
				{
					mMouseState &= ~msg;
				}

				mListener->onMouse( MouseMsg( event.mouseButton.x , event.mouseButton.y , msg , mMouseState ) );
			}
			break;
		case sf::Event::MouseMoved:
			{
				mListener->onMouse( MouseMsg( event.mouseMove.x , event.mouseMove.y , MBS_MOVING , mMouseState ) );
			}
			break;
		}
	}
}

void GLContextSFML::swapBuffers()
{
	mWindow->display();
}

bool GLContextSFML::setCurrent()
{
	return mWindow->setActive( true );
}

bool GLContextSFML::init( GameWindowSFML& window , GLConfig& config )
{
	mWindow = &window.mImpl;
	return true;
}

void GLContextSFML::release()
{
	delete this;
}

#endif //USE_SFML