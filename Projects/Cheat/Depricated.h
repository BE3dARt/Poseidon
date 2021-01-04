#pragma once
/*

                      //Aimbot for FPS Weapons
                        else if (!attachObject && isWieldedWeapon)
                        {
                            if (cfg.aim.players.bEnable && actor->isPlayer() && actor != localCharacter && !actor->IsDead())
                            {
                                do {

                                    FVector playerLoc = actor->K2_GetActorLocation();
                                    float dist = localLoc.DistTo(playerLoc);
                                    if (dist > localWeapon->WeaponParameters.ProjectileMaximumRange) { break; }

                                    if (cfg.aim.players.bVisibleOnly) if (!localController->LineOfSightTo(actor, cameraLoc, false)) { break; }
                                    if (!cfg.aim.players.bTeam) if (UCrewFunctions::AreCharactersInSameCrew(actor, localCharacter)) break;

                                    FRotator rotationDelta = UKismetMathLibrary::NormalizedDeltaRotator(UKismetMathLibrary::FindLookAtRotation(cameraLoc, playerLoc), cameraRot);

                                    float absYaw = abs(rotationDelta.Yaw);
                                    float absPitch = abs(rotationDelta.Pitch);
                                    if (absYaw > cfg.aim.players.fYaw || absPitch > cfg.aim.players.fPitch) { break; }
                                    float sum = absYaw + absPitch;

                                    if (sum < aimBest.best)
                                    {
                                        aimBest.target = actor;
                                        aimBest.location = playerLoc;
                                        aimBest.delta = rotationDelta;
                                        aimBest.best = sum;
                                        aimBest.smoothness = cfg.aim.players.fSmoothness;
                                    }

                                } while (false);
                            }
                            else if (cfg.aim.skeletons.bEnable && actor->isSkeleton() && !actor->IsDead())
                            {
                                do {
                                    //Get Distance to Target
                                    const FVector playerLoc = actor->K2_GetActorLocation();
                                    const float dist = localLoc.DistTo(playerLoc);

                                    //Exit if further than maximal range
                                    if (dist > localWeapon->WeaponParameters.ProjectileMaximumRange) break;

                                    //Exit if target is not visible
                                    if (cfg.aim.skeletons.bVisibleOnly) {
                                        if (!localController->LineOfSightTo(actor, cameraLoc, false)) {
                                            break;
                                        }
                                    }

                                    //Find a rotation for an object at Start location to point at Target location.
                                    const FRotator rotationDelta = UKismetMathLibrary::NormalizedDeltaRotator(UKismetMathLibrary::FindLookAtRotation(cameraLoc, playerLoc), cameraRot);
                                    const float absYaw = abs(rotationDelta.Yaw);
                                    const float absPitch = abs(rotationDelta.Pitch);

                                    if (absYaw > cfg.aim.skeletons.fYaw || absPitch > cfg.aim.skeletons.fPitch) break;
                                    const float sum = absYaw + absPitch;

                                    if (sum < aimBest.best)
                                    {
                                        aimBest.target = actor;
                                        aimBest.location = playerLoc;
                                        aimBest.delta = rotationDelta;
                                        aimBest.best = sum;
                                        aimBest.smoothness = cfg.aim.skeletons.fSmoothness;
                                    }

                                } while (false);
                            }
                        }















*/