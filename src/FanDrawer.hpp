
#ifndef FAN_DRAWER_HPP
#define FAN_DRAWER_HPP

#include "Common.hpp"
#include "GLmm/BufferObject.hpp"

class CFantexMesh;

/** Draws fans in texture space.
*/
class CFanDrawer :
	public boost::noncopyable
{
public:
	CFanDrawer( CFantexMesh& Mesh );

	void							Draw( uint Index );

private:
	void							FillVertexBuffer( uint Index, GLubyte* Target );
	
	GLmm::BufferObject				Vbo;
	std::size_t						VboElementCount;

	CFantexMesh&					Mesh;
};


#endif // FAN_DRAWER_HPP
