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

//Check if given actor is in center of screen
ACharacter* activeCharacter = nullptr;
float deviationFromCenterMin = 10000;
ImVec4 IsActiveActor(AController* const localController, ACharacter* const actor, ImVec4 color) {

    float Screensize[2] = { 1920, 1080 };

    FVector origin, extent;
    actor->GetActorBounds(true, origin, extent);

    FVector2D center;
    if (!localController->ProjectWorldLocationToScreen({ origin.X, origin.Y, origin.Z + extent.Z }, center)) {
        return color;
    }
    
    //1 Abweichung zur Mitte berechnen
    float deviation[2] = { abs(center.X - (Screensize[0] / 2)), abs(center.Y - (Screensize[1] / 2)) };

    //Distance zur Mitte berechnen (Satz des Pythagoras)
    float distance = sqrt((deviation[0] * deviation[0]) + (deviation[1] * deviation[1]));

    if (activeCharacter != nullptr)
    {
        if (activeCharacter == actor)
        {
            deviationFromCenterMin = distance;

            color.x = 1.f;
            color.y = 0.f;
            color.z = 0.f;

            return color;
        }
        else
        {

            if (deviationFromCenterMin > distance)
            {
                activeCharacter = actor;
                deviationFromCenterMin = distance;

                color.x = 1.f;
                color.y = 0.f;
                color.z = 0.f;

                return color;
            }
            else
            {
                return color;
            }
        }

        //char name[0x64];
        //sprintf_s(name, "Distance: [%f]", distance);
        //Cheat::Renderer::Drawing::RenderText(name, center, colorText);
    }
    else
    {
        activeCharacter = actor;
    }

    return color;
}

//Get orientation of actor
std::string getOrientation(ACharacter* const actor) {

    auto rotation = actor->K2_GetActorRotation();
    const char* directions[] = { "N", "NE", "E", "SE", "S", "SW", "W", "NW" };
    int yaw = ((int)rotation.Yaw + 450) % 360;
    int index = int(yaw + 22.5f) % 360 * 0.0222222f;

    char name[0x40];
    sprintf_s(name, "[%d° (%s)]", yaw, const_cast<char*>(directions[index]));

    std::string returnString = name;
    return returnString;

}