#include "CrossHairClass.h"
#include <stdio.h>

const float AREA_NEAR = 0.033f;
const float AREA_FAR = 0.01f;
const float AREA_MID = 0.5f*(AREA_NEAR + AREA_FAR);

CrossHairClass::CrossHairClass():
   X(0.0f),
   YNear(0.0f),
   YFar(0.0f)
{
    //ctor
}

void CrossHairClass::Update_Calibration(float tx,float ty,float area)
{
    X = tx;

    // two point calibration.  We know that the area of the target when we are near the goal is 0.033
    // The area of the target when we're far from the goal is 0.01
    // We're going to maintain a line segment for the CrossHairY defined by two points:
    // (0.033,m_CrossHairYNear)  (0.01,m_CrossHairYFar)
    // When we get a calibration request, we decide which point we are closer to and update that
    // point, proportionally to the current area.

    // if we are closer to 'AREA_NEAR' we update YNear, otherwise update YFar
    if (area > AREA_MID)
    {
        //Updating YNear, assume YFar is fixed, compute YNear which would give us ty,area
        // (YFar - YNear*) / (AREA_FAR - AREA_NEAR) == (YFar - ty) / (AREA_FAR - area)
        float slope = (YFar - ty) / (AREA_FAR - area);
        YNear = -slope * (AREA_FAR - AREA_NEAR) + YFar;
    }
    else
    {
        //Updating YFar. assume YNear is fixed, compute YFar which would give us ty,area
        // (YFar* - YNear) / (AREA_FAR - AREA_NEAR) == (ty - YNear) / (area - AREA_NEAR)
        float slope = (ty - YNear) / (area - AREA_NEAR);
        YFar = slope * (AREA_FAR - AREA_NEAR) + YNear;
    }

    // double check
    float ycheck = Get_Y(area);
    printf("Calibrated!  YIn: %f  YCheck: %f\r\n",ty,ycheck);
}

float CrossHairClass::Get_Y(float area)
{
   // return the Y value proportional to the area given
   float fraction = (area - AREA_NEAR) / (AREA_FAR - AREA_NEAR);
   if(fraction < -0.25f)
   {
       fraction = -0.25f;
   }
   else if(fraction > 1.25f)
   {
       fraction = 1.25f;
   }
   return YNear + fraction * (YFar - YNear);
}
