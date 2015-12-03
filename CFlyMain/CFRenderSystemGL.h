#ifndef CFRenderSystemGL_h__
#define CFRenderSystemGL_h__

namespace CFly
{
	class RenderSystem
	{
	public:

		void setAmbientColor( Color4f const& color )
		{
			glLightModelfv( GL_LIGHT_MODEL_AMBIENT , color );
		}
		void setLighting( bool bE ){ enableGL( GL_LIGHTING , bE ); }
		void setLight( int idx , bool bE ){ enableGL( GL_LIGHT0 + idx , bE ); }
		void setupLight( int idx , Light& light )
		{
			GLenum id = GL_LIGHT0 + idx;

			glLightfv( id , GL_AMBIENT , light.getAmbientColor() );
			glLightfv( id , GL_DIFFUSE , light.getDiffuseColor() );
			glLightfv( id , GL_SPECULAR, light.getSpecularColor() );

			float pos[4];
			switch( light.getLightType() )
			{
			case CFLT_SPOT:
				{
					glLightfv( id , GL_SPOT_DIRECTION, light.getLightDir() );
					//glLightf( id , GL_SPOT_CUTOFF , angle); // angle is 0 to 180
					//glLightf( id , GL_SPOT_EXPONENT, exp); // exponent is 0 to 128
				}
			case CFLT_POINT:
				{
					Vector3 const& lpos = light.getWorldPosition();
					pos[0] = lpos.x;
					pos[1] = lpos.y;
					pos[2] = lpos.z;
					pos[3] = 1;
				}
				break;
			case CFLT_PARALLEL:
				{
					Vector3 const& dir = light.getLightDir();
					pos[0] = dir.x;
					pos[1] = dir.y;
					pos[2] = dir.z;
					pos[3] = 0;
				}
				break;
			default:
			}
			glLightfv( id , GL_POSITION , pos );
			float c , l , q;
			light.getAttenuation( c , l , q );
			glLightf( id , GL_CONSTANT_ATTENUATION , c );
			glLightf( id , GL_LINEAR_ATTENUATION , l );
			glLightf( id , GL_QUADRATIC_ATTENUATION , q );
		}

		void setMaterialColor( Color4f const& amb , Color4f const& dif , Color4f const& spe , Color4f const& emi , float power )
		{
			glMaterialfv( GL_FRONT , GL_AMBIENT  , amb );
			glMaterialfv( GL_FRONT , GL_DIFFUSE  , dif );
			glMaterialfv( GL_FRONT , GL_SPECULAR , spe );
			glMaterialfv( GL_FRONT , GL_EMISSION , emi );
			glMaterialf ( GL_FRONT , GL_SHININESS , power );
		}

		void RenderSystem::setupTexture( int idxSampler , Texture* texture , int idxCoord )
		{
			GLenum texGL = convertType( texture->getType() );
			glBindTexture( texGL , idxSampler , texture->mIdHandle );
		}


		GLenum  convertType( TextureType type )
		{
			GLenum  texMap = { GL_TEXTURE_1D , GL_TEXTURE_2D , GL_TEXTURE_3D , GL_TEXTURE_CUBE_MAP };
			return texMap[ type ];
		}
		void setupTexAdressMode( TextureType texType , int idxSampler , TexAddressMode mode[] )
		{
			GLint modeGL[3] = { D3DTADDRESS_WRAP , D3DTADDRESS_WRAP , D3DTADDRESS_WRAP };
			for( int i = 0 ; i < 3 ; ++i )
			{
				switch( mode[i] )
				{
				case CFAM_WRAP   : modeGL[i] = GL_REPEAT; break;
				case CFAM_MIRROR : modeGL[i] = GL_MIRRORED_REPEAT; break;
				case CFAM_CLAMP  : modeGL[i] = GL_CLAMP_TO_EDGE; break;
				case CFAM_BORDER : modeGL[i] = GL_CLAMP_TO_BORDER; break;
				case CFAM_MIRROR_ONCE : modeGL[i] = GL_MIRROR_CLAMP_TO_EDGE; break;
				}
			}

			switch( texType )
			{
			case CFT_TEXTURE_1D:
				glTexParameteri( GL_TEXTURE_1D , GL_TEXTURE_WRAP_S ,  modeGL[0] );
				break;
			case CFT_TEXTURE_2D:
				glTexParameteri( GL_TEXTURE_2D , GL_TEXTURE_WRAP_S ,  modeGL[0] );
				glTexParameteri( GL_TEXTURE_2D , GL_TEXTURE_WRAP_T ,  modeGL[1] );
				break;
			case CFT_TEXTURE_3D:
				glTexParameteri( GL_TEXTURE_3D , GL_TEXTURE_WRAP_S ,  modeGL[0] );
				glTexParameteri( GL_TEXTURE_3D , GL_TEXTURE_WRAP_T ,  modeGL[1] );
				glTexParameteri( GL_TEXTURE_3D , GL_TEXTURE_WRAP_R ,  modeGL[2] );
				break;
			case CFT_TEXTURE_CUBE_MAP:
				glTexParameteri( GL_TEXTURE_CUBE_MAP , GL_TEXTURE_WRAP_S ,  modeGL[0] );
				glTexParameteri( GL_TEXTURE_CUBE_MAP , GL_TEXTURE_WRAP_T ,  modeGL[1] );
				glTexParameteri( GL_TEXTURE_CUBE_MAP , GL_TEXTURE_WRAP_R ,  modeGL[2] );
				break;
			}
		}

		void setFillMode( RenderMode mode )
		{
			GLenum fillmode = GL_FILL;
			switch ( mode )
			{
			case CFRM_POINT:     fillmode = GL_POINT; break;
			case CFRM_WIREFRAME: fillmode = GL_LINE; break;
			}
			glPolygonMode( GL_FRONT_AND_BACK , fillmode );
		}


	private:
		void enableGL( GLenum value , bool beE )
		{
			if ( beE )
				glEnable( value );
			else
				glDisable( value );
		}

	};

}//namespace CFly

#endif // CFRenderSystemGL_h__
