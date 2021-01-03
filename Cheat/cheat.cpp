#include "cheat.h"
#include "Renderer.h"
#include "Logger.h"
#include "Tools.h"
#include "AimHelp.h"
#include <filesystem>
#include <imgui/imgui_impl_dx11.h>
#include <imgui/imgui_internal.h>
#include <imgui/imgui_impl_win32.h>
#include <HookLib/HookLib.h>

void Cheat::Hacks::OnWeaponFiredHook(UINT64 arg1, UINT64 arg2)
{
    Logger::Log("arg1: %p, arg2: %p\n", arg1, arg2);
    auto& cameraCache = cache.localCamera->CameraCache.POV;
    auto prev = cameraCache.Rotation;
    cameraCache.Rotation = { -cameraCache.Rotation.Pitch, -cameraCache.Rotation.Yaw, 0.f };
    return OnWeaponFiredOriginal(arg1, arg2);
}

void Cheat::Hacks::Init()
{
    /*UFunction* fn = UObject::FindObject<UFunction>("Function Athena.ProjectileWeapon.OnWeaponFired");
    if (fn) {
        if (SetHook(fn->Func, OnWeaponFiredHook, reinterpret_cast<void**>(&OnWeaponFiredOriginal)))
        {
            Logger::Log("StartFire: %p\n", fn->Func);
        }
    }*/
}

inline void Cheat::Hacks::Remove()
{
    //RemoveHook(orig);
}

