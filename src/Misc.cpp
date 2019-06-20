
#include "Misc.hpp"
#include <replay/vector_math.hpp>
#include <list>
#include <replay/minimal_sphere.hpp>
#include <replay/quaternion.hpp>


CSphere ApproximateBoundingBall( const std::vector<CSphere>& Input )
{
	typedef std::list<vec3> ContainerType;

	ContainerType Points;
	float MaxRadius = Input.front().Radius;
	Points.push_back( Input.front().Center );

	auto i=Input.begin();
	++i;

	for ( ; i!=Input.end(); ++i )
	{
		if ( i->Radius > MaxRadius )
			MaxRadius = i->Radius;

		Points.push_back( i->Center );
	}

	minimal_ball<float,vec3,3>	MiniBall(Points, 1e-15f);
	float ExtendedRadius = std::sqrt(MiniBall.square_radius())+MaxRadius;


	return CSphere( MiniBall.center(), ExtendedRadius );
}

void CDirectionalLight::Setup( vec3 Direction,
									const vec4& Ambient,
									const vec4& Diffuse )
{
	replay::normalize( Direction );

	this->Direction = vec4( Direction, 0.f );
	this->Ambient = Ambient;
	this->Diffuse = Diffuse;
		
	// this is positive because the light direction is directed towards the light
	const quaternion Rotation = shortest_arc(
		vec3( 0.f, 0.f, 1.f ), 
		Direction);

	this->ViewMatrix = matrix4( inverse( Rotation ) );
}

CDisplayData::CDisplayData( const vec3& Eyepoint, const replay::matrix4& Scene, int w, int h )
    : Matrix(Scene)
{
	this->WorldEyepoint=Eyepoint;
	replay::math::extract_frustum( Matrix, Frustum.data() );
	this->w = w;
	this->h = h;
}

inline void AlphaDownsampleRgbaPixel( const byte* a, const byte* b,
								  const byte* c, const byte* d,
								  byte* target )
{
	uint alpha = (a[3]+b[3]+c[3]+d[3]);
	uint inv_factor = alpha>0 ? (255*255*4)/alpha : 0;

	alpha = (alpha*inv_factor)/(4*255);
	target[3] = alpha;

	for ( uint i=0;i<3;++i )
	{
		uint component = (a[i]+b[i]+c[i]+d[i]);
		component = (component*inv_factor)/(4*255);
		target[i]=component;
	}
}

/** Shrink four images into one image of equal size, using box-filtering.
*/
void AlphaDownsampleRgba(
					const replay::pixbuf& a, const replay::pixbuf& b,
					const replay::pixbuf& c, const replay::pixbuf& d,
					replay::pixbuf& target )
{
	const uint w = target.get_width();
	const uint w_half = w/2;

	for ( uint y=0; y<w_half; ++y )
	for ( uint x=0; x<w_half; ++x )
	{
		uint x2=x*2;
		uint y2=y*2;

		AlphaDownsampleRgbaPixel( a.get_pixel(x2,y2), a.get_pixel(x2+1,y2),
			a.get_pixel(x2+1,y2+1), a.get_pixel(x2,y2+1), target.get_pixel(x,y) );

		AlphaDownsampleRgbaPixel( b.get_pixel(x2,y2), b.get_pixel(x2+1,y2),
			b.get_pixel(x2+1,y2+1), b.get_pixel(x2,y2+1), target.get_pixel(x+w_half,y) );

		AlphaDownsampleRgbaPixel( c.get_pixel(x2,y2), c.get_pixel(x2+1,y2),
			c.get_pixel(x2+1,y2+1), c.get_pixel(x2,y2+1), target.get_pixel(x,y+w_half) );

		AlphaDownsampleRgbaPixel( d.get_pixel(x2,y2), d.get_pixel(x2+1,y2),
			d.get_pixel(x2+1,y2+1), d.get_pixel(x2,y2+1), target.get_pixel(x+w_half,y+w_half) );

	}
}


void PreMultiply( const matrix2& Lhs, vec4& Rhs0, vec4& Rhs1 )
{
	vec4 x0( Lhs[0]*Rhs0[0]+Lhs[2]*Rhs1[0],
			Lhs[0]*Rhs0[1]+Lhs[2]*Rhs1[1],
			Lhs[0]*Rhs0[2]+Lhs[2]*Rhs1[2],
			Lhs[0]*Rhs0[3]+Lhs[2]*Rhs1[3] );

	vec4 x1( Lhs[1]*Rhs0[0]+Lhs[3]*Rhs1[0],
			Lhs[1]*Rhs0[1]+Lhs[3]*Rhs1[1],
			Lhs[1]*Rhs0[2]+Lhs[3]*Rhs1[2],
			Lhs[1]*Rhs0[3]+Lhs[3]*Rhs1[3] );

	Rhs0=x0;
	Rhs1=x1;
}


