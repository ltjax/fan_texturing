
#include "TextureGenerator.hpp"
#include "GLSLUtils.hpp"
#include <boost/foreach.hpp>
#include <replay/pixbuf_io.hpp>

namespace {

void LoadTexture( GLmm::Texture2D& Target, const boost::filesystem::path& Filename, GLuint WrapMode )
{	
	replay::shared_pixbuf Image = replay::pixbuf_io::load_from_file( Filename );
	Target.SetImage( 0, GL_RGBA, Image->get_width(), Image->get_height(),
		Image->get_channels()==4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, Image->get_data() );
	Target.SetFilter( GL_LINEAR, GL_LINEAR );
	Target.SetWrap( WrapMode, WrapMode );
}
	
int perm[256]= {151,160,137,91,90,15,
  131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
  190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
  88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
  77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
  102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
  135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
  5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
  223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
  129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
  251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
  49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
  138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180};

int grad3[16][3] = {{0,1,1},{0,1,-1},{0,-1,1},{0,-1,-1},
                   {1,0,1},{1,0,-1},{-1,0,1},{-1,0,-1},
                   {1,1,0},{1,-1,0},{-1,1,0},{-1,-1,0}, // 12 cube edges
                   {1,0,-1},{-1,0,-1},{0,-1,1},{0,1,1}}; // 4 more to make 16

replay::shared_pixbuf CreateSimplexNoiseTexture()
{
	// Create a 256x256 RGBA texture
	replay::shared_pixbuf Result = replay::pixbuf::create(256,256, replay::pixbuf::rgba );

	for( int i = 0; i<256; i++)
    for( int j = 0; j<256; j++)
	{
		char value = perm[(j+perm[i]) & 0xFF];
		Result->set_pixel(i,j,
			grad3[value & 0x0F][0] * 64 + 64,
			grad3[value & 0x0F][1] * 64 + 64,
			grad3[value & 0x0F][2] * 64 + 64, value );
    }

	return Result;
}

}

CTextureGenerator::CTextureGenerator( const CFantexMesh& Mesh, const CDirectionalLight& Light )
: ShadowmapRenderer( Mesh, Light, 512 )
{
	SetupTextures();
	SetupGLSLShader();
}

CTextureGenerator::~CTextureGenerator()
{
}

void CTextureGenerator::SetupTextures()
{
	// Setup the noise permutation table
	{
		replay::shared_pixbuf Image = CreateSimplexNoiseTexture();
		SimplexNoiseTexture.SetImage( 0, GL_RGBA, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, Image->get_data() );
		SimplexNoiseTexture.SetFilter( GL_NEAREST, GL_NEAREST );
	}

	LoadTexture( RockTexture, "rock2.png", GL_CLAMP );
	LoadTexture( GrassTexture, "Grass_17-2.png", GL_REPEAT );
	LoadTexture( SandTexture, "sand2.png", GL_REPEAT );
}

void CTextureGenerator::SetupGLSLShader()
{
	typedef boost::tuple<uint,std::string> CShaderSection;

	std::vector<CShaderSection> ShaderSections;

	// Load shader source code.
	LoadComboShaderSource( "Generate.glsl", ShaderSections );
	LoadComboShaderSource( "Simplex.glsl", ShaderSections );


	// Compile all the translation units
	try {

		BOOST_FOREACH( const CShaderSection& x, ShaderSections )
		{
			GLmm::Shader Temp( x.get<0>() );
			Temp.SetSource( x.get<1>() );
			Temp.Compile();
			GenerateProgram.Attach( Temp );
		}

	}
	catch ( GLmm::Shader::CompileError& x )
	{
		throw std::runtime_error( std::string("Compile error: ") + x.what() );
	}

	// Link the program
	try {
		GenerateProgram.Link();
	}
	catch( GLmm::Program::LinkError& x )
	{
		throw std::runtime_error( std::string( "Link error: " ) + x.what() );
	}
}

void CTextureGenerator::Bind( const vec3& Center, const CSphere& Bounds )
{
	ShadowmapRenderer.Render( Bounds );

	GenerateProgram.Use();

	glUniform4fv( GenerateProgram.GetUniformLocation("Center"), 1, vec4(Center,0.f).ptr() );

	glUniform1i( GenerateProgram.GetUniformLocation("PermTexture"), 0 );
	glActiveTexture( GL_TEXTURE0 );
	SimplexNoiseTexture.Bind();

	glUniform1i( GenerateProgram.GetUniformLocation("RockTexture"), 1 );
	glActiveTexture( GL_TEXTURE1 );
	RockTexture.Bind();

	glUniform1i( GenerateProgram.GetUniformLocation("GrassTexture"), 2 );
	glActiveTexture( GL_TEXTURE2 );
	GrassTexture.Bind();

	glUniform1i( GenerateProgram.GetUniformLocation("SandTexture"), 3 );
	glActiveTexture( GL_TEXTURE3 );
	SandTexture.Bind();

	glUniform1i( GenerateProgram.GetUniformLocation("ShadowTexture"), 4 );
	glActiveTexture( GL_TEXTURE4 );
	ShadowmapRenderer.GetShadowTexture().Bind();

	glUniformMatrix4fv( GenerateProgram.GetUniformLocation("ShadowMatrix"),
		1,GL_FALSE,ShadowmapRenderer.GetShadowMatrix().ptr() );
}