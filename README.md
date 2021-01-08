# Poseidon

A WIP **Mod Menu** for *(...)*. It's so WIP that it crashes the when certain buttons are pressed, so don't use it right now. 

I will also add installation and user guidance once I believe the menu is ready for deployment.

## Features

* ESP
* Aim assistant for the cannon (Bot would get you banned)
* Information on ships, AI and other players

## Description

### Injection Method

This mod menu uses a basic *CreateRemoteThread* respectively *LoadLibrary* injection. There is no client-side anti-cheat system in place. So this method is completely safe to use; No need to overcomplicate stuff.

## How To Edit Wrong Memory References

When something (or everything) does not work after a patch of the game, it is mostly due to a changed memory offset. If that is the case, follow these steps:

**1** Find the most recent [SDK](https://github.com/pubgsdk) for the game.

**2** Identify the error in the mod menu's SDK; For example:
```
char pad4[0x3B8]; // 0x04E8 (HEX)
UWieldedItemComponent* WieldedItemComponent; // 0x08A0 (HEX)
```

**3** In the [SDK](https://github.com/pubgsdk) it says the memory address should be 0x08B0 instead of 0x08A0:
```
class UWieldedItemComponent* WieldedItemComponent; // 0x08B0 (HEX)
```

**4** We need to adjust the "char pad4[0x3C8]; // 0x04E8". The comment says we currently are at adress 0x04E8. Insted of adding up to 0x08A0, we need add up to 0x08B0:
```
0x04E8 + 0x03B8 = 0x08A0 //How it was
0x04E8 + 0x03C8 = 0x08B0 //How it needs to be
```

**5** Modify the "char pad4[0x3C8]; // 0x04E8" with the new summand that we just got:
```
char pad4[0x3B8]; // 0x04E8
UWieldedItemComponent* WieldedItemComponent; // 0x08A0 (HEX)

To:

char pad4[0x3C8]; // 0x04E8
UWieldedItemComponent* WieldedItemComponent; // 0x08B0 (HEX)
```

**6** You NEED to adjust the following addresses too. After every object you have defined, the memory also jumps by 08 (HEX); Meaning your next pad starts the summand from before later:
```
char pad4[0x3C8]; // 0x04E8
UWieldedItemComponent* WieldedItemComponent; // 0x08B0 (HEX)

char pad5[0x3C8]; //Starting point of next pad is 0x08B0 + 08 = 0x08B8, not 0x08A8 as it was before
```