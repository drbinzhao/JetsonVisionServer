#ifndef CROSSHAIRCLASS_H
#define CROSSHAIRCLASS_H


class CrossHairClass
{
    public:
        CrossHairClass();

        void Update_Calibration(float tx,float ty,float area);
        float Get_Y(float area);
        float Get_Average_Y() { return 0.5f*(YFar + YNear); }

    public:
        float X;
        float YNear;
        float YFar;

};

#endif // CROSSHAIRCLASS_H