bool LeftTurn( vec2* Vertex, uint a, uint b, uint c )
{
	// Use determinant to classify a left-turn
	return replay::math::det( Vertex[b]-Vertex[a], Vertex[c]-Vertex[b] ) > 0.f;
}

/** An output sensitive convex hull algorithm.
	FIXME: Apparently bugged.
*/
std::size_t JarvisMarch( vec2* Vertex, std::size_t n )
{
	// No need to construct a hull
	if ( n < 3 )
		return n;

	// Find the starting point on the convex hull
	std::size_t Candidate = std::min_element( Vertex, Vertex+n, replay::array_less<2>() ) - Vertex;
	std::swap( Vertex[0], Vertex[Candidate] );

	// Find all other points
	for ( uint k=1; k<n; ++k )
	{
		Candidate=k;
		for ( uint i=k+1; i<n; ++i )
			if ( replay::math::det( Vertex[Candidate]-Vertex[k-1], Vertex[i]-Vertex[Candidate] ) < 0.f )
				Candidate=i;

		// Try to close the hull
		if ( k > 1 && replay::math::det( Vertex[Candidate]-Vertex[k-1], Vertex[0]-Vertex[Candidate] ) < 0.f )
			return k;

		std::swap( Vertex[k], Vertex[Candidate] );
	}

	return n;
}

/*void MelkmansAlgorithm( vec2* Vertex, uint n, boost::circular_buffer<uint>& Result )
{
	Result.resize(n+1);

	// assume counter clockwise winding
	Result.push_back(2);
	Result.push_back(0);
	Result.push_back(1);
	Result.push_back(2);

	for ( uint i=3; i<n; ++i )
	{
		// Vertex in the current convex hull?
		if ( LeftTurn(Vertex,*(Result.rbegin()+1),*Result.rbegin(),i) &&
			 LeftTurn(Vertex,i,*Result.begin(),*(Result.begin()+1)) )
			continue;

		// New vertex violates convexity, repair.
		// Assertion: the old cone-tip is removed on one of the sides,
		// but the new one is added on both -> *Result.begin() == *Result.rbegin()

		while ( !LeftTurn(Vertex,*(Result.rbegin()+1),*Result.rbegin(),i) )
			Result.pop_back();

		Result.push_back(i);

		while( !LeftTurn(Vertex,i,*Result.begin(),*(Result.begin()+1)) )
			Result.pop_front();

		Result.push_front(i);
	}

	// remove the double element
	Result.pop_front(); 

	// make sure the index buffer is ascending
	Result.rotate(std::min_element( Result.begin(), Result.end() ));
}*/

class RotatingCalipersBox
{
public:
	RotatingCalipersBox( const vec2* P, std::size_t n )
	{

		// Get an initial box and the extrema
		InitialExtrema();

		// Start with no rotation
		Current.u= vec2(1.f,0.f);
		
		// Check all possible configurations
		float	MinArea=ComputeSize();
		Best=Current;

		for ( std::size_t i=1; i<n && (Current.u[0] > 0.f); ++i )
		{
			RotateSmallestAngle();

			float Area = ComputeSize();

			// Did we find a better one?
			if ( Area < MinArea )
			{
				MinArea = Area;
				Best = Current;
			}
		}
	}


private:

	static vec2 RotatedLeft( vec2 x )
	{
		return vec2( -x[1], x[0] );
	}

	inline vec2 GetEdge( std::size_t i )
	{
		return P[(i+1)%n]-P[i];
	}

	inline void RotateSmallestAngle()
	{
		float ra,ta,la,ba;
		const float ZeroDeg = 1.f - 0.0001f;
		vec2 t = Current.u;
		while ( (ba=dot( t, normalized(GetEdge(Bottom)))) >= ZeroDeg )
			Bottom = (Bottom+1)%n;
		
		t = RotatedLeft( Current.u );
		while ( (ra=dot( t, normalized(GetEdge(Right)))) >= ZeroDeg )
			Right = (Right+1)%n;

		t = RotatedLeft( Current.u );
		while ( (ta=dot( t, normalized(GetEdge(Top)))) >= ZeroDeg)
			Top = (Top+1)%n;

		t = RotatedLeft( Current.u );
		while ( (la=dot( t, normalized(GetEdge(Left)))) >= ZeroDeg )
			Left = (Left+1)%n;

		uint s = 0;
		float m = ra;

		if ( ra > ba ) { s=1; m=ba; }
		if ( ta > m ) { s=2; m=ta; }
		if ( la > m ) { s=3; m=la; }

		switch( s )
		{
		case 0:
			t = normalized(GetEdge(Bottom));
			Current.u = t;
			break;
		case 1:
			
			t = normalized(GetEdge(Right));
			Current.u = vec2( -t[1],t[0] );
			break;
		case 2:
			t = normalized(GetEdge(Top));
			Current.u = -t;
			break;
		case 3:
			t = normalized(GetEdge(Left));
			Current.u = vec2( t[1], -t[0] );
			break;
		}
	}

