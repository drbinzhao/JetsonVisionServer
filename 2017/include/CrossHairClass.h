#ifndef CROSSHAIRCLASS_H
#define CROSSHAIRCLASS_H

enum CameraMode
{
    CAM_FRONT = 0,
    CAM_BACK
};

class CrossHairClass
{
    public:
        CrossHairClass();

        void Update_Calibration(float tx,float ty);
        float Get_X(float y);
        float Get_Average_X() { return 0.5f*(XFar + XNear); }

        float Get_Y_Near();
        float Get_Y_Far();

    public:
        float XNear;
        float XFar;

};

#endif // CROSSHAIRCLASS_H
