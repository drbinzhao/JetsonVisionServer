#include "CrossHairClass.h"
#include <stdio.h>

const float Y_NEAR = 0.1f;
const float Y_FAR = -0.7f;
const float Y_MID = 0.5f*(Y_NEAR + Y_FAR);


CrossHairClass::CrossHairClass():
   XNear(0.0f),
   XFar(0.0f)
{
    //ctor
}

void CrossHairClass::Update_Calibration(float tx,float ty)
{

    // two point calibration.  We know that the area of the target when we are near the goal is 0.033
    // The area of the target when we're far from the goal is 0.01
    // We're going to maintain a line segment for the CrossHairY defined by two points:
    // (0.033,m_CrossHairYNear)  (0.01,m_CrossHairYFar)
    // When we get a calibration request, we decide which point we are closer to and update that
    // point, proportionally to the current area.

    // if we are closer to 'Y_NEAR' we update XNear, otherwise update XFar
    if (ty > Y_MID)
    {
        //Updating XNear, assume XFar is fixed, compute XNear which would give us tx,ty
        float slope = (XFar - tx) / (Y_FAR - ty);
        XNear = -slope * (Y_FAR - Y_NEAR) + XFar;
    }
    else
    {
        //Updating XFar. assume XNear is fixed, compute XFar which would give us tx,ty
        float slope = (tx - XNear) / (ty - Y_NEAR);
        XFar = slope * (Y_FAR - Y_NEAR) + XNear;
    }

    // double check
    float xcheck = Get_X(ty);
    printf("Calibrated!  XIn: %f  XCheck: %f YIn: %f  \r\n",tx,xcheck,ty);
}

float CrossHairClass::Get_X(float y)
{
   float fraction = (y - Y_NEAR) / (Y_FAR - Y_NEAR);
   if(fraction < -0.25f)
   {
       fraction = -0.25f;
   }
   else if(fraction > 1.25f)
   {
       fraction = 1.25f;
   }
   return XNear + fraction * (XFar - XNear);
}

float CrossHairClass::Get_Y_Near()
{
   return Y_NEAR;
}

float CrossHairClass::Get_Y_Far()
{
   return Y_FAR;
}
