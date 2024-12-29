#include "Bomb.h"

#include <engine/shared/config.h>
#include <game/server/player.h>
#include <game/server/entities/character.h>

#define GAME_TYPE_NAME "ZombieMode"

CGameControllerZombie::CGameControllerZombie(class CGameContext *pGameServer)
    : IGameController(pGameServer)
{
    m_pGameType = GAME_TYPE_NAME;
    m_RoundStartTick = 0;
    m_ZombieInitialized = false;
}

void CGameControllerZombie::OnCharacterSpawn(CCharacter *pChr)
{
    int ClientId = pChr->GetPlayer()->GetCid();
    IGameController::OnCharacterSpawn(pChr);

    // Базовые параметры персонажей
    pChr->SetArmor(10);
    pChr->GiveWeapon(WEAPON_GRENADE, true); // Ракетница вместо молотка
    pChr->SetWeapon(WEAPON_GRENADE);

    if (m_aPlayers[ClientId].m_Zombie)
        SetZombieSkin(pChr->GetPlayer());
    else
        SetHumanSkin(pChr->GetPlayer());
}

void CGameControllerZombie::OnPlayerConnect(CPlayer *pPlayer)
{
    IGameController::OnPlayerConnect(pPlayer);

    char aBuf[128];
    str_format(aBuf, sizeof(aBuf), "'%s' зашел в зомби-режим.", Server()->ClientName(pPlayer->GetCid()));
    GameServer()->SendChat(-1, TEAM_ALL, aBuf);
}

void CGameControllerZombie::Tick()
{
    IGameController::Tick();

    // Инициализация раунда
    if (!m_ZombieInitialized && AmountOfPlayers(STATE_ALIVE) > 1)
    {
        StartZombieRound();
        m_RoundStartTick = Server()->Tick();
        m_ZombieInitialized = true;
    }

    // Проверка условий победы
    if (m_ZombieInitialized)
    {
        int AliveHumans = AmountOfHumans();
        if (Server()->Tick() >= m_RoundStartTick + SERVER_TICK_SPEED * 180 && AliveHumans > 0) // 3 минуты
        {
            GameServer()->SendChat(-1, TEAM_ALL, "Выжившие победили!");
            EndRound();
        }
        else if (AliveHumans == 0)
        {
            GameServer()->SendChat(-1, TEAM_ALL, "Зомби победили!");
            EndRound();
        }
    }
}

void CGameControllerZombie::OnCharacterDeath(CCharacter *pVictim, CPlayer *pKiller, int Weapon)
{
    int VictimId = pVictim->GetPlayer()->GetCid();

    // Если убит человек — превращается в зомби
    if (!m_aPlayers[VictimId].m_Zombie)
    {
        m_aPlayers[VictimId].m_Zombie = true;
        GameServer()->SendChat(-1, TEAM_ALL, "'%s' превратился в зомби!", Server()->ClientName(VictimId));

        // Переспавним игрока как зомби
        GameServer()->GetPlayer(VictimId)->Respawn();
    }
}

void CGameControllerZombie::StartZombieRound()
{
    // Случайный выбор первого зомби
    int PlayerCount = Server()->MaxClients();
    std::vector<int> HumanPlayers;

    for (int i = 0; i < PlayerCount; ++i)
    {
        CPlayer *pPlayer = GameServer()->GetPlayer(i);
        if (pPlayer && !pPlayer->m_Zombie)
            HumanPlayers.push_back(i);
    }

    if (!HumanPlayers.empty())
    {
        int FirstZombie = HumanPlayers[rand() % HumanPlayers.size()];
        m_aPlayers[FirstZombie].m_Zombie = true;
        GameServer()->SendChat(-1, TEAM_ALL, "'%s' стал первым зомби!", Server()->ClientName(FirstZombie));

        // Респавним первого зомби
        GameServer()->GetPlayer(FirstZombie)->Respawn();
    }
}

void CGameControllerZombie::SetZombieSkin(CPlayer *pPlayer)
{
    // Пример кастомного скина для зомби
    pPlayer->SetCustomSkin("skeleton");
}

void CGameControllerZombie::SetHumanSkin(CPlayer *pPlayer)
{
    // Пример кастомного скина для человека
    pPlayer->SetCustomSkin("default");
}

int CGameControllerZombie::AmountOfHumans() const
{
    int Count = 0;
    for (int i = 0; i < Server()->MaxClients(); ++i)
    {
        if (GameServer()->GetPlayer(i) && !m_aPlayers[i].m_Zombie)
            ++Count;
    }
    return Count;
}

int CGameControllerZombie::AmountOfPlayers(int State) const
{
    int Count = 0;
    for (int i = 0; i < Server()->MaxClients(); ++i)
    {
        CPlayer *pPlayer = GameServer()->GetPlayer(i);
        if (pPlayer && pPlayer->GetTeam() == TEAM_SPECTATORS && pPlayer->m_PlayerFlags & State)
            ++Count;
    }
    return Count;
}
