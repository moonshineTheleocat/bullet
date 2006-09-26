/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2006 Erwin Coumans  http://continuousphysics.com/Bullet/

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#ifndef CONE_MINKOWSKI_H
#define CONE_MINKOWSKI_H

#include "btConvexShape.h"
#include "BulletCollision/BroadphaseCollision/btBroadphaseProxy.h" // for the types

/// implements cone shape interface
class btConeShape : public btConvexShape

{

	float m_sinAngle;
	float m_radius;
	float m_height;
	
	btSimdVector3 ConeLocalSupport(const btSimdVector3& v) const;


public:
	btConeShape (SimdScalar radius,SimdScalar height);
	
	virtual btSimdVector3	LocalGetSupportingVertex(const btSimdVector3& vec) const;
	virtual btSimdVector3	LocalGetSupportingVertexWithoutMargin(const btSimdVector3& vec) const;
	virtual void	BatchedUnitVectorGetSupportingVertexWithoutMargin(const btSimdVector3* vectors,btSimdVector3* supportVerticesOut,int numVectors) const;

	float GetRadius() const { return m_radius;}
	float GetHeight() const { return m_height;}


	virtual void	CalculateLocalInertia(SimdScalar mass,btSimdVector3& inertia)
	{
		btSimdTransform identity;
		identity.setIdentity();
		btSimdVector3 aabbMin,aabbMax;
		GetAabb(identity,aabbMin,aabbMax);

		btSimdVector3 halfExtents = (aabbMax-aabbMin)*0.5f;

		float margin = GetMargin();

		SimdScalar lx=2.f*(halfExtents.x()+margin);
		SimdScalar ly=2.f*(halfExtents.y()+margin);
		SimdScalar lz=2.f*(halfExtents.z()+margin);
		const SimdScalar x2 = lx*lx;
		const SimdScalar y2 = ly*ly;
		const SimdScalar z2 = lz*lz;
		const SimdScalar scaledmass = mass * 0.08333333f;

		inertia = scaledmass * (btSimdVector3(y2+z2,x2+z2,x2+y2));

//		inertia.x() = scaledmass * (y2+z2);
//		inertia.y() = scaledmass * (x2+z2);
//		inertia.z() = scaledmass * (x2+y2);
	}



		virtual int	GetShapeType() const { return CONE_SHAPE_PROXYTYPE; }

		virtual char*	GetName()const 
		{
			return "Cone";
		}
};


#endif //CONE_MINKOWSKI_H

