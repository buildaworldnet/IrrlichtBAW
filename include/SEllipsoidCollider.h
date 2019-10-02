#ifndef __S_ELLIPSOID_COLLIDER_H_INCLUDED__
#define __S_ELLIPSOID_COLLIDER_H_INCLUDED__

#include "vectorSIMD.h"
#include "irr/core/math/glslFunctions.tcc"

namespace irr
{
namespace core
{

class SEllipsoidCollider// : public AllocationOverrideDefault EBO inheritance problem
{
        vectorSIMDf negativeCenter;
        vectorSIMDf reciprocalAxes;
    public:
        SEllipsoidCollider(bool& validEllipse, const vectorSIMDf& centr, const vectorSIMDf& axisLengths) : negativeCenter(-centr)
        {
            for (size_t i=0; i<3; i++)
            {
                if (axisLengths.pointer[i]==0.f)
                {
                    validEllipse = false;
                    return;
                }
            }

            reciprocalAxes = reciprocal_approxim(axisLengths);

            validEllipse = true;
        }

        inline bool CollideWithRay(float& collisionDistance, vectorSIMDf origin, vectorSIMDf direction, const float& dirMaxMultiplier) const
        {
            origin += negativeCenter;
            origin *= reciprocalAxes;

            float originLen2 = dot(origin,origin).x;
            if (originLen2<=1.f) //point is inside
            {
                collisionDistance = 0.f;
                return true;
            }

            direction *= reciprocalAxes;
            vectorSIMDf dirLen2 = dot(direction,direction);
            float dirInvLen = core::inversesqrt(dirLen2).x;
            float dirDotOrigin = dot(direction,origin).x*dirInvLen;
            float partDet = originLen2-dirDotOrigin*dirDotOrigin;
            if (partDet>1.0)
                return false;

            float t = -dirDotOrigin-core::sqrt(1.f-partDet);
            if (t<0.f)
                return false;

            t *= dirInvLen;
            if (t<dirMaxMultiplier)
            {
                collisionDistance = t;
                return true;
            }
            else
                return false;
        }
};


}
}

#endif