	inline float ComputeSize()
	{
		const vec2& u(Current.u);
		// phi is defined by the matrix:
		// u[0] -u[1]
		// u[1]  u[0]

		const vec2& l(P[Left]);
		const vec2& b(P[Bottom]);

		Current.Min = vec2(
			l[0]*u[0]-l[1]*u[1], // x component of phi(P[Left])
			b[0]*u[1]+b[1]*u[0] // y component of phi(P[Bottom])
		);

		// Likewise for right and top
		const vec2& r(P[Right]);
		const vec2& t(P[Top]);

		Current.Max = vec2(
			r[0]*u[0]-r[1]*u[1],
			t[0]*u[1]+t[1]*u[0]
		);

		float dx = Current.Max[0]-Current.Min[0];
		float dy = Current.Max[1]-Current.Min[1];

		return dx*dy;
	}

	inline void InitialExtrema()
	{
		Left = Right = Top = Bottom = 0;

		// Find an initial bounding box and the extrema
		for ( std::size_t i=1; i<n; ++i )
		{
			float x = P[i][0];
			float y = P[i][1];

			// Check for a new x
			if ( x < P[Left][0] )
				Left = i;
			else if ( x > P[Right][0] )
				Right = i;
			else
			{
				if ( x == P[Left][0] && y < P[Left][1] )
					Left = i;

				else if ( x == P[Right][0] && y > P[Right][1] )
					Right = 1;
			}

			// Check for new y
			if ( y < P[Bottom][1] )
				Bottom = i;
			else if ( y > P[Top][1] )
				Top = i;
			else
			{
				if ( y == P[Bottom][1] && x > P[Bottom][0] )
					Bottom = i;

				else if ( y == P[Top][1] && x < P[Top][0] )
					Top = i;
			}
		}
	}

	// The convex hull
	const vec2*		P;
	std::size_t		n;

	struct BoxType {
		// box
		vec2			Min;
		vec2			Max;

		// rotation
		vec2			u; 
	};

	BoxType			Current;
	BoxType			Best;

	// Current extrema
	std::size_t Top;
	std::size_t Bottom;
	std::size_t Left;
	std::size_t Right;

	// Selected edge
	std::size_t SelectedEdge;
};


float BoundingRectangleUnderTransformation( const vec2* Points, uint n, const replay::matrix2& M,
										  vec2& Min, vec2& Max )
{
	Min=M*Points[0];
	Max=Min;

	for ( uint i=1; i<n; ++i )
	{
		vec2 p=M*Points[i];
		float x=p[0];
		float y=p[1];

		if ( x < Min[0] )
			Min[0]=x;
		else if ( x > Max[0] )
			Max[0]=x;

		if ( y < Min[1] )
			Min[1]=y;
		else if ( y > Max[1] )
			Max[1]=y;	
	}

	return (Max[0]-Min[0])*(Max[1]-Min[1]);	
}

void MinimalAreaBoundingRectangle( const vec2* ConvexHull, uint n, matrix2& A,
								   vec2& MinOut, vec2& MaxOut )
{
	// simple quadratic algorithm
	// FIXME: use rotating calipers

	float MinimalArea=std::numeric_limits<float>::max();
	vec2 BestEdge;

	for ( uint i=0; i<n; ++i  )
	{
		vec2 Edge=ConvexHull[(i+1)%n]-ConvexHull[i];

		float Length = magnitude(Edge);
		if ( replay::math::fuzzy_zero(Length) )
			continue;

		Edge /= Length;
		
		// make sure the direction is in the first two quadrants
		if ( Edge[1] < 0.f )
			Edge.negate();

		// if the direction is in the first quadrant, use as x, otherwise, use as y
		matrix2 M;
		if ( Edge[0] > 0.f )
		{
			M.set(  Edge[0], Edge[1],
				   -Edge[1], Edge[0] );
		}
		else
		{
			M.set(  Edge[1],-Edge[0],
				    Edge[0], Edge[1] );
		}

		vec2 Min,Max;
		float Area = BoundingRectangleUnderTransformation( ConvexHull, n, M, Min, Max );

		if ( Area < MinimalArea )
		{
			MinOut=Min;
			MaxOut=Max;
			A=M;
			MinimalArea=Area;
		}	
	}
}

bool IsCCW( const vec2* Hull, std::size_t n )
{
	vec2 Last=Hull[0]-Hull[n-1];

	for ( std::size_t i=0; i<n; ++i )
	{
		const vec2 Delta=Hull[(i+1)%n]-Hull[i];

		if ( replay::math::det( Last, Delta ) >= 0.f )
			return false;

		Last = Delta;
	}

	return true;
}