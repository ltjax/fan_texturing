
#include "ShaderTestKernel.hpp"

#include <replay/bstream.hpp>
#include <replay/vector_math.hpp>
#include <replay/aabb.hpp>
#include <replay/pixbuf_io.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem.hpp>
#include <boost/assign.hpp>
#include <boost/bind.hpp>
#include "GLmm/GL.hpp"
#include "GLmm/Shader.hpp"
#include "GLSLUtils.hpp"


namespace {
	
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


CShaderTestKernel::CShaderTestKernel()
: Width(1),Height(1), RenderingEnabled(false)
{
	glsk::print( "Starting.." );

	using namespace boost::assign;
	std::vector<float> QuadElements;
	QuadElements += 0.f,0.f, 1.f,0.f, 0.f,1.f, 1.f,1.f;
	Quad.SetData( GL_ARRAY_BUFFER, QuadElements, GL_STATIC_DRAW );

	UpdateShader();

	FileMonitor.SignalChange().connect(
		boost::bind( &CShaderTestKernel::OnFileChange, this, _1 ) );

	FileMonitor.Start( boost::filesystem::current_path() );
	// Setup the noise permutation table
	{
		replay::shared_pixbuf Image = CreateSimplexNoiseTexture();
		NoiseTexture.SetImage( 0, GL_RGBA, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, Image->get_data() );
		NoiseTexture.SetFilter( GL_NEAREST, GL_NEAREST );
	}
}

void CShaderTestKernel::UpdateShader()
{
	GLmm::Program NewProg;
	try {
		CompileShader( "TestShader.glsl", NewProg );
		CompileShader( "Simplex.glsl", NewProg );
	}
	catch( std::runtime_error& x )
	{
		return;
	}

	try {
		NewProg.Link();
	}
	catch( GLmm::Program::LinkError& x )
	{
		return;
	}

	RenderingEnabled=true;
	NewProg.swap(this->Prog);
}

CShaderTestKernel::~CShaderTestKernel()
{
}

void CShaderTestKernel::OnFileChange( const std::vector<boost::filesystem::path>& Paths )
{
	if ( std::find( Paths.begin(), Paths.end(),
		boost::filesystem::initial_path()/"TestShader.glsl" ) != Paths.end() )
		UpdateShader();
}

void CShaderTestKernel::OnMouse( const glsk::mouse_event& Msg )
{
}

void CShaderTestKernel::OnInput( const glsk::input_event& Msg )
{
	// Ignore everything but KeyDown events
	if ( Msg.get_type() != glsk::key || !Msg.get_value() )
		return;
	
	// Quit the application?
	if ( Msg.get_identifier() == glsk::get_keycode("Escape") )
	{
		glsk::main::quit();
		return;
	}	


	if ( Msg.get_identifier() == glsk::get_keycode("c") )
	{
		UpdateShader();
		return;
	}
}

void CShaderTestKernel::OnResize( int w, int h )
{
	this->Width = w;
	this->Height = h;

	// Setup the viewport
	glViewport( 0, 0, w, h );
	glClearColor( 0.2f, 0.2f, 0.34f, 1.0f );
	glEnable(GL_DEPTH_TEST);
}

void CShaderTestKernel::OnIdle()
{
	FileMonitor.Update();
	// Clear the screen
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	if ( RenderingEnabled )
	{
		Prog.Use();
		glUniform1i( Prog.GetUniformLocation("PermTexture"), 0 );
		NoiseTexture.Bind();
		Quad.Bind( GL_ARRAY_BUFFER );
		glEnableClientState( GL_VERTEX_ARRAY );
		glVertexPointer( 2, GL_FLOAT, sizeof(float)*2, 0 );

		glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
		
		glDisableClientState( GL_VERTEX_ARRAY );
	}
}
