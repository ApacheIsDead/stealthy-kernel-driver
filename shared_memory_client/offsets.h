#pragma once

/*namespace offsets
{
    constexpr auto iViewMatrix = 0x17DFD0;
    constexpr auto iLocalPlayer = 0x0018AC00;
    constexpr auto iEntityList = 0x00191FCC;

    constexpr auto vHead = 0x4;
    constexpr auto iTeam = 0x30C;
    constexpr auto isDead = 0x0318;
    constexpr auto pYaw = 0x34;
    constexpr auto vFeet = 0x28;
    constexpr auto pHealth = 0xEC;
    constexpr auto pPitch = 0x38;
}*/


struct Offsets {
    //offsets.cs
    const uintptr_t dwEntityList = 0x1A1F730;
    const uintptr_t dwLocalPlayerPawn = 0x1874050;

    //client_dll.cs
    const uintptr_t m_Glow = 0xC00;
    const uintptr_t m_iGlowType = 0x30;
    const uintptr_t m_glowColorOverride = 0x40;
    const uintptr_t m_bGlowing = 0x51;
    const uintptr_t m_iHealth = 0x344;
    const uintptr_t m_iTeamNum = 0x3E3;
    const uintptr_t m_hPlayerPawn = 0x814;
}Offsets;