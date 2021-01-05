#pragma once

//PADS
//Define Name							//Address		//Static		//Hierarchy Top															//Result
#define UPROPERTY						0xC

#define	UFUNCTION						0x4

#define PLAYERNAME						0x0478							//APlayerState : public AInfo

#define CAMERACACHE						0x04D0							//APlayerCameraManager : public AActor									//0x04E0

#define CHARACTER						0x0488							//AController : public AActor
#define PLAYERCAMERAMANAGER				0x70			//0x0500		//APlayerController : public AController				
#define IDLEDISCONNECTENABLED			0x1049			//0x1551		//AOnlineAthenaPlayerController : public AAthenaPlayerController

#define SPACEBASESARRAY					0x590							//UParticleSystemComponent : public UPrimitiveComponent					//0x0588 Not sure

#define ACTIVEHULLDAMAGEZONES			0x04B8							//AHullDamage : public AActor

#define DISPLAYNAME						0x0898							//AFauna : public AAICreatureCharacter

#define USKELETALMESHCOMPONENT			0x0550			//?																						//Inconclusive
#define SWIMMINGCREATURETYPE			0x5C			//?				//ASwimmingCreaturePawn : public APawn									//0x05BC

#define LOCALISABLENAME					0x80							//FAIEncounterSpecification

#define TITLE							0x0028							//UItemDescList : public UDataAsset

#define DESC							0x04C8							//AItemInfo : public AActor

#define	CURRENTLYWIELDEDITEM			0x0298							//UWieldedItemComponent : public USceneComponent

#define VELOCITY						0x10							//FWeaponProjectileParams

#define PROJECTILEMAXIMUMRANGE			0x0054							//FProjectileWeaponParameters
#define AMMOPARAMS						0x18			//0x0070		//FProjectileWeaponParameters

#define WEAPONPARAMETERS				0x0848							//AProjectileWeapon : public ASkeletalMeshWieldableItem

#define WORLDSPACECAMERAPOSITION		0x48							//FWorldMapIslandDataCaptureParams										//0x0018 Might be wrong

#define WORLDMAPDATA					0x40							//UIslandDataAssetEntry : public UDataAsset
#define LOCALISEDNAME					0x68			//0x00B0		//UIslandDataAssetEntry : public UDataAsset

#define ISLANDDATAENTRIES				0x0048							//UIslandDataAsset : public UDataAsset

#define ISLANDDATAASSET					0x04E8							//AIslandService : public AActor

#define INITIALDOORMESHLOCATION			0x05CC							//ASlidingDoor : public ADoor

#define OUTERDOOR						0x1090							//APuzzleVault : public AInteractableObject

#define PLAYERS							0x20							//FCrew
#define PLAYERS_						0x50							//FCrew																	//Addition

#define CREWS							0x0638							//ACrewService : public AActor

#define KRAKEN							0x05B8							//AKrakenService : public AActor

#define WINDSERVICE						0x0658							//AAthenaGameState : public AServiceProviderGameState
//... Carry on in same class

//Pay attention to the falling Hierarchy... (UObject ->) AActor -> APawn -> ACharacter -> AAthenaCharacter -> AAthenaPlayerCharacter
#define PLAYERSTATE						0x468 			//0x0490		//APawn : public AActor													//Maybe summand because of "struct FActorTickFunction {PrimaryActorTick // 0x0028}"
#define CONTROLLER						0x10			//0x04A8		//APawn : public AActor
#define MESH							0x28			//0x04D8		//ACharacter : public APawn
#define CHARACTERMOVEMENT				-				//0x04E0		//ACharacter : public APawn
#define WIELDEDITEMCOMPONENT			0x3C8			//0x08B0		//AAthenaCharacter : public ACharacter
#define HEALTHCOMPONENT					0x20			//0x08D8		//AAthenaCharacter : public ACharacter
#define DROWNINGCOMPONENT				0x428			//0x0D08		//AAthenaPlayerCharacter : public AAthenaCharacter

#define PERSISTENTLEVEL					0x0030							//UWorld : public UObject
#define GAMESTATE						0x20			//0x0058		//UWorld : public UObject
#define LEVELS							0xF0			//0x0150		//UWorld : public UObject	
#define CURRENTLEVEL					0x50			//0x01B0		//UWorld : public UObject
#define GAMEINSTANCE					0x8				//0x01C0		//UWorld : public UObject

//UNKNOWN DATA
//Define Name							//Address		//Static		//Hierarchy Top															//Result
#define _FOV							0x10			//0x0028		//struct FMinimalViewInfo

#define _POV							0xC				//0x0010		//struct FCameraCacheEntry		

#define KEYNAME							0x18			//?				//?																		//?

#define PLAYERCONTROLLER				0x30							//UPlayer : public UObject

#define LOCALPLAYER						0x0038							//UGameInstance : public UObject

#define AACTORS							0xA0							//ULevel : public UObject