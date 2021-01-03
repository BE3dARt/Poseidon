#pragma once
#include <math.h> 

#define PI 3.141592653589f

float distanceToZDelta(int dist) {
	float angle = ((asin(((float)dist * 9.81f) / (60.5f * 60.5f))) * (float)180.f / PI) / 2;
	float deltaZ = (float)dist * (sin((angle * 3.141592653589f / (float)180.f))) * 100;
	return deltaZ;
}

bool DisplayAimHelper(AController * const localController, ACharacter* const actor, const FVector cameraLoc, ImVec4 colorVis, ImVec4 colorInvis, ImVec4 colorText, const FVector localLoc, Cheat::Config::EBox boxType, bool on) {

    FVector origin, extent;
    actor->GetActorBounds(true, origin, extent);

    FVector2D headPos;
    if (!localController->ProjectWorldLocationToScreen({ origin.X, origin.Y, origin.Z + extent.Z }, headPos)) {
        return false;
    }

    FVector2D footPos;
    if (!localController->ProjectWorldLocationToScreen({ origin.X, origin.Y, origin.Z - extent.Z }, footPos)) {
        return false;
    }

    float height = abs(footPos.Y - headPos.Y);
    float width = height * 0.6f;

    bool bVisible = localController->LineOfSightTo(actor, cameraLoc, false);
    ImVec4 col = bVisible ? colorVis : colorInvis;

    auto displayName = reinterpret_cast<AFauna*>(actor)->DisplayName;

    const int dist = localLoc.DistTo(origin) * 0.01f;
    char name[0x64];
    const int len = displayName->multi(name, 0x50);
    snprintf(name + len, sizeof(name) - len, " [%d]", dist);
    const float adjust = height * 0.05f;
    FVector2D pos = { headPos.X, headPos.Y - adjust };
    Cheat::Renderer::Drawing::RenderText(name, pos, colorText);

    if (on)
    {
        float deltaZ = distanceToZDelta(dist);

        if (!localController->ProjectWorldLocationToScreen({ origin.X, origin.Y, ((origin.Z + extent.Z) + deltaZ) }, headPos)) {
            return false;
        }

        if (!localController->ProjectWorldLocationToScreen({ origin.X, origin.Y, ((origin.Z + extent.Z) + deltaZ) }, footPos)) {
            return false;
        }
    }


    switch (boxType)
    {
    case Cheat::Config::EBox::E2DBoxes:
    {
        Cheat::Renderer::Drawing::Render2DBox(headPos, footPos, height, width, col);
        break;
    }
    case Cheat::Config::EBox::E3DBoxes:
    {
        FRotator rotation = actor->K2_GetActorRotation();
        FVector ext = { 40.f, 40.f, extent.Z };
        if (!Cheat::Renderer::Drawing::Render3DBox(localController, origin, ext, rotation, col)) {
            return false;
        }
        break;
    }
    }

}

ACharacter* test = nullptr;
float deviationFromCenterMin = 10000;

//Check if given actor is in center of screen
bool IsInFrontofMe(AController* const localController, ACharacter* const actor, ImVec4 colorText) {

    float Screensize[2] = { 1920, 1080 };

    FVector origin, extent;
    actor->GetActorBounds(true, origin, extent);

    FVector2D center;
    if (!localController->ProjectWorldLocationToScreen({ origin.X, origin.Y, origin.Z + extent.Z }, center)) {
        return false;
    }
    
    /*
    //Make active when in center of screen (Multiple Targets possible)
    if (center.X >= (Screensize[0] / 6) * 2 && center.X <= (Screensize[0] / 6) * 4 && center.Y >= (Screensize[1] / 4) * 1 && center.Y <= (Screensize[1] / 4) * 3)
    {
        char name[0x64];
        sprintf_s(name, "Center: [%f], [%f]", center.X, center.Y);
        Cheat::Renderer::Drawing::RenderText(name, center, colorText);
    }
    */

    //1 Abweichung zur Mitte berechnen
    float deviation[2] = { abs(center.X - (Screensize[0] / 2)), abs(center.Y - (Screensize[1] / 2)) };

    //Distance zur Mitte berechnen (Satz des Pythagoras)
    float distance = sqrt((deviation[0] * deviation[0]) + (deviation[1] * deviation[1]));

    if (test != nullptr)
    {
        if (test == actor)
        {
            deviationFromCenterMin = distance;
        }
        else
        {

            if (deviationFromCenterMin > distance)
            {
                test = actor;
                deviationFromCenterMin = distance;
            }
            else
            {
                return false;
            }
        }

        char name[0x64];
        sprintf_s(name, "Distance: [%f]", distance);
        Cheat::Renderer::Drawing::RenderText(name, center, colorText);
    }
    else
    {
        test = actor;
    }



    //Distinguish between actors
}