HRESULT Cheat::Renderer::PresentHook(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags)
{ 
    int distanceArray[64];

    if (!device)
    {
        ID3D11Texture2D* surface = nullptr;
        goto init;
    cleanup:
        Cheat::Remove();
        if (surface) surface->Release();
        return fnPresent(swapChain, syncInterval, flags);
    init:

        if (FAILED(swapChain->GetBuffer(0, __uuidof(surface), reinterpret_cast<PVOID*>(&surface))))  { goto cleanup; };
       Logger::Log("ID3D11Texture2D* surface = %p\n", surface); 

        if (FAILED(swapChain->GetDevice(__uuidof(device), reinterpret_cast<PVOID*>(&device)))) goto cleanup;

        Logger::Log("ID3D11Device* device = %p\n", device);

        if (FAILED(device->CreateRenderTargetView(surface, nullptr, &renderTargetView))) goto cleanup;

       Logger::Log("ID3D11RenderTargetView* renderTargetView = %p\n", renderTargetView);

        surface->Release();
        surface = nullptr;

        device->GetImmediateContext(&context);

        Logger::Log("ID3D11DeviceContext* context = %p\n", context);

        ImGui::CreateContext();

        {
            ImGuiIO& io = ImGui::GetIO();
            ImFontConfig config;
            config.GlyphRanges = io.Fonts->GetGlyphRangesCyrillic();
            config.RasterizerMultiply = 1.125f;
            io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Arial.ttf", 16.0f, &config);
            io.IniFilename = nullptr;
        }

        DXGI_SWAP_CHAIN_DESC desc;
        swapChain->GetDesc(&desc);
        auto& window = desc.OutputWindow;
        gameWindow = window;

        Logger::Log("gameWindow = %p\n", window);

        if (!ImGui_ImplWin32_Init(window)) goto cleanup;
        if (!ImGui_ImplDX11_Init(device, context)) goto cleanup;
        if (!ImGui_ImplDX11_CreateDeviceObjects()) goto cleanup;
        Logger::Log("ImGui initialized successfully!\n");

        HookInput();
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    
    ImGui::Begin("#1", nullptr, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar);
    auto& io = ImGui::GetIO();
    ImGui::SetWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y), ImGuiCond_Always);
   
    auto drawList = ImGui::GetCurrentWindow()->DrawList; 
    
    try 
    {
        do 
        {
            memset(&cache, 0, sizeof(Cache));
            auto const world = *UWorld::GWorld;
           
            if (!world) break;
            auto const game = world->GameInstance;
            if (!game) break;
            auto const gameState = world->GameState;
            if (!gameState) break;
            cache.gameState = gameState;
            if (!game->LocalPlayers.Data) break;
            
            auto const localPlayer = game->LocalPlayers[0];
            if (!localPlayer) break;
            auto const localController = localPlayer->PlayerController;
            if (!localController) break;
            cache.localController = localController;
            auto const camera = localController->PlayerCameraManager;
            if (!camera) break;
            cache.localCamera = camera;
            const auto cameraLoc = camera->GetCameraLocation();
            const auto cameraRot = camera->GetCameraRotation();
            auto const localCharacter = localController->Character;
            if (!localCharacter) break;
            const auto levels = world->Levels;
            if (!levels.Data) break;
            const auto localLoc = localCharacter->K2_GetActorLocation();
           
            bool isWieldedWeapon = false;
            auto item = localCharacter->GetWieldedItem();
            if (item) isWieldedWeapon = item->isWeapon();

           // check isWieldedWeapon before accessing!
           auto const localWeapon = *reinterpret_cast<AProjectileWeapon**>(&item);
           ACharacter* attachObject = localCharacter->GetAttachParentActor();

           bool isHarpoon = false;
           if (attachObject)
           {
               if (cfg.aim.harpoon.bEnable && attachObject->isHarpoon()) { isHarpoon = true; }
           }

           cache.good = true;

            static struct {
                ACharacter* target = nullptr;
                FVector location;
                FRotator delta;
                float best = FLT_MAX;
                float smoothness = 1.f;
            } aimBest;

            aimBest.target = nullptr;
            aimBest.best = FLT_MAX;

            //Loop though all the objects IN VIEWPORT
            for (auto l = 0u; l < levels.Count; l++)
            {
                auto const level = levels[l];
                if (!level) continue;
                const auto actors = level->AActors;
                if (!actors.Data) continue;

                // todo: make functions for similar code 
                for (auto a = 0u; a < actors.Count; a++)
                {
                    auto const actor = actors[a];
                    if (!actor) continue;
                    if (cfg.aim.bEnable)
                    {
                        //Aimbot for Harpoon
                        if (isHarpoon)
                        {
                            if (actor->isItem())
                            {
                                do {
                                    
                                    //Get Distance to Target
                                    FVector location = actor->K2_GetActorLocation();
                                    float dist = cameraLoc.DistTo(location);
                                    if (dist > 7500.f || dist < 260.f) { break; }

                                    //Only Visible
                                    if (cfg.aim.harpoon.bVisibleOnly) if (!localController->LineOfSightTo(actor, cameraLoc, false)) { break; }

                                    auto harpoon = reinterpret_cast<AHarpoonLauncher*>(attachObject);
                                    auto center = UKismetMathLibrary::NormalizedDeltaRotator(cameraRot, harpoon->rotation);

                                    FRotator delta = UKismetMathLibrary::NormalizedDeltaRotator(UKismetMathLibrary::FindLookAtRotation(cameraLoc, location), center);

                                    if (delta.Pitch < -35.f || delta.Pitch > 67.f || abs(delta.Yaw) > 50.f) { break; }
                                    FRotator diff = delta - harpoon->rotation;
                                    float absPitch = abs(diff.Pitch);   
                                    float absYaw = abs(diff.Yaw);
                                    if (absPitch > cfg.aim.harpoon.fPitch || absYaw > cfg.aim.harpoon.fYaw) { break; }
                                    float sum = absYaw + absPitch;
                                    if (sum < aimBest.best)
                                    {
                                        aimBest.target = actor;
                                        aimBest.location = location;
                                        aimBest.delta = delta;
                                        aimBest.best = sum;
                                    }

                                } while (false);
                            }
                        }
                    }
                    
                    if (cfg.visuals.client.bDebug)
                    {
                        const FVector location = actor->K2_GetActorLocation();
                        const float dist = localLoc.DistTo(location) * 0.01f;
                        if (dist < cfg.visuals.client.fDebug)
                        {
                            auto const actorClass = actor->Class;
                            if (!actorClass) continue;
                            auto super = actorClass->SuperField;
                            if (!super) continue;
                            FVector2D screen;
                            if (localController->ProjectWorldLocationToScreen(location, screen))
                            {
                                auto superName = super->GetNameFast();
                                auto className = actorClass->GetNameFast();
                                if (superName && className)
                                {
                                    char buf[0x128];
                                    sprintf_s(buf, "%s %s [%d] (%p)", className, superName, (int)dist, actor);
                                    Drawing::RenderText(buf, screen, ImVec4(1.f, 1.f, 1.f, 1.f));
                                }
                            }
                        }
                    }
                    else {

                        //Items
                        if (cfg.visuals.items.bEnable && actor->isItem()) {

                            auto location = actor->K2_GetActorLocation();
                            FVector2D screen;
                            if (localController->ProjectWorldLocationToScreen(location, screen))
                            {
                                auto const desc = actor->GetItemInfo()->Desc;
                                if (!desc) continue;
                                const int dist = localLoc.DistTo(location) * 0.01f;
                                distanceArray[0] = dist;
                                char name[0x64];
                                const int len = desc->Title->multi(name, 0x50);
                                snprintf(name + len, sizeof(name) - len, " [%d]", dist);
                                Drawing::RenderText(name, screen, cfg.visuals.items.textCol);
                            };

                            continue;
                        }

                        //Barrels
                        if (cfg.visuals.barrels.bEnable && actor->isBarrel()) {

                            auto location = actor->K2_GetActorLocation();
                            FVector2D screen;
                            if (localController->ProjectWorldLocationToScreen(location, screen))
                            {
                                const int dist = localLoc.DistTo(location) * 0.01f;
                                char name[0x64];
                                sprintf_s(name, "Barrel [%d]", dist);

                                Drawing::RenderText(name, screen, cfg.visuals.items.textCol);
                            };

                            IsInFrontofMe(localController, actor, cfg.visuals.items.textCol);

                            continue;
                        }

                        //Shipwrecks
                        else if (cfg.visuals.shipwrecks.bEnable && actor->isShipwreck())
                        {
                            auto location = actor->K2_GetActorLocation();
                            FVector2D screen;
                            if (localController->ProjectWorldLocationToScreen(location, screen))
                            {
                                const int dist = localLoc.DistTo(location) * 0.01f;
                                char name[0x64];
                                sprintf_s(name, "Shipwreck [%d]", dist);
                                Drawing::RenderText(name, screen, cfg.visuals.items.textCol);
                            };
                            continue;
                        }

                        //Other Players
                        else if ((cfg.visuals.players.bName || cfg.visuals.players.bSkeleton || cfg.visuals.players.bDrawTeam) && actor->isPlayer() && actor != localCharacter && !actor->IsDead())
                        {

                            const bool teammate = UCrewFunctions::AreCharactersInSameCrew(actor, localCharacter);
                            if (teammate && !cfg.visuals.players.bDrawTeam) continue;

                            FVector origin, extent;
                            actor->GetActorBounds(true, origin, extent);
                            const FVector location = actor->K2_GetActorLocation();

                            FVector2D headPos;
                            if (!localController->ProjectWorldLocationToScreen({ location.X, location.Y, location.Z + extent.Z }, headPos)) continue;
                            FVector2D footPos;
                            if (!localController->ProjectWorldLocationToScreen({ location.X, location.Y, location.Z - extent.Z }, footPos)) continue;

                            const float height = abs(footPos.Y - headPos.Y);
                            const float width = height * 0.4f;

                            const bool bVisible = localController->LineOfSightTo(actor, cameraLoc, false);
                            ImVec4 col;
                            if (teammate) col = bVisible ? cfg.visuals.players.teamColorVis : cfg.visuals.players.teamColorInv;
                            else  col = bVisible ? cfg.visuals.players.enemyColorVis : cfg.visuals.players.enemyColorInv;

                            switch (cfg.visuals.players.boxType)
                            {
                                case Config::EBox::E2DBoxes:
                                {
                                    Drawing::Render2DBox(headPos, footPos, height, width, col);
                                    break;
                                }
                                case Config::EBox::E3DBoxes:
                                {
                                    FRotator rotation = actor->K2_GetActorRotation();
                                    FVector ext = { 35.f, 35.f, extent.Z };
                                    if (!Drawing::Render3DBox(localController, location, ext, rotation, col)) continue;
                                    break;
                                }
                                /*
                                case Config::EBox::EDebugBoxes:
                                {
                                    FVector ext = { 35.f, 35.f, extent.Z };
                                    UKismetMathLibrary::DrawDebugBox(actor, location, ext, *reinterpret_cast<FLinearColor*>(&col), actor->K2_GetActorRotation(), 0.0f);
                                    break;
                                }
                                */
                            }


                            if (cfg.visuals.players.bName)
                            {

                                auto const playerState = actor->PlayerState;
                                if (!playerState) continue;

                                const auto playerName = playerState->PlayerName;
                                if (!playerName.Data) continue;

                                char name[0x30];
                                const int len = playerName.multi(name, 0x20);
                                const int dist = localLoc.DistTo(origin) * 0.01f;

                                snprintf(name + len, sizeof(name) - len, " [%d]", dist);

                                const float adjust = height * 0.05f;
                                FVector2D pos = { headPos.X, headPos.Y - adjust };
                                Drawing::RenderText(name, pos, cfg.visuals.players.textCol);
                            }

                            if (cfg.visuals.players.barType != Config::EBar::ENone)
                            {
                                auto const healthComp = actor->HealthComponent;
                                if (!healthComp) continue;
                                const float hp = healthComp->GetCurrentHealth() / healthComp->GetMaxHealth();
                                const float width2 = width * 0.5f;
                                const float adjust = height * 0.025f;
                                switch (cfg.visuals.players.barType)
                                {
                                    case Config::EBar::ELeft:
                                    {
                                        const float len = height * hp;
                                        drawList->AddRectFilled({ headPos.X - width2 - adjust * 2.f, headPos.Y }, { headPos.X - width2 - adjust, footPos.Y - len }, ImGui::GetColorU32(IM_COL32(255, 0, 0, 255)));
                                        drawList->AddRectFilled({ headPos.X - width2 - adjust * 2.f, footPos.Y - len }, { headPos.X - width2 - adjust, footPos.Y }, ImGui::GetColorU32(IM_COL32(0, 255, 0, 255)));
                                        break;
                                    }
                                    case Config::EBar::ERight:
                                    {
                                        const float len = height * hp;
                                        drawList->AddRectFilled({ headPos.X + width2 + adjust, headPos.Y }, { headPos.X + width2 + adjust * 2.f, footPos.Y - len }, ImGui::GetColorU32(IM_COL32(255, 0, 0, 255)));
                                        drawList->AddRectFilled({ headPos.X + width2 + adjust, footPos.Y - len }, { headPos.X + width2 + adjust * 2.f, footPos.Y }, ImGui::GetColorU32(IM_COL32(0, 255, 0, 255)));
                                        break;
                                    }
                                    case Config::EBar::EBottom:
                                    {
                                        const float len = width * hp;
                                        drawList->AddRectFilled({ headPos.X - width2, footPos.Y + adjust }, { headPos.X - width2 + len, footPos.Y + adjust * 2.f }, ImGui::GetColorU32(IM_COL32(0, 255, 0, 255)));
                                        drawList->AddRectFilled({ headPos.X - width2 + len, footPos.Y + adjust }, { headPos.X + width2, footPos.Y + adjust * 2.f }, ImGui::GetColorU32(IM_COL32(255, 0, 0, 255)));
                                        break;
                                    }
                                    case Config::EBar::ETop:
                                    {
                                        const float len = width * hp;
                                        drawList->AddRectFilled({ headPos.X - width2, headPos.Y - adjust * 2.f }, { headPos.X - width2 + len, headPos.Y - adjust }, ImGui::GetColorU32(IM_COL32(0, 255, 0, 255)));
                                        drawList->AddRectFilled({ headPos.X - width2 + len, headPos.Y - adjust * 2.f }, { headPos.X + width2, headPos.Y - adjust }, ImGui::GetColorU32(IM_COL32(255, 0, 0, 255)));
                                        break;
                                    }
                                }

                            }

                            if (cfg.visuals.players.bSkeleton)
                            {
                                auto const mesh = actor->Mesh;
                                if (!actor->Mesh) continue;


                                const BYTE bodyHead[] = { 4, 5, 6, 51, 7, 6, 80, 7, 8, 9 };
                                const BYTE neckHandR[] = { 80, 81, 82, 83, 84 };
                                const BYTE neckHandL[] = { 51, 52, 53, 54, 55 };
                                const BYTE bodyFootR[] = { 4, 111, 112, 113, 114 };
                                const BYTE bodyFootL[] = { 4, 106, 107, 108, 109 };

                                const std::pair<const BYTE*, const BYTE> skeleton[] = { {bodyHead, 10}, {neckHandR, 5}, {neckHandL, 5}, {bodyFootR, 5}, {bodyFootL, 5} };



                                const FMatrix comp2world = mesh->K2_GetComponentToWorld().ToMatrixWithScale();

                                if (!Drawing::RenderSkeleton(localController, mesh, comp2world, skeleton, 5, col)) continue;


                            }

                            continue;

                        }

                        //Skeletons
                        else if ((cfg.visuals.skeletons.bSkeleton || cfg.visuals.skeletons.bName) && actor->isSkeleton() && !actor->IsDead()) {
                            // todo: make a function to draw both skeletons and players as they are similar
                            FVector origin, extent;
                            actor->GetActorBounds(true, origin, extent);

                            const FVector location = actor->K2_GetActorLocation();
                            FVector2D headPos;
                            if (!localController->ProjectWorldLocationToScreen({ location.X, location.Y, location.Z + extent.Z }, headPos)) continue;
                            FVector2D footPos;
                            if (!localController->ProjectWorldLocationToScreen({ location.X, location.Y, location.Z - extent.Z }, footPos)) continue;

                            const float height = abs(footPos.Y - headPos.Y);
                            const float width = height * 0.4f;

                            const bool bVisible = localController->LineOfSightTo(actor, cameraLoc, false);
                            const ImVec4 col = bVisible ? cfg.visuals.skeletons.colorVis : cfg.visuals.skeletons.colorInv;

                            if (cfg.visuals.skeletons.bSkeleton)
                            {
                                auto const mesh = actor->Mesh;
                                if (!actor->Mesh) continue;

                                const BYTE bodyHead[] = { 4, 5, 6, 7, 8, 9 };
                                const BYTE neckHandR[] = { 7, 41, 42, 43 };
                                const BYTE neckHandL[] = { 7, 12, 13, 14 };
                                const BYTE bodyFootR[] = { 4, 71, 72, 73, 74 };
                                const BYTE bodyFootL[] = { 4, 66, 67, 68, 69 };

                                const std::pair<const BYTE*, const BYTE> skeleton[] = { {bodyHead, 6}, {neckHandR, 4}, {neckHandL, 4}, {bodyFootR, 5}, {bodyFootL, 5} };

                                const FMatrix comp2world = mesh->K2_GetComponentToWorld().ToMatrixWithScale();

                                if (!Drawing::RenderSkeleton(localController, mesh, comp2world, skeleton, 5, col)) continue;


                            }

                            switch (cfg.visuals.skeletons.boxType)
                            {
                                case Config::EBox::E2DBoxes:
                                {
                                    Drawing::Render2DBox(headPos, footPos, height, width, col);
                                    break;
                                }
                                case Config::EBox::E3DBoxes:
                                {
                                    FRotator rotation = actor->K2_GetActorRotation();
                                    if (!Drawing::Render3DBox(localController, origin, extent, rotation, col)) continue;
                                    break;
                                }
                                /*
                                case Config::EBox::EDebugBoxes:
                                {
                                    UKismetMathLibrary::DrawDebugBox(actor, origin, extent, *reinterpret_cast<const FLinearColor*>(&col), actor->K2_GetActorRotation(), 0.0f);
                                    break;
                                }
                                */
                            }

                            if (cfg.visuals.skeletons.bName)
                            {
                                const int dist = localLoc.DistTo(location) * 0.01f;
                                char name[0x64];
                                sprintf_s(name, "Skeleton [%d]", dist);
                                Drawing::RenderText(name, headPos, cfg.visuals.skeletons.textCol);
                            }

                            if (cfg.visuals.skeletons.barType != Config::EBar::ENone)
                            {
                                auto const healthComp = actor->HealthComponent;
                                if (!healthComp) continue;
                                const float hp = healthComp->GetCurrentHealth() / healthComp->GetMaxHealth();
                                const float width2 = width * 0.5f;
                                const float adjust = height * 0.025f;

                                switch (cfg.visuals.skeletons.barType)
                                {
                                    case Config::EBar::ELeft:
                                    {
                                        const float len = height * hp;
                                        drawList->AddRectFilled({ headPos.X - width2 - adjust * 2.f, headPos.Y }, { headPos.X - width2 - adjust, footPos.Y - len }, ImGui::GetColorU32(IM_COL32(255, 0, 0, 255)));
                                        drawList->AddRectFilled({ headPos.X - width2 - adjust * 2.f, footPos.Y - len }, { headPos.X - width2 - adjust, footPos.Y }, ImGui::GetColorU32(IM_COL32(0, 255, 0, 255)));
                                        break;
                                    }
                                    case Config::EBar::ERight:
                                    {
                                        const float len = height * hp;
                                        drawList->AddRectFilled({ headPos.X + width2 + adjust, headPos.Y }, { headPos.X + width2 + adjust * 2.f, footPos.Y - len }, ImGui::GetColorU32(IM_COL32(255, 0, 0, 255)));
                                        drawList->AddRectFilled({ headPos.X + width2 + adjust, footPos.Y - len }, { headPos.X + width2 + adjust * 2.f, footPos.Y }, ImGui::GetColorU32(IM_COL32(0, 255, 0, 255)));
                                        break;
                                    }
                                    case Config::EBar::EBottom:
                                    {
                                        const float len = width * hp;
                                        drawList->AddRectFilled({ headPos.X - width2, footPos.Y + adjust }, { headPos.X - width2 + len, footPos.Y + adjust * 2.f }, ImGui::GetColorU32(IM_COL32(0, 255, 0, 255)));
                                        drawList->AddRectFilled({ headPos.X - width2 + len, footPos.Y + adjust }, { headPos.X + width2, footPos.Y + adjust * 2.f }, ImGui::GetColorU32(IM_COL32(255, 0, 0, 255)));
                                        break;
                                    }
                                    case Config::EBar::ETop:
                                    {
                                        const float len = width * hp;
                                        drawList->AddRectFilled({ headPos.X - width2, headPos.Y - adjust * 2.f }, { headPos.X - width2 + len, headPos.Y - adjust }, ImGui::GetColorU32(IM_COL32(0, 255, 0, 255)));
                                        drawList->AddRectFilled({ headPos.X - width2 + len, headPos.Y - adjust * 2.f }, { headPos.X + width2, headPos.Y - adjust }, ImGui::GetColorU32(IM_COL32(255, 0, 0, 255)));
                                        break;
                                    }
                                }
                            }
                            continue;
                        }

                        //Animals
                        else if (cfg.visuals.animals.bEnable && actor->isAnimal()) {
                            
                            if (cfg.visuals.cannon.bAimHelp) {
                                DisplayAimHelper(localController, actor, cameraLoc, cfg.visuals.animals.colorVis, cfg.visuals.animals.colorInv, cfg.visuals.animals.textCol, localLoc, cfg.visuals.animals.boxType, true);
                            }
                            else {
                                DisplayAimHelper(localController, actor, cameraLoc, cfg.visuals.animals.colorVis, cfg.visuals.animals.colorInv, cfg.visuals.animals.textCol, localLoc, cfg.visuals.animals.boxType, false);
                                
                            }
                            continue;
                        }

                        //Sharks
                        else if (cfg.visuals.sharks.bEnable && actor->isShark())
                        {
                            FVector origin, extent;
                            actor->GetActorBounds(true, origin, extent);

                            FVector2D headPos;
                            if (!localController->ProjectWorldLocationToScreen({ origin.X, origin.Y, origin.Z + extent.Z }, headPos)) continue;
                            FVector2D footPos;
                            if (!localController->ProjectWorldLocationToScreen({ origin.X, origin.Y, origin.Z - extent.Z }, footPos)) continue;

                            const float height = abs(footPos.Y - headPos.Y);
                            const float width = height * 0.6f;

                            const bool bVisible = localController->LineOfSightTo(actor, cameraLoc, false);
                            const ImVec4 col = bVisible ? cfg.visuals.animals.colorVis : cfg.visuals.animals.colorInv;

                            auto const shark = reinterpret_cast<ASharkPawn*>(actor);
                            auto const mesh = shark->Mesh;
                            if (!actor->Mesh) continue;
                            const FMatrix comp2world = mesh->K2_GetComponentToWorld().ToMatrixWithScale();
                            switch (shark->SwimmingCreatureType)
                            {
                                case ESwimmingCreatureType::Shark:
                                {
                                    const BYTE bone1[] = { 17, 16, 5, 6, 7, 8, 9, 10, 11, 12 };
                                    const BYTE bone2[] = { 10, 13, 14 };
                                    const BYTE bone3[] = { 5, 18, 19 };
                                    const BYTE bone4[] = { 6, 15, 7 };
                                    const std::pair<const BYTE*, const BYTE> skeleton[] = { {bone1, 10}, {bone2, 3}, {bone3, 3}, {bone4, 3} };
                                    if (!Drawing::RenderSkeleton(localController, mesh, comp2world, skeleton, 4, col)) continue;
                                    break;
                                }
                                case ESwimmingCreatureType::TinyShark:
                                {
                                    const BYTE bone1[] = { 26, 25, 24, 23, 22, 21, 20, 19 };
                                    const BYTE bone2[] = { 28, 27, 24 };
                                    const BYTE bone3[] = { 33, 32, 31, 21, 34, 35, 36 };
                                    const std::pair<const BYTE*, const BYTE> skeleton[] = { {bone1, 8}, {bone2, 3}, {bone3, 7} };
                                    if (!Drawing::RenderSkeleton(localController, mesh, comp2world, skeleton, 3, col)) continue;
                                    break;
                                }
                            }

                            char name[0x20];
                            const int dist = localLoc.DistTo(origin) * 0.01f;
                            sprintf_s(name, "Shark [%d]", dist);
                            const float adjust = height * 0.05f;
                            FVector2D pos = { headPos.X, headPos.Y - adjust };
                            Drawing::RenderText(name, pos, cfg.visuals.sharks.textCol);

                            continue;
                        }

                        //Ship
                        //Ship Functions
                        //Function Athena.ShipServiceInterface.GetCrewFromShip
                        //Function Athena.Ship.GetCharacterShipRegion
                        //Function Athena.Ship.GetCurrentVelocity 
                        //Function Athena.Ship.GetDeckSurfaceWater
                        //Function Athena.Ship.GetHullDamage
                        //Function Athena.Ship.GetInternalWater 
                        //Function Athena.Ship.GetShipLocatorPosition 
                        //Function Athena.Ship.GetShipRegion 
                        //Function Athena.ShipService.GetNumShips 
                        else if (actor->isShip() && (cfg.visuals.ships.bName || cfg.visuals.ships.bDamage))
                        {
                            const FVector location = actor->K2_GetActorLocation();
                            const int dist = localLoc.DistTo(location) * 0.01f;

                            //Show Name + Water 
                            if (cfg.visuals.ships.bName && dist <= 1500)
                            {
                                FVector2D screen;
                                if (localController->ProjectWorldLocationToScreen(location, screen)) {
                                    int amount = 0;
                                    auto water = actor->GetInternalWater();
                                    if (water) amount = water->GetNormalizedWaterAmount() * 100.f;
                                    char name[0x40];
                                    sprintf_s(name, "Ship (%d%%) [%d]", amount, dist);
                                    Drawing::RenderText(const_cast<char*>(name), screen, cfg.visuals.ships.textCol);
                                };
                            }

                            //Show Holes
                            if (cfg.visuals.ships.bDamage && dist <= 300)
                            {
                                auto const damage = actor->GetHullDamage();
                                if (!damage) continue;
                                const auto holes = damage->ActiveHullDamageZones;
                                for (auto h = 0u; h < holes.Count; h++)
                                {
                                    auto const hole = holes[h];
                                    const FVector location = hole->K2_GetActorLocation();
                                    FVector2D screen;
                                    if (localController->ProjectWorldLocationToScreen(location, screen))
                                    {
                                        auto color = cfg.visuals.ships.damageColor;
                                        drawList->AddLine({ screen.X - 6.f, screen.Y + 6.f }, { screen.X + 6.f, screen.Y - 6.f }, ImGui::GetColorU32(color));
                                        drawList->AddLine({ screen.X - 6.f, screen.Y - 6.f }, { screen.X + 6.f, screen.Y + 6.f }, ImGui::GetColorU32(color));
                                    }
                                }
                            }

                            switch (cfg.visuals.ships.boxType)
                            {
                            case Config::EShipBox::E3DBoxes:
                            {

                                FVector origin, extent;
                                actor->GetActorBounds(true, origin, extent);
                                FRotator rotation = actor->K2_GetActorRotation();
                                if (!Drawing::Render3DBox(localController, origin, extent, rotation, cfg.visuals.ships.boxColor)) continue;
                                break;
                            }
                            }

                            continue;
                        }
                        
                        //Far Ship
                        else if (actor->isFarShip() && (cfg.visuals.ships.bName || cfg.visuals.ships.bDamage))
                        {
                            const FVector location = actor->K2_GetActorLocation();
                            const int dist = localLoc.DistTo(location) * 0.01f;

                            if (cfg.visuals.ships.bName && dist > 1500)
                            {
                                FVector2D screen;
                                if (localController->ProjectWorldLocationToScreen(location, screen)) {
                                    char name[0x30];
                                    sprintf_s(name, "Ship [%d]", dist);
                                    Drawing::RenderText(const_cast<char*>(name), screen, cfg.visuals.ships.textCol);
                                };
                            }
                            continue;
                        }

                        //Puzzle
                        if (cfg.visuals.puzzles.bEnable && actor->isPuzzleVault())
                        {
                            auto vault = reinterpret_cast<APuzzleVault*>(actor);

                            const FVector location = reinterpret_cast<ACharacter*>(vault->OuterDoor)->K2_GetActorLocation();
                            FVector2D screen;

                            if (localController->ProjectWorldLocationToScreen(location, screen)) {
                                char name[0x64];
                                const int dist = localLoc.DistTo(location) * 0.01f;
                                sprintf_s(name, "Vault door [%d]", dist);
                                Drawing::RenderText(name, screen, cfg.visuals.puzzles.textCol);
                            };

                            continue;
                        }
                    }
                    
                }
            }

            //Islands (BROKE)
            if (cfg.visuals.islands.bEnable)
            {
                do {
                    auto const islandService = gameState->IslandService;
                    if (!islandService) break;
                    auto const islandDataAsset = islandService->IslandDataAsset;
                    if (!islandDataAsset) break;
                    auto const islandDataEntries = islandDataAsset->IslandDataEntries;
                    if (!islandDataEntries.Data) break;
                    for (auto i = 0u; i < islandDataEntries.Count; i++)
                    {
                        auto const island = islandDataEntries[i];
                        auto const WorldMapData = island->WorldMapData;
                        if (!WorldMapData) continue;

                        const FVector islandLoc = WorldMapData->WorldSpaceCameraPosition;
                        const int dist = localLoc.DistTo(islandLoc) * 0.01f;

                        if (dist > cfg.visuals.islands.intMaxDist) continue;
                        FVector2D screen;
                        if (localController->ProjectWorldLocationToScreen(islandLoc, screen))
                        {
                            char name[0x64];
                            auto len = island->LocalisedName->multi(name, 0x50);


                            snprintf(name + len, sizeof(name) - len, " [%d]", dist);
                            Drawing::RenderText(name, screen, cfg.visuals.islands.textCol);

                        }
                    }

                } while (false);
            }

            //Crosshair
            if (cfg.visuals.client.bCrosshair)
            {
                drawList->AddLine({ io.DisplaySize.x * 0.5f - cfg.visuals.client.fCrosshair, io.DisplaySize.y * 0.5f }, { io.DisplaySize.x * 0.5f + cfg.visuals.client.fCrosshair, io.DisplaySize.y * 0.5f }, ImGui::GetColorU32(cfg.visuals.client.crosshairColor));
                drawList->AddLine({ io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f - cfg.visuals.client.fCrosshair }, { io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f + cfg.visuals.client.fCrosshair }, ImGui::GetColorU32(cfg.visuals.client.crosshairColor));
            }

            //Oxygen
            if (cfg.visuals.client.bOxygen && localCharacter->IsInWater())
            {
                auto drownComp = localCharacter->DrowningComponent;
                if (!drownComp) break;
                auto level = drownComp->GetOxygenLevel();
                auto posX = io.DisplaySize.x * 0.5f;
                auto posY = io.DisplaySize.y * 0.85f;
                auto barWidth2 = io.DisplaySize.x * 0.05f;
                auto barHeight2 = io.DisplaySize.y * 0.0030f;
                drawList->AddRectFilled({ posX - barWidth2, posY - barHeight2 }, { posX + barWidth2, posY + barHeight2 }, ImGui::GetColorU32(IM_COL32(0, 0, 0, 255)));
                drawList->AddRectFilled({ posX - barWidth2, posY - barHeight2 }, { posX - barWidth2 + barWidth2 * level * 2.f, posY + barHeight2 }, ImGui::GetColorU32(IM_COL32(0, 200, 255, 255)));
            }

            //Compass
            if (cfg.visuals.client.bCompass)
            {

                //float dist = cameraLoc.DistTo(location);

                const char* directions[] = { "N", "NE", "E", "SE", "S", "SW", "W", "NW" };
                int yaw = ((int)cameraRot.Yaw + 450) % 360;
                int index = int(yaw + 22.5f) % 360 * 0.0222222f;

                FVector2D pos = { io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.02f };
                auto col = ImVec4(1.f, 1.f, 1.f, 1.f);

                Drawing::RenderText(const_cast<char*>(directions[index]), pos, col);

                char buf[0x30];
                int len = sprintf_s(buf, "%d", yaw);
                pos.Y += 15.f;
                Drawing::RenderText(buf, pos, col);

            }
            
            //Actual Aimbot
            if (aimBest.target != nullptr)
            {
                FVector2D screen;
                if (localController->ProjectWorldLocationToScreen(aimBest.location, screen)) 
                {
                    auto col = ImGui::GetColorU32(IM_COL32(0, 200, 0, 255));
                    drawList->AddLine({ io.DisplaySize.x * 0.5f , io.DisplaySize.y * 0.5f }, { screen.X, screen.Y }, col);
                    drawList->AddCircle({ screen.X, screen.Y }, 3.f, col);
                }

                if (ImGui::IsMouseDown(1))
                {
                    
                    if (isHarpoon)
                    {
                        //Turns the Harpoon I guess
                        //reinterpret_cast<AHarpoonLauncher*>(attachObject)->rotation = aimBest.delta;
                    }
                    else {
                        /*
                        * LV - Local velocity
                        * TV - Target velocity
                        * RV - Target relative velocity
                        * BS - Bullet speed
                        * RL - Relative local location
                        */
                        FVector LV = localCharacter->GetVelocity();
                        if (auto const localShip = localCharacter->GetCurrentShip()) {
                            LV += localShip->GetVelocity();
                        }

                        FVector TV = aimBest.target->GetVelocity();
                        if (auto const targetShip = aimBest.target->GetCurrentShip()) {
                            TV += targetShip->GetVelocity();
                        }

                        const FVector RV = TV - LV; //Target relative velocity
                        const float BS = localWeapon->WeaponParameters.AmmoParams.Velocity;
                        const FVector RL = localLoc - aimBest.location;
                        const float a = RV.Size() - BS * BS;
                        const float b = (RL * RV * 2.f).Sum();
                        const float c = RL.SizeSquared();
                        const float D = b*b - 4 * a * c;
                        if (D > 0)
                        {
                            const float DRoot = sqrtf(D);
                            const float x1 = (-b + DRoot) / (2 * a);
                            const float x2 = (-b - DRoot) / (2 * a);
                            if (x1 >= 0 && x1 >= x2) aimBest.location += RV * x1;
                            else if (x2 >= 0) aimBest.location += RV * x2;

                            aimBest.delta = UKismetMathLibrary::NormalizedDeltaRotator(UKismetMathLibrary::FindLookAtRotation(cameraLoc, aimBest.location), cameraRot);
                            auto smoothness = 1.f / aimBest.smoothness;
                            //localController->AddYawInput(aimBest.delta.Yaw* smoothness);
                            //localController->AddPitchInput(aimBest.delta.Pitch * -smoothness);
                        }

                    }
                }
               
            }

            if (!localController->IdleDisconnectEnabled && !(cfg.misc.bEnable && cfg.misc.client.bEnable && cfg.misc.client.bIdleKick))
            {
                localController->IdleDisconnectEnabled = true;
            }

            if (cfg.misc.bEnable)
            {
                if (cfg.misc.client.bEnable) 
                {
                    if (cfg.misc.client.bShipInfo)
                    {
                        auto ship = localCharacter->GetCurrentShip();
                        if (ship)
                        {
                            FVector velocity = ship->GetVelocity() / 100.f;
                            char buf[0xFF];
                            sprintf(buf, "X: %.0f Y: %.0f Z: %.0f", velocity.X, velocity.Y, velocity.Z);

                            FVector2D pos {10.f, 50.f};
                            ImVec4 col{ 1.f,1.f,1.f,1.f };
                            Drawing::RenderText(buf, pos, col, true, false);

                            auto speed = velocity.Size();
                            sprintf(buf, "Speed: %.0f", speed);
                            pos.Y += 15.f;
                            Drawing::RenderText(buf, pos, col, true, false);

                        }
                    }
                    if (localController->IdleDisconnectEnabled && cfg.misc.client.bIdleKick)
                    {
                        localController->IdleDisconnectEnabled = false;
                    }

                }
                if (cfg.misc.game.bEnable)
                {
                    if (cfg.misc.game.bShowPlayers) 
                    {
                        ImGui::PopStyleColor();
                        ImGui::PopStyleVar(2);
                        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x * 0.2f, io.DisplaySize.y * 0.25f), ImGuiCond_Once);
                        ImGui::Begin("PlayersList", 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);
                        
                        auto shipsService = gameState->ShipService;
                        if (shipsService)
                        {
                            ImGui::BeginChild("Info", { 0.f, 15.f });
                            ImGui::Text("Total ships (including AI): %d", shipsService->GetNumShips());
                            ImGui::EndChild();
                        }
                        
                        auto crewService = gameState->CrewService;
                        auto crews = crewService->Crews;
                        if (crews.Data)
                        {
                            ImGui::Columns(3, "CrewPlayers");
                            ImGui::Separator();
                            ImGui::Text("Name"); ImGui::NextColumn();
                            ImGui::Text("Activity"); ImGui::NextColumn();
                            ImGui::Text("Distance"); ImGui::NextColumn();
                            ImGui::Separator();
                            for (uint32_t i = 0; i < crews.Count; i++)
                            {
                                auto& crew = crews[i];
                                auto players = crew.Players;
                                if (players.Data)
                                {
                                    for (uint32_t k = 0; k < players.Count; k++)
                                    {
                                        auto& player = players[k];
                                        char buf[0x64];
                                        player->PlayerName.multi(buf, 0x50);
                                        
                                        ImGui::Selectable(buf);
                                        if (ImGui::BeginPopupContextItem(buf))
                                        {
                                            ImGui::Button("gay");
                                            ImGui::EndPopup();
                                        }
                                        ImGui::NextColumn();
                                        const char* actions[] = { "None", "Bailing", "Cannon", "CannonEnd", "Capstan", "CapstanEnd", "CarryingBooty", "CarryingBootyEnd", "Dead", "DeadEnd", "Digging", "Dousing", "EmptyingBucket", "Harpoon", "Harpoon_END", "LoseHealth", "Repairing", "Sails", "Sails_END", "Wheel", "Wheel_END" };
                                        auto activity = (uint8_t)player->GetPlayerActivity();
                                        if (activity < 21) { 
                                            ImGui::Text(actions[activity]); 
                                        }
                                      
                                        ImGui::NextColumn();

                                        char name[0x64];
                                        //sprintf_s(name, "%d m", distanceArray[(k-1)]);
                                        sprintf_s(name, "%d m", distanceArray[0]);

                                        const char* text = name;
                                        ImGui::Text(text);
                                        ImGui::NextColumn();
                                    }
                                    ImGui::Separator();
                                }

                            }
                            ImGui::Columns();
                        }
                        ImGui::End();
                        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
                        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
                        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
                    }
                    
                }
            }

        } while (false);
    }
    catch (const std::exception& exc)
    {
        const char* thros = exc.what();
        Logger::Log(thros);
    }

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
    
    if (ImGui::IsKeyPressed(VK_INSERT)) bIsOpen = !bIsOpen;

    //Actual GUI
    if (bIsOpen) {
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x * 0.3f, io.DisplaySize.y * 0.8f), ImGuiCond_Once);
        ImGui::Begin("Menu", 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
        if (ImGui::BeginTabBar("Bars")) {
            //Tab 1 "Visuals"
            if (ImGui::BeginTabItem("ESP")) {

                //ImGui::Columns(2, "CLM1", false);
                const char* boxes[] = { "None", "2DBox", "3DBox" };
                
                ImGui::Text("Players");
                if (ImGui::BeginChild("PlayersSettings", ImVec2(0.f, 90.f), true, 0))
                {
                    ImGui::Checkbox("Draw teammates", &cfg.visuals.players.bDrawTeam);
                    ImGui::Checkbox("Draw name", &cfg.visuals.players.bName);
                    ImGui::Checkbox("Draw skeleton", &cfg.visuals.players.bSkeleton);
                }
                ImGui::EndChild();

                //ImGui::NextColumn();

                ImGui::Text("Skeletons");
                if (ImGui::BeginChild("SkeletonsSettings", ImVec2(0.f, 65.f), true, 0))
                {
                    ImGui::Checkbox("Draw name", &cfg.visuals.skeletons.bName);
                    ImGui::Checkbox("Draw skeleton", &cfg.visuals.skeletons.bSkeleton);
                }
                ImGui::EndChild();

                //ImGui::NextColumn();

                ImGui::Text("Ships");
                if (ImGui::BeginChild("ShipsSettings", ImVec2(0.f, 65.f), true, 0)) {
                    ImGui::Checkbox("Draw name", &cfg.visuals.ships.bName);
                    ImGui::Checkbox("Show holes", &cfg.visuals.ships.bDamage);   
                }
                ImGui::EndChild();

                //ImGui::NextColumn();

                ImGui::Text("Other");
                if (ImGui::BeginChild("Other", ImVec2(0.f, 224.f), true, 0)) {
                    ImGui::Checkbox("Show Islands", &cfg.visuals.islands.bEnable);
                    ImGui::Checkbox("Show Items", &cfg.visuals.items.bEnable);
                    ImGui::Checkbox("Show Barrels", &cfg.visuals.barrels.bEnable);
                    ImGui::Checkbox("Show Shipwrecks", &cfg.visuals.shipwrecks.bEnable);
                    ImGui::Checkbox("Show Animals", &cfg.visuals.animals.bEnable);
                    //ImGui::Checkbox("Draw Shark Skeleton", &cfg.visuals.sharks.bSkeleton);
                    ImGui::Checkbox("Show Sharks", &cfg.visuals.sharks.bEnable);
                    ImGui::Checkbox("Show Puzzle Doors", &cfg.visuals.puzzles.bEnable);
                    ImGui::Checkbox("Show Cannon Aim Helper", &cfg.visuals.cannon.bAimHelp);
                }
                ImGui::EndChild();

                //ImGui::NextColumn();

                ImGui::Text("Client");
                if (ImGui::BeginChild("ClientSettings", ImVec2(0.f, 220.f), true, 0))
                {

                    ImGui::Checkbox("Crosshair", &cfg.visuals.client.bCrosshair);
                    if (cfg.visuals.client.bCrosshair)
                    {
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(75.f);
                        ImGui::SliderFloat("Radius##1", &cfg.visuals.client.fCrosshair, 1.f, 100.f);
                    }

                    ImGui::Checkbox("Oxygen level", &cfg.visuals.client.bOxygen);
                    ImGui::Checkbox("Compass", &cfg.visuals.client.bCompass);

                    ImGui::Checkbox("Debug", &cfg.visuals.client.bDebug);
                    if (cfg.visuals.client.bDebug)
                    {
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(150.f);
                        ImGui::SliderFloat("Radius##2", &cfg.visuals.client.fDebug, 1.f, 1000.f);
                    }
                    ImGui::ColorEdit4("Crosshair color", &cfg.visuals.client.crosshairColor.x, ImGuiColorEditFlags_DisplayRGB);

                }

                ImGui::EndChild();
                //ImGui::Columns();

                ImGui::EndTabItem();
            }
            //Tab 2 "Aim"
            /*
            if (ImGui::BeginTabItem("Aim")) {

                ImGui::Text("Global Aim");
                if (ImGui::BeginChild("Global", ImVec2(0.f, 38.f), true, 0))
                {
                    ImGui::Checkbox("Enable", &cfg.aim.bEnable);
                }
                ImGui::EndChild();

                
                ImGui::Columns(2, "CLM1", false);
                ImGui::Text("Players");
                if (ImGui::BeginChild("PlayersSettings", ImVec2(0.f, 180.f), true, 0))
                {
                    // todo: add bones selection
                    ImGui::Checkbox("Enable", &cfg.aim.players.bEnable);
                    ImGui::Checkbox("Visible only", &cfg.aim.players.bVisibleOnly);
                    ImGui::Checkbox("Aim at teammates", &cfg.aim.players.bTeam);
                    ImGui::SliderFloat("Yaw", &cfg.aim.players.fYaw, 1.f, 180.f, "%.0f", ImGuiSliderFlags_AlwaysClamp);
                    ImGui::SliderFloat("Pitch", &cfg.aim.players.fPitch, 1.f, 180.f, "%.0f", ImGuiSliderFlags_AlwaysClamp);
                    ImGui::SliderFloat("Smoothness", &cfg.aim.players.fSmoothness, 1.f, 100.f, "%.0f", ImGuiSliderFlags_AlwaysClamp);
                }
                ImGui::EndChild();

                ImGui::NextColumn();

                ImGui::Text("Skeletons");
                if (ImGui::BeginChild("SkeletonsSettings", ImVec2(0.f, 180.f), true, 0))
                {
                    ImGui::Checkbox("Enable", &cfg.aim.skeletons.bEnable);
                    ImGui::Checkbox("Visible only", &cfg.aim.skeletons.bVisibleOnly);
                    ImGui::SliderFloat("Yaw", &cfg.aim.skeletons.fYaw, 1.f, 180.f, "%.0f", ImGuiSliderFlags_AlwaysClamp);
                    ImGui::SliderFloat("Pitch", &cfg.aim.skeletons.fPitch, 1.f, 180.f, "%.0f", ImGuiSliderFlags_AlwaysClamp);
                    ImGui::SliderFloat("Smoothness", &cfg.aim.skeletons.fSmoothness, 1.f, 100.f, "%.0f", ImGuiSliderFlags_AlwaysClamp);
                    

                }
                ImGui::EndChild();

                ImGui::NextColumn();

                ImGui::Text("Harpoon");
                if (ImGui::BeginChild("HarpoonSettings", ImVec2(0.f, 180.f), true, 0))
                {
                    ImGui::Checkbox("Enable", &cfg.aim.harpoon.bEnable);
                    ImGui::Checkbox("Visible only", &cfg.aim.harpoon.bVisibleOnly);
                    ImGui::SliderFloat("Yaw", &cfg.aim.harpoon.fYaw, 1.f, 100.f, "%.0f", ImGuiSliderFlags_AlwaysClamp);
                    ImGui::SliderFloat("Pitch", &cfg.aim.harpoon.fPitch, 1.f, 102.f, "%.0f", ImGuiSliderFlags_AlwaysClamp);
                }
                ImGui::EndChild();

                ImGui::NextColumn();

                /*
                
               
                ImGui::Text("Cannon");
                if (ImGui::BeginChild("CannonSettings", ImVec2(0.f, 180.f), true, 0))
                {
                    ImGui::Checkbox("Enable", &cfg.aim.cannon.bEnable);
                    ImGui::Checkbox("Visible only", &cfg.aim.cannon.bVisibleOnly);
                    ImGui::SliderFloat("Yaw", &cfg.aim.cannon.fYaw, 1.f, 100.f, "%.0f", ImGuiSliderFlags_AlwaysClamp);
                    ImGui::SliderFloat("Pitch", &cfg.aim.cannon.fPitch, 1.f, 102.f, "%.0f", ImGuiSliderFlags_AlwaysClamp);
                }
                ImGui::EndChild();

                 

                ImGui::Columns();



                ImGui::EndTabItem();
            }
            */
            //Tab 3 "Misc"
            if (ImGui::BeginTabItem("Misc")) {

                ImGui::Text("Global Misc");
                if (ImGui::BeginChild("Global", ImVec2(0.f, 38.f), true, 0))
                {
                    ImGui::Checkbox("Enable", &cfg.misc.bEnable);
                }
                ImGui::EndChild();

                ImGui::Columns(2, "CLM1", false);
                ImGui::Text("Client");
                if (ImGui::BeginChild("ClientSettings", ImVec2(0.f, 180.f), true, 0))
                {
                    ImGui::Checkbox("Enable", &cfg.misc.client.bEnable);
                    ImGui::Checkbox("Ship speed", &cfg.misc.client.bShipInfo);
                    ImGui::Checkbox("Disable idle kick", &cfg.misc.client.bIdleKick);
                    ImGui::Separator();
                    if (ImGui::Button("Save settings"))
                    {
                        do {
                            wchar_t buf[MAX_PATH];
                            GetModuleFileNameW(hinstDLL, buf, MAX_PATH);
                            fs::path path = fs::path(buf).remove_filename() / ".settings";
                            auto file = CreateFileW(path.wstring().c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                            if (file == INVALID_HANDLE_VALUE) break;
                            DWORD written;
                            if (WriteFile(file, &cfg, sizeof(cfg), &written, 0)) ImGui::OpenPopup("##SettingsSaved");
                            CloseHandle(file);
                        } while (false);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Load settings")) 
                    {
                        do {
                            wchar_t buf[MAX_PATH];
                            GetModuleFileNameW(hinstDLL, buf, MAX_PATH);
                            fs::path path = fs::path(buf).remove_filename() / ".settings";
                            auto file = CreateFileW(path.wstring().c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                            if (file == INVALID_HANDLE_VALUE) break;
                            DWORD readed;
                            if (ReadFile(file, &cfg, sizeof(cfg), &readed, 0))  ImGui::OpenPopup("##SettingsLoaded");
                            CloseHandle(file);
                        } while (false);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Tests")) {
                        auto h = CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(Tests), nullptr, 0, nullptr);
                        if (h) CloseHandle(h);
                    }

                    ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
                    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                    if (ImGui::BeginPopupModal("##SettingsSaved", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
                    {
                        ImGui::Text("\nSettings have been saved\n\n");
                        ImGui::Separator();
                        if (ImGui::Button("OK", { 170.f , 0.f })) { ImGui::CloseCurrentPopup(); }
                        ImGui::EndPopup();
                    }
                    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                    if (ImGui::BeginPopupModal("##SettingsLoaded", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
                    {
                        ImGui::Text("\nSettings have been loaded\n\n");
                        ImGui::Separator();
                        if (ImGui::Button("OK", { 170.f , 0.f })) { ImGui::CloseCurrentPopup(); }
                        ImGui::EndPopup();
                    }
                }
                ImGui::EndChild();

                ImGui::NextColumn();

                ImGui::Text("Game");
                if (ImGui::BeginChild("GameSettings", ImVec2(0.f, 180.f), true, 0))
                {
                    ImGui::Checkbox("Enable", &cfg.misc.game.bEnable);
                    ImGui::Checkbox("Show players list", &cfg.misc.game.bShowPlayers);
                }
                ImGui::EndChild();

                ImGui::NextColumn();


                ImGui::Text("Kraken");
                if (ImGui::BeginChild("KrakenSettings", ImVec2(0.f, 180.f), true, 0))
                {
                    

                    AKrakenService* krakenService;
                    bool isActive = false;
                    if (cache.good)  
                    { 
                        krakenService = cache.gameState->KrakenService;
                        if (krakenService) { krakenService->IsKrakenActive(); }
                    }
                    ImGui::Text("IsKrakenActive: %d", isActive);

                    /*
                    if (ImGui::Button("Attempt to request kraken"))
                    {
                        if (krakenService) { krakenService->RequestKrakenWithLocation(localCharacter->K2_GetActorLocation(), localCharacter); }
                    }
                    if (ImGui::Button("Dismiss kraken"))
                    {
                        if (krakenService) { krakenService->DismissKraken(); }
                    }
                    */
                }
                ImGui::EndChild();

                ImGui::Columns();


                ImGui::EndTabItem();
            }
            //Tab 4 "Configuration"
            if (ImGui::BeginTabItem("Configuration")) {

                ImGui::Columns(2, "CLM1", false);
                const char* boxes[] = { "None", "2DBox", "3DBox" };

                ImGui::Text("Players");
                if (ImGui::BeginChild("PlayersSettings", ImVec2(0.f, 310.f), true, 0))
                {
                    const char* bars[] = { "None", "2DRectLeft", "2DRectRight", "2DRectBottom", "2DRectTop" };
                    ImGui::Combo("Box type", reinterpret_cast<int*>(&cfg.visuals.players.boxType), boxes, IM_ARRAYSIZE(boxes));
                    ImGui::Combo("Health bar type", reinterpret_cast<int*>(&cfg.visuals.players.barType), bars, IM_ARRAYSIZE(bars));
                    ImGui::ColorEdit4("Visible Enemy color", &cfg.visuals.players.enemyColorVis.x, 0);
                    ImGui::ColorEdit4("Invisible Enemy color", &cfg.visuals.players.enemyColorInv.x, 0);
                    ImGui::ColorEdit4("Visible Team color", &cfg.visuals.players.teamColorVis.x, 0);
                    ImGui::ColorEdit4("Invisible Team color", &cfg.visuals.players.teamColorInv.x, 0);
                    ImGui::ColorEdit4("Text color", &cfg.visuals.players.textCol.x, 0);
                }
                ImGui::EndChild();

                ImGui::NextColumn();

                ImGui::Text("Skeletons");
                if (ImGui::BeginChild("SkeletonsSettings", ImVec2(0.f, 310.f), true, 0))
                {
                    const char* bars[] = { "None", "2DRectLeft", "2DRectRight", "2DRectBottom", "2DRectTop" };
                    ImGui::Combo("Box type", reinterpret_cast<int*>(&cfg.visuals.skeletons.boxType), boxes, IM_ARRAYSIZE(boxes));
                    ImGui::Combo("Health bar type", reinterpret_cast<int*>(&cfg.visuals.skeletons.barType), bars, IM_ARRAYSIZE(bars));
                    ImGui::ColorEdit4("Visible Color", &cfg.visuals.skeletons.colorVis.x, 0);
                    ImGui::ColorEdit4("Invisible Color", &cfg.visuals.skeletons.colorInv.x, 0);
                    ImGui::ColorEdit4("Text color", &cfg.visuals.skeletons.textCol.x, 0);

                }
                ImGui::EndChild();

                ImGui::NextColumn();

                ImGui::Text("Ships");
                if (ImGui::BeginChild("ShipsSettings", ImVec2(0.f, 200.f), true, 0)) {

                    const char* shipBoxes[] = { "None", "3DBox" };
                    ImGui::Combo("Box type", reinterpret_cast<int*>(&cfg.visuals.ships.boxType), shipBoxes, IM_ARRAYSIZE(shipBoxes));
                    ImGui::ColorEdit4("Box color", &cfg.visuals.ships.boxColor.x, 0);
                    ImGui::ColorEdit4("Damage color", &cfg.visuals.ships.damageColor.x, 0);
                    ImGui::ColorEdit4("Text color", &cfg.visuals.ships.textCol.x, 0);
                }
                ImGui::EndChild();

                ImGui::NextColumn();

                ImGui::Text("Islands");
                if (ImGui::BeginChild("IslandsSettings", ImVec2(0.f, 200.f), true, 0)) {
                    ImGui::SliderInt("Max distance", &cfg.visuals.islands.intMaxDist, 100, 10000, "%d", ImGuiSliderFlags_AlwaysClamp);
                    ImGui::ColorEdit4("Text color", &cfg.visuals.islands.textCol.x, 0);
                }
                ImGui::EndChild();

                ImGui::NextColumn();

                ImGui::Text("Animals");
                if (ImGui::BeginChild("AnimalsSettings", ImVec2(0.f, 220.f), true, 0))
                {
                    ImGui::Combo("Box type", reinterpret_cast<int*>(&cfg.visuals.animals.boxType), boxes, IM_ARRAYSIZE(boxes));
                    ImGui::ColorEdit4("Visible Color", &cfg.visuals.animals.colorVis.x, 0);
                    ImGui::ColorEdit4("Invisible Color", &cfg.visuals.animals.colorInv.x, 0);
                    ImGui::ColorEdit4("Text color", &cfg.visuals.animals.textCol.x, 0);
                }

                ImGui::EndChild();

                ImGui::NextColumn();

                ImGui::Text("Sharks");
                if (ImGui::BeginChild("SharksSettings", ImVec2(0.f, 220.f), true, 0))
                {
                    //ImGui::Combo("Box type", reinterpret_cast<int*>(&cfg.visuals.sharks.boxType), boxes, IM_ARRAYSIZE(boxes));
                    ImGui::ColorEdit4("Visible Color", &cfg.visuals.sharks.colorVis.x, 0);
                    ImGui::ColorEdit4("Invisible Color", &cfg.visuals.sharks.colorInv.x, 0);
                    ImGui::ColorEdit4("Text color", &cfg.visuals.sharks.textCol.x, 0);
                }
                ImGui::EndChild();

                ImGui::NextColumn();

                ImGui::Text("Puzzles");
                if (ImGui::BeginChild("PuzzlesSettings", ImVec2(0.f, 220.f), true, 0))
                {
                    ImGui::ColorEdit4("Text color", &cfg.visuals.puzzles.textCol.x, 0);
                }
                ImGui::EndChild();

                ImGui::Columns();

                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        };
        ImGui::End();
    }
  
    context->OMSetRenderTargets(1, &renderTargetView, nullptr);
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    return PresentOriginal(swapChain, syncInterval, flags);
}

HRESULT Cheat::Renderer::ResizeHook(IDXGISwapChain* swapChain, UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags)
{

    if (renderTargetView)
    {
        ImGui_ImplDX11_Shutdown();
        ImGui::DestroyContext();
        renderTargetView->Release();
        renderTargetView = nullptr;
    }
    if (context)
    {
        context->Release();
        context = nullptr;
    }
    if (device)
    {
        device->Release();
        device = nullptr;
    }
    
    return ResizeOriginal(swapChain, bufferCount, width, height, newFormat, swapChainFlags);
}

bool Cheat::Init(HINSTANCE _hinstDLL)
{
    hinstDLL = _hinstDLL;
    if (!Logger::Init())
    {
        return false;
    };
    if (!K32GetModuleInformation(GetCurrentProcess(), GetModuleHandleA(nullptr), &gBaseMod, sizeof(MODULEINFO))) 
    {
        return false;
    };
    Logger::Log("Base address: %p\n", gBaseMod.lpBaseOfDll);
    if (!Tools::FindNameArray()) 
    {
        Logger::Log("Can't find NameArray!\n");
        return false;
    }
    Logger::Log("NameArray: %p\n", FName::GNames);
    if (!Tools::FindObjectsArray()) 
    {
        Logger::Log("Can't find ObjectsArray!\n");
        return false;
    } 
    Logger::Log("ObjectsArray: %p\n", UObject::GObjects);
    if (!Tools::FindWorld())
    {
        Logger::Log("Can't find World!\n");
        return false;
    }
    Logger::Log("World: %p\n", UWorld::GWorld);
    if (!Tools::InitSDK())
    {
        Logger::Log("Can't find important objects!\n");
        return false;
    };
    
    if (!Renderer::Init())
    {
        Logger::Log("Can't initialize renderer\n");
        return false;
    }
    Hacks::Init();

    auto t = CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(ClearingThread), nullptr, 0, nullptr);
    if (t) CloseHandle(t);

    return true;
}

void Cheat::ClearingThread()
{
    while (true) {
        if (GetAsyncKeyState(VK_END) & 1) {
            FreeLibraryAndExitThread(hinstDLL, 0);
        }
        Sleep(20);
    }
}

void Cheat::Tests()
{
    /*
    auto world = *UWorld::GWorld;
    auto localController = world->GameInstance->LocalPlayers[0]->PlayerController;
    Logger::Log("%p\n", localController);
    */
}

bool Cheat::Remove()
{

    Logger::Log("Removing cheat...\n");
    
    if (!Renderer::Remove() || !Logger::Remove())
    {
        return false;
    };

    Hacks::Remove();


    // some other stuff...

    

    return true;
}
