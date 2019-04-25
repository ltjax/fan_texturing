
#ifndef USER_CAMERA_HPP
#define USER_CAMERA_HPP

#include "Common.hpp"

class CUserCamera
{
public:
						CUserCamera();
						~CUserCamera();

	const affinity&		GetAffinity() const;
	void				SetAffinity( const affinity& Rhs );
	void				OnMouseButton( int px, int py, int Index, int Value );
	void				OnMouseMove( int px, int py );
private:
	vector2i			LastMouse;
	int					MoveIndex;
	affinity			Affinity;
};

#endif
