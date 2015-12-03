#ifndef CFShader_h__
#define CFShader_h__

namespace CFly
{
	typedef IDirect3DVertexShader9 D3DVertexShader;
	typedef IDirect3DPixelShader9  D3DPixelShader;

	class Shader
	{
	public:
	};

	enum ShaderVersion
	{
		CF_VS_11 ,
		CF_VS_20 ,
		CF_VS_25 ,
		CF_VS_30 ,
		CF_VS_40 ,
	};

	char const* shaderVersion[] =  
	{
		"vs_1_1" ,
		"vs_2_0" ,
	};

	class VertexShader : public Shader
	{
	public:
		VertexShader( char const* fileName )
		{

		}

		bool createFromFile( D3DDevice* d3dDevice , char const* path , char const* entryFun , ShaderVersion version )
		{
			ID3DXBuffer* buffer;
			HRESULT hr = D3DXCompileShaderFromFile(
				path,            //filepath
				NULL,            //macro's
				NULL,            //includes
				entryFun ,       //main function
				shaderVersion[ version ] ,        //shader profile
				0,               //flags
				&buffer,         //compiled operations
				NULL,            //errors
				&mConstTable );  //constants

			if( hr != D3D_OK )
			{
				return false;
			}

			d3dDevice->CreateVertexShader( (DWORD*)buffer->GetBufferPointer() , &mD3DVertexShader );
			return true;

		}

	
		void setupDevice( D3DDevice* d3dDevice )
		{
			d3dDevice->SetVertexShader( mD3DVertexShader );
		}



	private:
		D3DVertexShader*    mD3DVertexShader;
		ID3DXConstantTable* mConstTable;
	};

	class PixelShader : public Shader
	{
	public:
		

	};


}//namespace CFly

#endif // CFShader_h__