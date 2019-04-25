
#ifndef TEXTURE_WRITER_HPP
#define TEXTURE_WRITER_HPP

#include "Common.hpp"
#include "FantexMesh.hpp"
#include <replay/buffer.hpp>
#include "FanDrawer.hpp"
#include "Misc.hpp"
#include <boost/threadpool.hpp>
#include "GLmm/Framebuffer.hpp"

/** Writes a texture using a texture generator.
*/
class CTextureWriter :
	public boost::noncopyable
{
public:
	CTextureWriter( CFantexMesh& Mesh,
		CAbstractTextureGenerator& Generator, uint TileSize );

	void operator()( const boost::filesystem::path& Filename );

private:
	replay::shared_pixbuf			GenerateNode( uint Level, uint Index, byte* Blocks,
		boost::threadpool::pool& Pool, GLmm::Framebuffer& Target );

	CFantexMesh&					Mesh;
	CAbstractTextureGenerator&		TextureGenerator;
	uint							TileSize;

	uint							ByteSize;
	uint							Alignment;

	replay::buffer<unsigned char>	CompressionBuffer;

	GLmm::Texture2D					FramebufferTexture;

	CFanDrawer						FanDrawer;

	int								CompressionFlags;

	double							RenderingTime;
	double							DownsamplingTime;
	double							CompressionTime;
};

#endif
