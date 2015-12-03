/*
* Copyright (C) 2012-2014 JadeCore <http://www.pandashan.com/>
* Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
* Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation; either version 2 of the License, or (at your
* option) any later version.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
*
* You should have received a copy of the GNU General Public License along
* with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "Common.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Player.h"
#include "PetBattle.h"
#include "WildBattlePet.h"
#include "AchievementMgr.h"

void WorldSession::SendPetBattleJournal()
{
    if (!m_Player || !m_Player->IsInWorld())
        return;

    std::vector<BattlePet::Ptr>     l_Pets              = m_Player->GetBattlePets();
    uint32                          l_UnlockedSlotCount = m_Player->GetUnlockedPetBattleSlot();
    BattlePet::Ptr                * l_PetSlots          = m_Player->GetBattlePetCombatTeam();

    WorldPacket l_Packet(SMSG_BATTLE_PET_JOURNAL, 15 * 1024);

    l_Packet << uint16(0);                                                                                  ///< 
    l_Packet << uint32(3);                                                                                  ///< Slots count
    l_Packet << uint32(l_Pets.size());                                                                      ///< Pets count

    for (uint32 l_I = 0; l_I < 3; l_I++)
    {
        uint64 l_Guid = 0;

        if (l_PetSlots[l_I])
            l_Guid = l_PetSlots[l_I]->JournalID;

        l_Packet.appendPackGUID(l_Guid);                                                                    ///< BattlePetGUID
        l_Packet << uint32(0);                                                                              ///< CollarID
        l_Packet << uint8(l_I);                                                                             ///< SlotIndex
        l_Packet.WriteBit(!((l_I + 1) <= l_UnlockedSlotCount));                                             ///< Locked
        l_Packet.FlushBits();
    }

    for (std::vector<BattlePet::Ptr>::iterator l_It = l_Pets.begin(); l_It != l_Pets.end(); ++l_It)
    {
        BattlePet::Ptr l_Pet = (*l_It);

        uint64 l_Guid = l_Pet->JournalID;
        bool l_HasOwnerInfo = false;
        BattlePetSpeciesEntry const* l_SpeciesInfo = sBattlePetSpeciesStore.LookupEntry(l_Pet->Species);

        l_Pet->UpdateStats();

        l_Packet.appendPackGUID(l_Guid);                                                                    ///< BattlePetGUID
        l_Packet << uint32(l_Pet->Species);                                                                 ///< SpeciesID
        l_Packet << uint32(l_SpeciesInfo ? l_SpeciesInfo->entry : 0);                                       ///< CreatureID
        l_Packet << uint32(l_Pet->DisplayModelID);                                                          ///< DisplayID
        l_Packet << uint16(l_Pet->Breed);                                                                   ///< BreedID
        l_Packet << uint16(l_Pet->Level);                                                                   ///< Level
        l_Packet << uint16(l_Pet->XP);                                                                      ///< Xp
        l_Packet << uint16(l_Pet->Flags);                                                                   ///< BattlePetDBFlags
        l_Packet << int32(l_Pet->InfoPower);                                                                ///< Power
        l_Packet << int32(l_Pet->Health > l_Pet->InfoMaxHealth ? l_Pet->InfoMaxHealth : l_Pet->Health);     ///< Health
        l_Packet << int32(l_Pet->InfoMaxHealth);                                                            ///< MaxHealth
        l_Packet << int32(l_Pet->InfoSpeed);                                                                ///< Speed
        l_Packet << uint8(l_Pet->Quality);                                                                  ///< BreedQuality

        l_Packet.WriteBits(l_Pet->Name.length(), 7);                                                        ///< CustomName
        l_Packet.WriteBit(l_HasOwnerInfo);                                                                  ///< HasOwnerInfo
        l_Packet.WriteBit(false);                                                                           ///< NoRename
        l_Packet.FlushBits();

        l_Packet.WriteString(l_Pet->Name);

        if (l_HasOwnerInfo)
        {
            uint64 l_OwnerGUID = 0;

            l_Packet.appendPackGUID(l_OwnerGUID);                                                           ///< Guid
            l_Packet << uint32(g_RealmID);                                                                  ///< PlayerVirtualRealm
            l_Packet << uint32(g_RealmID);                                                                  ///< PlayerNativeRealm
        }
    }

    l_Packet.WriteBit(true);                                                                                ///< HasJournalLock
    l_Packet.FlushBits();

    SendPacket(&l_Packet);
}

void WorldSession::SendPetBattleJournalBattleSlotUpdate()
{
    if (!m_Player || !m_Player->IsInWorld())
        return;

    uint32                          l_UnlockedSlotCount = m_Player->GetUnlockedPetBattleSlot();
    BattlePet::Ptr                * l_PetSlots          = m_Player->GetBattlePetCombatTeam();

    if (l_UnlockedSlotCount > 0)
        m_Player->SetFlag(PLAYER_FIELD_PLAYER_FLAGS, PLAYER_FLAGS_HAS_BATTLE_PET_TRAINING);

    WorldPacket l_Packet(SMSG_PET_BATTLE_SLOT_UPDATES, 100);

    l_Packet << uint32(l_UnlockedSlotCount);    ///< Slots count

    for (uint32 l_I = 0; l_I < 3; l_I++)
    {
        uint64 l_Guid = 0;

        if (l_PetSlots[l_I])
            l_Guid = l_PetSlots[l_I]->JournalID;

        l_Packet.appendPackGUID(l_Guid);                                                                    ///< BattlePetGUID
        l_Packet << uint32(0);                                                                              ///< CollarID
        l_Packet << uint8(l_I);                                                                             ///< SlotIndex
        l_Packet.WriteBit(!((l_I + 1) <= l_UnlockedSlotCount));                                             ///< Locked
        l_Packet.FlushBits();
    }

    l_Packet.WriteBit(true);                    ///< NewSlotUnlocked
    l_Packet.WriteBit(false);                   ///< AutoSlotted
    l_Packet.FlushBits();

    SendPacket(&l_Packet);
}

void WorldSession::SendPetBattleRequestFailed(uint8 p_Reason)
{
    WorldPacket l_Packet(SMSG_PETBATTLE_REQUEST_FAILED, 2);
    l_Packet << p_Reason;

    SendPacket(&l_Packet);
}

void WorldSession::SendPetBattleFullUpdate(PetBattle* battle)
{
    WorldPacket packet(SMSG_PETBATTLE_FULL_UPDATE, 1000);

    ObjectGuid l_Guid = 0;

    if (battle->BattleType == PETBATTLE_TYPE_PVE)
        l_Guid = battle->Teams[1]->OwnerGuid;

    uint32 l_NpcDisplayID = 0;
    uint32 l_NpcCreatureID = 0;
    uint16 l_WaitingForFrontPetsMaxSecs = 30;
    uint16 l_PvpMaxRoundTime = 30;
    uint8 l_CurPetBattleState = 1;
    bool l_CanAwardXP = true;
    bool l_IsPVP = false;

    for (uint32 l_TeamID = 0; l_TeamID < MAX_PETBATTLE_TEAM; l_TeamID++)
    {
        bool l_IsPVP = battle->BattleType != PETBATTLE_TYPE_PVE;
        uint16 l_PvpMaxRoundTime = l_IsPVP ? 30 : 0;

        ObjectGuid l_OwnerGuid = battle->Teams[l_TeamID]->OwnerGuid;

        if (battle->BattleType == PETBATTLE_TYPE_PVE && l_TeamID == PETBATTLE_PVE_TEAM_ID)
            l_OwnerGuid = 0;

        packet.appendPackGUID(l_OwnerGuid);
        packet << uint32(battle->Teams[l_TeamID]->GetCatchAbilityID());
        packet << uint32(l_TeamID == PETBATTLE_TEAM_1 ? 5 : 2); // dwordc
        packet << uint16(l_PvpMaxRoundTime);

        if (l_TeamID == PETBATTLE_TEAM_1)
            packet << uint8(battle->Teams[l_TeamID]->ActivePetID);
        else
            packet << uint8(battle->Teams[l_TeamID]->ActivePetID - (l_TeamID == PETBATTLE_TEAM_2 ? MAX_PETBATTLE_SLOTS : 0));
        packet << uint8(6);

        packet.WriteBits(battle->Teams[l_TeamID]->TeamPetCount, 2);
        packet.FlushBits();

        for (uint32 l_PetID = 0; l_PetID < battle->Teams[l_TeamID]->TeamPetCount; l_PetID++)
        {
            std::shared_ptr<BattlePetInstance> l_Pet = battle->Teams[l_TeamID]->TeamPets[l_PetID];

            ObjectGuid l_JournalID = (battle->BattleType == PETBATTLE_TYPE_PVE && l_TeamID == PETBATTLE_PVE_TEAM_ID) ? 0 : l_Pet->JournalID;

            uint32 l_StatesCount = 0;
            uint32 l_AbilityCount = 0;

            for (uint32 l_State = 0; l_State < NUM_BATTLEPET_STATES; l_State++)
                if (l_State == 20 ||l_State == 19 ||l_State == 18 ||l_State == 40 || l_State == 49)
                    l_StatesCount++;

            for (uint32 l_Ability = 0; l_Ability < MAX_PETBATTLE_ABILITIES; l_Ability++)
                if (l_Pet->Abilities[l_Ability])
                    l_AbilityCount++;


            packet.appendPackGUID(l_JournalID);
            packet << uint32(l_Pet->Species);
            packet << uint32(l_Pet->DisplayModelID);
            packet << uint32(0);
            packet << uint16(l_Pet->Level);
            packet << uint16(l_Pet->XP);
            packet << uint32(l_Pet->Health);
            packet << uint32(l_Pet->InfoMaxHealth);
            packet << uint32(l_Pet->InfoPower);
            packet << uint32(l_Pet->InfoSpeed);
            packet << uint32(0);
            packet << uint16(l_Pet->Quality);
            packet << uint16(l_Pet->Flags & ~PETBATTLE_FLAG_CAPTURED);
            packet << uint8(l_PetID);
            packet << uint32(l_AbilityCount);
            packet << uint32(0); 
            packet << uint32(l_StatesCount);

            for (uint32 l_CurrentAbilitySlot = 0; l_CurrentAbilitySlot < MAX_PETBATTLE_ABILITIES; l_CurrentAbilitySlot++)
            {
                if (l_Pet->Abilities[l_CurrentAbilitySlot])
                {
                    packet << uint32(l_Pet->Abilities[l_CurrentAbilitySlot]);
                    packet << uint16(0);
                    packet << uint16(0);
                    packet << uint8(l_CurrentAbilitySlot);
                    packet << uint8(l_Pet->ID);
                }
            }

            for (int l_State = 0; l_State < NUM_BATTLEPET_STATES; l_State++)
                if (l_State == 20 || l_State == 19 || l_State == 18 || l_State == 40 || l_State == 49)
                    packet << uint32(l_State) << int32(l_Pet->States[l_State]);

            packet.WriteBits(l_Pet->Name.length(), 7);
            packet.FlushBits();

            packet.WriteString(l_Pet->Name);
        }
    }

    for (uint32 l_I = 0; l_I < 3; l_I++)
    {
        packet << uint32(0);
        packet << uint32(0);
    }

    packet << uint16(l_WaitingForFrontPetsMaxSecs);
    packet << uint16(l_PvpMaxRoundTime);
    packet << uint32(battle->Turn); ///< Turn ? always 0 here
    packet << uint32(l_NpcCreatureID);
    packet << uint32(l_NpcDisplayID);
    packet << uint8(l_CurPetBattleState);
    packet << uint8(battle->GetForfeitHealthPenalityPct());

    packet.WriteBit(l_IsPVP);
    packet.WriteBit(l_CanAwardXP);
    packet.FlushBits();

    packet.appendPackGUID(l_Guid);

    SendPacket(&packet);
}

void WorldSession::SendPetBattleFirstRound(PetBattle* p_Battle)
{
    WorldPacket l_Packet(SMSG_PETBATTLE_FIRST_ROUND, 100);

    l_Packet << uint32(p_Battle->Turn);
    l_Packet << uint8(p_Battle->RoundResult);
    l_Packet << uint32(p_Battle->RoundEvents.size());

    for (uint32 l_TeamID = 0; l_TeamID < MAX_PETBATTLE_TEAM; l_TeamID++)
    {
        l_Packet << uint8(p_Battle->Teams[l_TeamID]->GetTeamInputFlags());  ///< NextInputFlags
        l_Packet << uint8(p_Battle->Teams[l_TeamID]->GetTeamTrapFlags());   ///< NextTrapStatus
        l_Packet << uint16(0);                                              ///< RoundTimeSecs
    }

    l_Packet << uint32(0);

    for (std::list<PetBattleEvent>::iterator l_EventIt = p_Battle->RoundEvents.begin(); l_EventIt != p_Battle->RoundEvents.end(); ++l_EventIt)
    {
        l_Packet << uint32(l_EventIt->AbilityEffectID);
        l_Packet << uint16(l_EventIt->Flags);
        l_Packet << uint16(l_EventIt->BuffTurn);    ///< can be swap down
        l_Packet << uint16(l_EventIt->RoundTurn);   ///< can be swap up
        l_Packet << uint8(l_EventIt->EventType);
        l_Packet << uint8(l_EventIt->SourcePetID);
        l_Packet << uint8(l_EventIt->StackDepth);
        l_Packet << uint32(l_EventIt->Updates.size());

        for (std::list<PetBattleEventUpdate>::iterator l_UpdateIt = l_EventIt->Updates.begin(); l_UpdateIt != l_EventIt->Updates.end(); ++l_UpdateIt)
        {
            l_Packet.WriteBits(l_UpdateIt->UpdateType, 3);
            l_Packet.FlushBits();

            l_Packet << uint8(l_UpdateIt->TargetPetID);

            if (l_UpdateIt->UpdateType == PETBATTLE_EVENT_UPDATE_BUFF)
            {
                l_Packet << uint32(l_UpdateIt->Buff.ID);
                l_Packet << uint32(l_UpdateIt->Buff.AbilityID);
                l_Packet << int32(l_UpdateIt->Buff.Duration);
                l_Packet << uint32(l_UpdateIt->Buff.Turn);
            }

            if (l_UpdateIt->UpdateType == PETBATTLE_EVENT_UPDATE_STATE)
            {
                l_Packet << uint32(l_UpdateIt->State.ID);
                l_Packet << int32(l_UpdateIt->State.Value);
            }

            if (l_UpdateIt->UpdateType == PETBATTLE_EVENT_UPDATE_HEALTH)
                l_Packet << int32(l_UpdateIt->Health);

            if (l_UpdateIt->UpdateType == PETBATTLE_EVENT_UPDATE_SPEED)
                l_Packet << int32(l_UpdateIt->Speed);

            if (l_UpdateIt->UpdateType == PETBATTLE_EVENT_UPDATE_TRIGGER)
                l_Packet << uint32(l_UpdateIt->TriggerAbilityId);

            if (l_UpdateIt->UpdateType == PETBATTLE_EVENT_UPDATE_ABILITY_CHANGE)
            {
                l_Packet << uint32(0);
                l_Packet << uint32(0);
                l_Packet << uint32(0);
            }

            if (l_UpdateIt->UpdateType == PETBATTLE_EVENT_UPDATE_NPC_EMOTE)
                l_Packet << int32(l_UpdateIt->NpcEmote.BroadcastTextID);
        }
    }

    l_Packet.WriteBits(0, 3);
    l_Packet.FlushBits();

    SendPacket(&l_Packet);
}

void WorldSession::SendPetBattleFinalRound(PetBattle* p_Battle)
{
    WorldPacket l_Packet(SMSG_PETBATTLE_FINAL_ROUND, 200);

    l_Packet.WriteBit(p_Battle->CombatResult == PETBATTLE_RESULT_ABANDON);
    l_Packet.WriteBit(p_Battle->BattleType != PETBATTLE_TYPE_PVE);

    for (size_t l_TeamId = 0; l_TeamId < MAX_PETBATTLE_TEAM; ++l_TeamId)
        l_Packet.WriteBit(p_Battle->WinnerTeamId == l_TeamId);

    l_Packet.FlushBits();

    for (size_t l_TeamId = 0; l_TeamId < MAX_PETBATTLE_TEAM; ++l_TeamId)
        l_Packet << uint32(0);

    l_Packet << uint32(p_Battle->TotalPetCount);

    for (uint32 l_CurrentPetslot = 0; l_CurrentPetslot < (MAX_PETBATTLE_TEAM * MAX_PETBATTLE_SLOTS); l_CurrentPetslot++)
    {
        if (!p_Battle->Pets[l_CurrentPetslot])
            continue;

        l_Packet.appendPackGUID(0);
        l_Packet << uint16(p_Battle->Pets[l_CurrentPetslot]->Level);
        l_Packet << uint16(p_Battle->Pets[l_CurrentPetslot]->XP);
        l_Packet << int32(p_Battle->Pets[l_CurrentPetslot]->Health);
        l_Packet << int32(p_Battle->Pets[l_CurrentPetslot]->InfoMaxHealth);
        l_Packet << uint16(p_Battle->Pets[l_CurrentPetslot]->OldLevel);
        l_Packet << uint8(p_Battle->Pets[l_CurrentPetslot]->ID);
        
        l_Packet.WriteBit(p_Battle->CatchedPetId == l_CurrentPetslot);
        l_Packet.WriteBit(p_Battle->CatchedPetId == l_CurrentPetslot);
        l_Packet.WriteBit(p_Battle->Pets[l_CurrentPetslot]->OldXP != p_Battle->Pets[l_CurrentPetslot]->XP);
        l_Packet.WriteBit(p_Battle->FightedPets.find(l_CurrentPetslot) != p_Battle->FightedPets.end());
        l_Packet.FlushBits();
    }

    SendPacket(&l_Packet);
}

void WorldSession::SendPetBattleRoundResult(PetBattle* p_Battle)
{
    WorldPacket l_Packet(SMSG_PETBATTLE_ROUND_RESULT, 700);

    uint32 l_CooldownCount = 0;

    for (uint32 l_CurrentPetslot = 0; l_CurrentPetslot < (MAX_PETBATTLE_TEAM * MAX_PETBATTLE_SLOTS); l_CurrentPetslot++)
    {
        if (!p_Battle->Pets[l_CurrentPetslot])
            continue;

        for (uint32 l_AbilitySlot = 0; l_AbilitySlot < MAX_PETBATTLE_ABILITIES; l_AbilitySlot++)
            if (p_Battle->Pets[l_CurrentPetslot]->Cooldowns[l_AbilitySlot] != -1)
                l_CooldownCount++;
    }

    l_Packet << uint32(p_Battle->Turn);
    l_Packet << uint8(p_Battle->RoundResult);
    l_Packet << uint32(p_Battle->RoundEvents.size());

    for (uint32 l_TeamID = 0; l_TeamID < MAX_PETBATTLE_TEAM; l_TeamID++)
    {
        l_Packet << uint8(p_Battle->Teams[l_TeamID]->GetTeamInputFlags());
        l_Packet << uint8(p_Battle->Teams[l_TeamID]->GetTeamTrapFlags());
        l_Packet << uint16(0);
    }

    l_Packet << uint32(l_CooldownCount);

    for (std::list<PetBattleEvent>::iterator l_EventIt = p_Battle->RoundEvents.begin(); l_EventIt != p_Battle->RoundEvents.end(); ++l_EventIt)
    {
        l_Packet << uint32(l_EventIt->AbilityEffectID);
        l_Packet << uint16(l_EventIt->Flags);
        l_Packet << uint16(l_EventIt->BuffTurn);    ///< can be swap down
        l_Packet << uint16(l_EventIt->RoundTurn);   ///< can be swap up
        l_Packet << uint8(l_EventIt->EventType);
        l_Packet << uint8(l_EventIt->SourcePetID);
        l_Packet << uint8(l_EventIt->StackDepth);
        l_Packet << uint32(l_EventIt->Updates.size());

        for (std::list<PetBattleEventUpdate>::iterator l_UpdateIt = l_EventIt->Updates.begin(); l_UpdateIt != l_EventIt->Updates.end(); ++l_UpdateIt)
        {
            l_Packet.WriteBits(l_UpdateIt->UpdateType, 3);
            l_Packet.FlushBits();

            l_Packet << uint8(l_UpdateIt->TargetPetID);

            if (l_UpdateIt->UpdateType == PETBATTLE_EVENT_UPDATE_BUFF)
            {
                l_Packet << uint32(l_UpdateIt->Buff.ID);
                l_Packet << uint32(l_UpdateIt->Buff.AbilityID);
                l_Packet << int32(l_UpdateIt->Buff.Duration);
                l_Packet << uint32(l_UpdateIt->Buff.Turn);
            }

            if (l_UpdateIt->UpdateType == PETBATTLE_EVENT_UPDATE_STATE)
            {
                l_Packet << uint32(l_UpdateIt->State.ID);
                l_Packet << int32(l_UpdateIt->State.Value);
            }

            if (l_UpdateIt->UpdateType == PETBATTLE_EVENT_UPDATE_HEALTH)
                l_Packet << int32(l_UpdateIt->Health);

            if (l_UpdateIt->UpdateType == PETBATTLE_EVENT_UPDATE_SPEED)
                l_Packet << int32(l_UpdateIt->Speed);

            if (l_UpdateIt->UpdateType == PETBATTLE_EVENT_UPDATE_TRIGGER)
                l_Packet << uint32(l_UpdateIt->TriggerAbilityId);

            if (l_UpdateIt->UpdateType == PETBATTLE_EVENT_UPDATE_ABILITY_CHANGE)
            {
                l_Packet << uint32(0);
                l_Packet << uint32(0);
                l_Packet << uint32(0);
            }

            if (l_UpdateIt->UpdateType == PETBATTLE_EVENT_UPDATE_NPC_EMOTE)
                l_Packet << int32(l_UpdateIt->NpcEmote.BroadcastTextID);
        }
    }

    for (uint32 l_CurrentPetslot = 0; l_CurrentPetslot < (MAX_PETBATTLE_TEAM * MAX_PETBATTLE_SLOTS); l_CurrentPetslot++)
    {
        if (!p_Battle->Pets[l_CurrentPetslot])
            continue;

        for (uint32 l_AbilitySlot = 0; l_AbilitySlot < MAX_PETBATTLE_ABILITIES; l_AbilitySlot++)
        {
            if (p_Battle->Pets[l_CurrentPetslot]->Cooldowns[l_AbilitySlot] != -1)
            {
                l_Packet << uint32(p_Battle->Pets[l_CurrentPetslot]->Abilities[l_AbilitySlot]);
                l_Packet << uint16(p_Battle->Pets[l_CurrentPetslot]->Cooldowns[l_AbilitySlot]);
                l_Packet << uint16(0);
                l_Packet << uint8(l_AbilitySlot);
                l_Packet << uint8(p_Battle->Pets[l_CurrentPetslot]->ID);
            }
        }
    }

    l_Packet.WriteBits(p_Battle->RoundDeadPets.size(), 3);
    l_Packet.FlushBits();

    for (uint32 l_I = 0; l_I < p_Battle->RoundDeadPets.size(); l_I++)
        l_Packet << uint8(p_Battle->RoundDeadPets[l_I]);

    SendPacket(&l_Packet);
}

void WorldSession::SendPetBattleFinalizeLocation(PetBattleRequest* p_Request)
{
    WorldPacket l_Packet(SMSG_PETBATTLE_FINALIZE_LOCATION, 16 + 16 + 16 + 1 + 4 + 4);

    l_Packet << uint32(p_Request->LocationResult);
    l_Packet << p_Request->PetBattleCenterPosition[0];
    l_Packet << p_Request->PetBattleCenterPosition[1];
    l_Packet << p_Request->PetBattleCenterPosition[2];
    l_Packet << float(p_Request->BattleFacing);

    for (int l_I = 0; l_I < 2; l_I++)
        l_Packet << p_Request->TeamPosition[l_I][0] << p_Request->TeamPosition[l_I][1] << p_Request->TeamPosition[l_I][2];

    SendPacket(&l_Packet);
}

void WorldSession::SendPetBattleFinished(PetBattle* battle)
{
    WorldPacket packet(SMSG_PETBATTLE_FINISHED, 0);
    SendPacket(&packet);
}

//////////////////////////////////////////////////////////////////////////

void WorldSession::HandlePetBattleSetAbility(WorldPacket& p_RecvData)
{
    uint64 l_PetJournalID;
    uint32 l_Flag = 0;
    uint8 l_Action = 0;

    p_RecvData.readPackGUID(l_PetJournalID);
    p_RecvData >> l_Flag;

    l_Action = p_RecvData.ReadBits(2);  // 0 add flag, 2 remove it

    BattlePet::Ptr l_BattlePet = m_Player->GetBattlePet(l_PetJournalID);

    if (l_BattlePet)
    {
        if (l_BattlePet->Flags & l_Flag)
            l_BattlePet->Flags = l_BattlePet->Flags & ~l_Flag;
        else
            l_BattlePet->Flags |= l_Flag;
    }
}

void WorldSession::HandlePetBattleRename(WorldPacket& p_RecvData)
{
    DeclinedName    l_DeclinedNames;
    uint64          l_PetJournalID;
    bool            l_HaveDeclinedNames = false;
    uint32          l_NameLenght        = 0;
    std::string     l_Name;

    uint32 l_DeclinedNameLens[MAX_DECLINED_NAME_CASES];

    p_RecvData.readPackGUID(l_PetJournalID);
    l_NameLenght        = p_RecvData.ReadBits(7);
    l_HaveDeclinedNames = p_RecvData.ReadBit();

    l_Name = p_RecvData.ReadString(l_NameLenght);

    if (l_HaveDeclinedNames)
    {
        p_RecvData.ResetBitReading();

        for (size_t l_I = 0 ; l_I < MAX_DECLINED_NAME_CASES ; ++l_I)
            l_DeclinedNameLens[l_I] = p_RecvData.ReadBits(7);

        for (size_t l_I = 0; l_I < MAX_DECLINED_NAME_CASES; ++l_I)
            l_DeclinedNames.name[l_I] = p_RecvData.ReadString(l_DeclinedNameLens[l_I]);
    }

    PetNameInvalidReason l_NameInvalidReason = sObjectMgr->CheckPetName(l_Name);
    if (l_NameInvalidReason != PET_NAME_SUCCESS)
    {
        SendPetNameInvalid(l_NameInvalidReason, l_Name, (l_HaveDeclinedNames) ? &l_DeclinedNames : NULL);
        return;
    }

    uint32 l_TimeStamp = l_Name.empty() ? 0 : time(0);

    BattlePet::Ptr l_BattlePet = m_Player->GetBattlePet(l_PetJournalID);

    if (l_BattlePet)
    {
        l_BattlePet->Name           = l_Name;
        l_BattlePet->NameTimeStamp  = l_TimeStamp;

        if (l_HaveDeclinedNames)
        {
            for (size_t l_I = 0; l_I < MAX_DECLINED_NAME_CASES; ++l_I)
                l_BattlePet->DeclinedNames[l_I] = l_DeclinedNames.name[l_I];
        }
    }

    m_Player->SetUInt32Value(UNIT_FIELD_BATTLE_PET_COMPANION_NAME_TIMESTAMP, l_TimeStamp);

    Creature* l_Creature = m_Player->GetSummonedBattlePet();

    if (!l_Creature)
        return;

    if (l_Creature->GetUInt32Value(UNIT_FIELD_BATTLE_PET_COMPANION_GUID) == l_PetJournalID)
    {
        l_Creature->SetName(l_Name);
        l_Creature->SetUInt32Value(UNIT_FIELD_BATTLE_PET_COMPANION_NAME_TIMESTAMP, l_TimeStamp);
    }

    //TODO: add declined names
}

void WorldSession::HandleSummonCompanion(WorldPacket& recvData)
{
    uint64 l_JournalID;

    recvData.readPackGUID(l_JournalID);

    if (m_Player->GetSummonedBattlePet() && m_Player->GetSummonedBattlePet()->GetGuidValue(UNIT_FIELD_BATTLE_PET_COMPANION_GUID) == l_JournalID)
        m_Player->UnsummonCurrentBattlePetIfAny(false);
    else
    {
        m_Player->UnsummonCurrentBattlePetIfAny(false);
        m_Player->SummonBattlePet(l_JournalID);
    }
}

void WorldSession::HandlePetBattleCagePet(WorldPacket& recvData)
{
    ObjectGuid l_Guid;

    l_Guid[6] = recvData.ReadBit();
    l_Guid[5] = recvData.ReadBit();
    l_Guid[0] = recvData.ReadBit();
    l_Guid[3] = recvData.ReadBit();
    l_Guid[4] = recvData.ReadBit();
    l_Guid[7] = recvData.ReadBit();
    l_Guid[2] = recvData.ReadBit();
    l_Guid[1] = recvData.ReadBit();

    recvData.ReadByteSeq(l_Guid[4]);
    recvData.ReadByteSeq(l_Guid[1]);
    recvData.ReadByteSeq(l_Guid[5]);
    recvData.ReadByteSeq(l_Guid[3]);
    recvData.ReadByteSeq(l_Guid[0]);
    recvData.ReadByteSeq(l_Guid[6]);
    recvData.ReadByteSeq(l_Guid[7]);
    recvData.ReadByteSeq(l_Guid[2]);

    if (m_Player->GetGuidValue(PLAYER_FIELD_SUMMONED_BATTLE_PET_GUID) == l_Guid)
        m_Player->UnsummonCurrentBattlePetIfAny(false);


    PetBattle pet;
    //TODO: aync load
    //TODO: create item cage
}

void WorldSession::HandlePetBattleQueryName(WorldPacket& p_RecvData)
{
    uint64 l_UnitGuid;
    uint64 l_JournalGuid;

    p_RecvData.readPackGUID(l_UnitGuid);
    p_RecvData.readPackGUID(l_JournalGuid);

    Creature* l_Creature = Unit::GetCreature(*m_Player, l_UnitGuid);

    if (!l_Creature)
        return;

    WorldPacket l_Packet(SMSG_QUERY_PET_NAME_RESPONSE, 0x40);

    l_Packet.appendPackGUID(l_JournalGuid);
    l_Packet.WriteBit(l_Creature->GetName() ? true : false);

    if (l_Creature->GetName())
    {
        l_Packet.WriteBits(l_Creature->GetName() ? strlen(l_Creature->GetName()) : 0, 8);
        l_Packet.WriteBit(0);   // unk maybe declined names

        for (uint32 l_I = 0; l_I < 5; ++l_I)
            l_Packet.WriteBits(0, 7);
    }

    l_Packet.FlushBits();

    if (l_Creature->GetName())
    {
        l_Packet << uint32(l_Creature->GetUInt32Value(UNIT_FIELD_BATTLE_PET_COMPANION_NAME_TIMESTAMP));

        if (l_Creature->GetName())
            l_Packet.WriteString(l_Creature->GetName());
    }

    m_Player->GetSession()->SendPacket(&l_Packet);
}

void WorldSession::HandleBattlePetSetBattleSlot(WorldPacket& p_RecvData)
{
    uint64  l_PetJournalID;
    uint8   l_DestSlot = 0;

    p_RecvData.readPackGUID(l_PetJournalID);
    p_RecvData >> l_DestSlot;

    if (l_DestSlot >= MAX_PETBATTLE_SLOTS)
        return;

    BattlePet::Ptr   l_BattlePet = m_Player->GetBattlePet(l_PetJournalID);
    BattlePet::Ptr * l_PetSlots  = m_Player->GetBattlePetCombatTeam();
    
    if (l_BattlePet)
    {
        for (uint8 l_I = 0; l_I < MAX_PETBATTLE_SLOTS; ++l_I)
        {
            if (l_PetSlots[l_I] && l_PetSlots[l_I]->Slot == l_DestSlot)
                l_PetSlots[l_I]->Slot = l_BattlePet->Slot;
        }

        l_BattlePet->Slot = l_DestSlot;
    }

    m_Player->UpdateBattlePetCombatTeam();
    SendPetBattleJournalBattleSlotUpdate();
}

void WorldSession::HandlePetBattleRequestWild(WorldPacket& p_RecvData)
{
    if (!m_Player || !m_Player->IsInWorld())
        return;

    PetBattleRequest* l_BattleRequest = sPetBattleSystem->CreateRequest(m_Player->GetGUID());
    uint64 l_OpponentGuid;

    p_RecvData.readPackGUID(l_OpponentGuid);
    p_RecvData >> l_BattleRequest->LocationResult;
    p_RecvData >> l_BattleRequest->PetBattleCenterPosition[0];
    p_RecvData >> l_BattleRequest->PetBattleCenterPosition[1];
    p_RecvData >> l_BattleRequest->PetBattleCenterPosition[2];
    p_RecvData >> l_BattleRequest->BattleFacing;

    for (int l_CurrentTeamID = 0; l_CurrentTeamID < MAX_PETBATTLE_TEAM; l_CurrentTeamID++)
        p_RecvData >> l_BattleRequest->TeamPosition[l_CurrentTeamID][0] >> l_BattleRequest->TeamPosition[l_CurrentTeamID][1] >> l_BattleRequest->TeamPosition[l_CurrentTeamID][2];

    l_BattleRequest->RequestType    = PETBATTLE_TYPE_PVE;
    l_BattleRequest->OpponentGuid   = l_OpponentGuid;

    // Player can't be already in battle
    if (m_Player->_petBattleId)
    {
        SendPetBattleRequestFailed(PETBATTLE_REQUEST_ALREADY_IN_PETBATTLE);
        sPetBattleSystem->RemoveRequest(l_BattleRequest->RequesterGuid);
        return;
    }

    // Player can't be in combat
    if (m_Player->isInCombat())
    {
        SendPetBattleRequestFailed(PETBATTLE_REQUEST_ALREADY_IN_COMBAT);
        sPetBattleSystem->RemoveRequest(l_BattleRequest->RequesterGuid);
        return;
    }

    // Check positions
    for (size_t l_CurrentTeamID = 0; l_CurrentTeamID < MAX_PETBATTLE_TEAM; ++l_CurrentTeamID)
    {
        if (m_Player->GetMap()->getObjectHitPos(m_Player->GetPhaseMask(),l_BattleRequest->PetBattleCenterPosition[0],        l_BattleRequest->PetBattleCenterPosition[1],        l_BattleRequest->PetBattleCenterPosition[2],
                                                                        l_BattleRequest->TeamPosition[l_CurrentTeamID][0],  l_BattleRequest->TeamPosition[l_CurrentTeamID][1],  l_BattleRequest->TeamPosition[l_CurrentTeamID][2],
                                                                        l_BattleRequest->TeamPosition[l_CurrentTeamID][0],  l_BattleRequest->TeamPosition[l_CurrentTeamID][1],  l_BattleRequest->TeamPosition[l_CurrentTeamID][2], 0.0f))
        {
            SendPetBattleRequestFailed(PETBATTLE_REQUEST_GROUND_NOT_ENOUGHT_SMOOTH);
            sPetBattleSystem->RemoveRequest(l_BattleRequest->RequesterGuid);
            return;
        }
    }

    // Wild should be for PetBattle and Player able to interact with it
    Creature* l_WildPet = m_Player->GetNPCIfCanInteractWith(l_BattleRequest->OpponentGuid, UNIT_NPC_FLAG_PETBATTLE);
    if (!l_WildPet)
    {
        SendPetBattleRequestFailed(PETBATTLE_REQUEST_INVALID_TARGET);
        sPetBattleSystem->RemoveRequest(l_BattleRequest->RequesterGuid);
        return;
    }

    // Ok, Despawn wild
    l_WildPet->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED | UNIT_FLAG_IMMUNE_TO_PC); // Immuned to all
    l_WildPet->SetTarget(m_Player->GetGUID());
    l_WildPet->SetControlled(true, UNIT_STATE_ROOT);

    m_Player->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED | UNIT_FLAG_IMMUNE_TO_NPC); // Immuned only to NPC
    m_Player->SetTarget(l_WildPet->GetGUID());

    BattlePetInstance::Ptr  l_PlayerPets[MAX_PETBATTLE_SLOTS];
    BattlePetInstance::Ptr  l_WildBattlePet;
    size_t                  l_PlayerPetCount = 0;
    PetBattle*              l_Battle;
    uint32                  l_ErrorCode = PETBATTLE_REQUEST_FAILED;

    // Temporary pet buffer
    for (size_t l_CurrentPetSlot = 0; l_CurrentPetSlot < MAX_PETBATTLE_SLOTS; ++l_CurrentPetSlot)
        l_PlayerPets[l_CurrentPetSlot] = 0;

    Creature* l_Wild = sObjectAccessor->GetCreature(*m_Player, l_BattleRequest->OpponentGuid);

    if (!l_Wild)
    {
        l_ErrorCode = PETBATTLE_REQUEST_INVALID_TARGET;

        m_Player->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED | UNIT_FLAG_IMMUNE_TO_NPC);
        m_Player->SetRooted(false);

        if (l_Wild)
        {
            l_Wild->SetTarget(0);
            l_Wild->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED | UNIT_FLAG_IMMUNE_TO_PC);
            l_Wild->SetControlled(false, UNIT_STATE_ROOT);
        }

        for (size_t l_CurrentPetID = 0; l_CurrentPetID < MAX_PETBATTLE_SLOTS; ++l_CurrentPetID)
            if (l_PlayerPets[l_CurrentPetID])
                l_PlayerPets[l_CurrentPetID] = BattlePetInstance::Ptr();

        l_WildBattlePet = BattlePetInstance::Ptr();

        SendPetBattleRequestFailed(l_ErrorCode);
        sPetBattleSystem->RemoveRequest(l_BattleRequest->RequesterGuid);
        return;
    }

    // Load player pets
    BattlePet::Ptr * l_PetSlots = m_Player->GetBattlePetCombatTeam();

    for (size_t l_CurrentPetSlot = 0; l_CurrentPetSlot < MAX_PETBATTLE_SLOTS; ++l_CurrentPetSlot)
    {
        if (!l_PetSlots[l_CurrentPetSlot])
            continue;

        if (l_PlayerPetCount >= MAX_PETBATTLE_SLOTS || l_PlayerPetCount >= m_Player->GetUnlockedPetBattleSlot())
            break;

        l_PlayerPets[l_PlayerPetCount] = BattlePetInstance::Ptr(new BattlePetInstance());
        l_PlayerPets[l_PlayerPetCount]->CloneFrom(l_PetSlots[l_CurrentPetSlot]);
        l_PlayerPets[l_PlayerPetCount]->Slot                = l_PlayerPetCount;
        l_PlayerPets[l_PlayerPetCount]->OriginalBattlePet   = l_PetSlots[l_CurrentPetSlot];

        ++l_PlayerPetCount;
    }

    // Check player team
    if (!l_PlayerPetCount)
    {
        l_ErrorCode = PETBATTLE_REQUEST_NEED_PET_IN_SLOTS;

        m_Player->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED | UNIT_FLAG_IMMUNE_TO_NPC);
        m_Player->SetRooted(false);

        if (l_Wild)
        {
            l_Wild->SetTarget(0);
            l_Wild->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED | UNIT_FLAG_IMMUNE_TO_PC);
            l_Wild->SetControlled(false, UNIT_STATE_ROOT);
        }

        for (size_t l_CurrentPetID = 0; l_CurrentPetID < MAX_PETBATTLE_SLOTS; ++l_CurrentPetID)
            if (l_PlayerPets[l_CurrentPetID])
                l_PlayerPets[l_CurrentPetID] = BattlePetInstance::Ptr();

        l_WildBattlePet = BattlePetInstance::Ptr();

        SendPetBattleRequestFailed(l_ErrorCode);
        sPetBattleSystem->RemoveRequest(l_BattleRequest->RequesterGuid);
        return;
    }

    ////////////////////////////////
    ////////////////////////////////
    // Wild should be wild
    if (!sWildBattlePetMgr->IsWildPet(l_Wild))
    {
        l_ErrorCode = PETBATTLE_REQUEST_INVALID_TARGET;

        m_Player->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED | UNIT_FLAG_IMMUNE_TO_NPC);
        m_Player->SetRooted(false);

        if (l_Wild)
        {
            l_Wild->SetTarget(0);
            l_Wild->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED | UNIT_FLAG_IMMUNE_TO_PC);
            l_Wild->SetControlled(false, UNIT_STATE_ROOT);
        }

        for (size_t l_CurrentPetID = 0; l_CurrentPetID < MAX_PETBATTLE_SLOTS; ++l_CurrentPetID)
            if (l_PlayerPets[l_CurrentPetID])
                l_PlayerPets[l_CurrentPetID] = BattlePetInstance::Ptr();

        l_WildBattlePet = BattlePetInstance::Ptr();

        SendPetBattleRequestFailed(l_ErrorCode);
        sPetBattleSystem->RemoveRequest(l_BattleRequest->RequesterGuid);
        return;
    }

    l_WildBattlePet = sWildBattlePetMgr->GetWildBattlePet(l_Wild);

    if (!l_WildBattlePet)
    {
        l_ErrorCode = PETBATTLE_REQUEST_INVALID_TARGET;

        m_Player->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED | UNIT_FLAG_IMMUNE_TO_NPC);
        m_Player->SetRooted(false);

        if (l_Wild)
        {
            l_Wild->SetTarget(0);
            l_Wild->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED | UNIT_FLAG_IMMUNE_TO_PC);
            l_Wild->SetControlled(false, UNIT_STATE_ROOT);
        }

        for (size_t l_CurrentPetID = 0; l_CurrentPetID < MAX_PETBATTLE_SLOTS; ++l_CurrentPetID)
            if (l_PlayerPets[l_CurrentPetID])
                l_PlayerPets[l_CurrentPetID] = BattlePetInstance::Ptr();

        l_WildBattlePet = BattlePetInstance::Ptr();

        SendPetBattleRequestFailed(l_ErrorCode);
        sPetBattleSystem->RemoveRequest(l_BattleRequest->RequesterGuid);
        return;
    }

    SendPetBattleFinalizeLocation(l_BattleRequest);

    m_Player->SetFacingTo(m_Player->GetAngle(l_BattleRequest->TeamPosition[PETBATTLE_TEAM_2][0], l_BattleRequest->TeamPosition[PETBATTLE_TEAM_2][1]));
    m_Player->SetRooted(true);

    // Ok, Create battle
    l_Battle = sPetBattleSystem->CreateBattle();

    // Add player pets
    l_Battle->Teams[PETBATTLE_TEAM_1]->OwnerGuid    = m_Player->GetGUID();
    l_Battle->Teams[PETBATTLE_TEAM_1]->PlayerGuid   = m_Player->GetGUID();

    for (size_t l_CurrentPetSlot = 0; l_CurrentPetSlot < MAX_PETBATTLE_SLOTS; ++l_CurrentPetSlot)
    {
        if (l_PlayerPets[l_CurrentPetSlot])
            l_Battle->AddPet(PETBATTLE_TEAM_1, l_PlayerPets[l_CurrentPetSlot]);
    }

    // Add wild pet
    //TODO: wild pets can group nearby battle pets with lesser or equal level
    l_Battle->Teams[PETBATTLE_TEAM_2]->OwnerGuid = l_Wild->GetGUID();

    l_Battle->BattleType = l_BattleRequest->RequestType;
    l_Battle->AddPet(PETBATTLE_TEAM_2, l_WildBattlePet);

    // Launch battle
    m_Player->_petBattleId = l_Battle->ID;
    l_Battle->Begin();

    sWildBattlePetMgr->EnterInBattle(l_Wild);
    sPetBattleSystem->RemoveRequest(l_BattleRequest->RequesterGuid);

    for (size_t l_CurrentPetID = 0; l_CurrentPetID < MAX_PETBATTLE_SLOTS; ++l_CurrentPetID)
        if (l_PlayerPets[l_CurrentPetID])
            l_PlayerPets[l_CurrentPetID] = BattlePetInstance::Ptr();

    l_WildBattlePet = BattlePetInstance::Ptr();
}

void WorldSession::HandlePetBattleRequestPvP(WorldPacket& p_RecvData)
{
}

void WorldSession::HandlePetBattleJoinQueue(WorldPacket& p_RecvData)
{
}

void WorldSession::HandlePetBattleRequestUpdate(WorldPacket& p_RecvData)
{
}

void WorldSession::HandlePetBattleCancelRequestPvPMatchmaking(WorldPacket& p_RecvData)
{
}

enum ePetBattleActions
{
    PETBATTLE_ACTION_REQUEST_LEAVE      = 0,
    PETBATTLE_ACTION_CAST               = 1,
    PETBATTLE_ACTION_SWAP_OR_PASS       = 2,
    PETBATTLE_ACTION_CATCH              = 3,
    PETBATTLE_ACTION_LEAVE_PETBATTLE    = 4
};

void WorldSession::HandlePetBattleInputNewFrontPet(WorldPacket& p_RecvData)
{
    if (!m_Player->_petBattleId)
    {
        SendPetBattleFinished(0);
        return;
    }

    PetBattle* l_PetBattle = sPetBattleSystem->GetBattle(m_Player->_petBattleId);

    if (!l_PetBattle || l_PetBattle->BattleStatus == PETBATTLE_STATUS_FINISHED)
    {
        SendPetBattleFinished(0);
        return;
    }

    uint8 l_NewFrontPetID = 0;
    uint32 l_PlayerTeamID = 0;

    p_RecvData >> l_NewFrontPetID;

    if (l_PetBattle->Teams[PETBATTLE_TEAM_2]->PlayerGuid == m_Player->GetGUID())
        l_PlayerTeamID = PETBATTLE_TEAM_2;

    // Skip if input already died
    if (l_PetBattle->Teams[l_PlayerTeamID]->Ready)
        return;

    l_NewFrontPetID = (l_PlayerTeamID == PETBATTLE_TEAM_2 ? MAX_PETBATTLE_SLOTS : 0) + l_NewFrontPetID;

    if (!l_PetBattle->Teams[l_PlayerTeamID]->CanSwap(l_NewFrontPetID))
        return;

    l_PetBattle->SwapPet(l_PlayerTeamID, l_NewFrontPetID);
    l_PetBattle->SwapPet(!l_PlayerTeamID, l_PetBattle->Teams[!l_PlayerTeamID]->ActivePetID);
}

void WorldSession::HandlePetBattleInput(WorldPacket& p_RecvData)
{
    uint8   l_Action        = 0;
    uint8   l_NewFrontPetID = 0;
    uint32  l_Ability       = 0;
    uint32  l_Turn          = 0;

    p_RecvData >> l_Action;
    p_RecvData >> l_NewFrontPetID;
    p_RecvData.read_skip<uint8>();
    p_RecvData.read_skip<uint8>();
    p_RecvData >> l_Ability;
    p_RecvData >> l_Turn;

    p_RecvData.ReadBit();

    if (l_Action == PETBATTLE_ACTION_LEAVE_PETBATTLE)
    {
        SendPetBattleFinished(0);
        return;
    }

    if (!m_Player->_petBattleId)
    {
        SendPetBattleFinished(0);
        return;
    }

    PetBattle * l_PetBattle = sPetBattleSystem->GetBattle(m_Player->_petBattleId);

    if (!l_PetBattle || l_PetBattle->BattleStatus == PETBATTLE_STATUS_FINISHED)
    {
        SendPetBattleFinished(0);
        return;
    }

    ///// Check sync
    if ((l_Turn + 1) != l_PetBattle->Turn)
    {
        sPetBattleSystem->ForfeitBattle(l_PetBattle->ID, m_Player->GetGUID());
        return;
    }

    if (l_Action)
    {
        uint32 l_PlayerTeamID = 0;

        if (l_PetBattle->Teams[PETBATTLE_TEAM_2]->PlayerGuid == m_Player->GetGUID())
            l_PlayerTeamID = PETBATTLE_TEAM_2;

        // Skip if input already died
        if (l_PetBattle->Teams[l_PlayerTeamID]->Ready)
            return;

        if (l_Action == PETBATTLE_ACTION_REQUEST_LEAVE)
        {
            sPetBattleSystem->ForfeitBattle(l_PetBattle->ID, m_Player->GetGUID());
        }
        else if (l_Action == PETBATTLE_ACTION_CAST)
        {
            if (l_PetBattle->CanCast(l_PlayerTeamID, l_Ability))
                l_PetBattle->PrepareCast(l_PlayerTeamID, l_Ability);
        }
        else if (l_Action == PETBATTLE_ACTION_CATCH)
        {
            uint32 l_CatchAbilityID = l_PetBattle->Teams[l_PlayerTeamID]->GetCatchAbilityID();

            if (l_PetBattle->Teams[l_PlayerTeamID]->CanCatchOpponentTeamFrontPet() == PETBATTLE_TEAM_CATCH_FLAG_ENABLE_TRAP)
                l_PetBattle->PrepareCast(l_PlayerTeamID, l_CatchAbilityID);
        }
        else if (l_Action == PETBATTLE_ACTION_SWAP_OR_PASS)
        {
            l_NewFrontPetID = (l_PlayerTeamID == PETBATTLE_TEAM_2 ? MAX_PETBATTLE_SLOTS : 0) + l_NewFrontPetID;

            if (!l_PetBattle->Teams[l_PlayerTeamID]->CanSwap(l_NewFrontPetID))
                return;

            l_PetBattle->SwapPet(l_PlayerTeamID, l_NewFrontPetID);
        }
    }
}
