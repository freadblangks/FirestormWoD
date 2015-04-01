/*
 * Copyright (C) 2008-20XX JadeCore <http://www.pandashan.com>
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "VMapFactory.h"
#include "mogu_shan_vault.h"

DoorData const doorData[] =
{
    {GOB_STONE_GUARD_DOOR_ENTRANCE,          DATA_STONE_GUARD,          DOOR_TYPE_ROOM,       BOUNDARY_E   },
    {GOB_STONE_GUARD_DOOR_EXIT,              DATA_STONE_GUARD,          DOOR_TYPE_PASSAGE,    BOUNDARY_W   },
    {GOB_FENG_DOOR_FENCE,                    DATA_FENG,                 DOOR_TYPE_ROOM,       BOUNDARY_NONE},
    {GOB_FENG_DOOR_EXIT,                     DATA_FENG,                 DOOR_TYPE_PASSAGE,    BOUNDARY_N   },
    {GOB_GARAJAL_FENCE,                      DATA_GARAJAL,              DOOR_TYPE_ROOM,       BOUNDARY_NONE},
    {GOB_GARAJAL_EXIT,                       DATA_GARAJAL,              DOOR_TYPE_PASSAGE,    BOUNDARY_W   },
    {GOB_SPIRIT_KINGS_WIND_WALL,             DATA_SPIRIT_KINGS,         DOOR_TYPE_ROOM,       BOUNDARY_NONE},
    {GOB_SPIRIT_KINGS_EXIT,                  DATA_SPIRIT_KINGS,         DOOR_TYPE_PASSAGE,    BOUNDARY_NONE},
    {GOB_CELESTIAL_DOOR,                     DATA_ELEGON,               DOOR_TYPE_ROOM,       BOUNDARY_E   },
    {GOB_ELEGON_DOOR_ENTRANCE,               DATA_SPIRIT_KINGS,         DOOR_TYPE_PASSAGE,    BOUNDARY_NONE},
    {GOB_ELEGON_CELESTIAL_DOOR,              DATA_ELEGON,               DOOR_TYPE_ROOM,       BOUNDARY_E   },
    {GOB_WILL_OF_EMPEROR_ENTRANCE,           DATA_ELEGON,               DOOR_TYPE_PASSAGE,    BOUNDARY_NONE},
    {0,                                      0,                         DOOR_TYPE_ROOM,       BOUNDARY_NONE},// END
};

#define DIST_BETWEEN_TWO_Z      32.39f
#define ACHIEVEMENT_SHOWMOVES   6455

Position woeSpawnPos[8] =
{
    // West
    {3895.44f, 1615.72f, 368.22f, 4.70f},
    {3874.47f, 1615.81f, 368.22f, 4.68f},
    {3851.89f, 1612.51f, 368.22f, 5.03f},
    {3833.06f, 1601.53f, 368.22f, 5.37f},

    // East
    {3833.46f, 1500.18f, 368.22f, 0.87f},
    {3852.07f, 1488.89f, 368.22f, 1.23f},
    {3874.45f, 1485.18f, 368.22f, 1.58f},
    {3895.44f, 1485.12f, 368.22f, 1.60f}
};

Position woeBossSpawnPos[2] =
{
    {3817.58f, 1583.18f, 368.22f, 5.73f},
    {3817.66f, 1517.56f, 368.22f, 0.56f}
};

Position woeRageSpawnPos[3] =
{
    {3812.60f, 1536.72f, 367.64f, 0.16f},
    {3810.98f, 1550.45f, 367.64f, 0.27f},
    {3812.32f, 1563.90f, 367.64f, 6.11f}
};

Position woeMiddleSpawnPos[1] =
{
    {3812.60f, 1536.72f, 367.64f, 0.16f}
};

class instance_mogu_shan_vault : public InstanceMapScript
{
    public:
        instance_mogu_shan_vault() : InstanceMapScript("instance_mogu_shan_vault", 1008) { }

        InstanceScript* GetInstanceScript(InstanceMap* map) const
        {
            return new instance_mogu_shan_vault_InstanceMapScript(map);
        }

        struct instance_mogu_shan_vault_InstanceMapScript : public InstanceScript
        {
            instance_mogu_shan_vault_InstanceMapScript(Map* map) : InstanceScript(map) {}

            uint32 actualPetrifierEntry;
            uint8  guardianCount;
            uint8  guardianAliveCount;
            bool   woeIsGasPhaseActive;

            int8   randomDespawnStoneGuardian;
            uint8  nextWillOfEmperorPhase;
            uint8  willOfEmperirLastBigAddSpawned;

            uint32 StoneGuardPetrificationTimer;
            uint32 willOfEmperorTimer;
            uint32 willOfEmperorBossSpawnTimer;
            uint32 willOfEmperorGasPhaseTimer;

            uint64 cursedMogu1Guid;
            uint64 cursedMogu2Guid;
            uint64 ghostEssenceGuid;

            uint64 stoneGuardControlerGuid;
            uint64 fengGuid;
            uint64 siphonShieldGuid;
            uint64 spiritKingsControlerGuid;
            uint64 elegonGuid;
            uint64 infiniteEnergyGuid;

            uint64 inversionGobGuid;
            uint64 stoneGuardExit;
            uint64 cancelGobGuid;
            uint64 ancientMoguDoorGuid;
            uint64 emperorsDoorGuid;
            uint64 celestialCommandGuid;
            uint64 ancientConsoleGuid;

            uint64 energyPlatformGuid;
            uint64 titanDiskGuid;
            uint64 janxiGuid;
            uint64 qinxiGuid;

            std::list<uint32> m_AuraToClear;

            std::vector<uint64> stoneGuardGUIDs;
            std::vector<uint64> fengStatuesGUIDs;
            std::vector<uint64> spiritKingsGUIDs;
            std::vector<uint64> titanCirclesGuids;
            std::list<uint32>   achievementGuids;

            void Initialize()
            {
                SetBossNumber(DATA_MAX_BOSS_DATA);
                LoadDoorData(doorData);

                guardianCount                   = 0;
                guardianAliveCount              = 0;

                stoneGuardControlerGuid         = 0;
                fengGuid                        = 0;
                siphonShieldGuid                = 0;

                inversionGobGuid                = 0;
                cancelGobGuid                   = 0;
                ancientMoguDoorGuid             = 0;
                emperorsDoorGuid                = 0;
                celestialCommandGuid            = 0;
                energyPlatformGuid              = 0;
                titanDiskGuid                   = 0;
                ancientConsoleGuid              = 0;
                woeIsGasPhaseActive             = false;

                randomDespawnStoneGuardian      = urand(1,4);
                nextWillOfEmperorPhase          = 0;
                willOfEmperirLastBigAddSpawned  = 0;

                actualPetrifierEntry            = 0;
                StoneGuardPetrificationTimer    = 10000;
                willOfEmperorTimer              = 0;
                willOfEmperorBossSpawnTimer     = 0;
                willOfEmperorGasPhaseTimer      = 0;

                cursedMogu1Guid                 = 0;
                cursedMogu2Guid                 = 0;
                ghostEssenceGuid                = 0;

                stoneGuardControlerGuid         = 0;
                inversionGobGuid                = 0;
                cancelGobGuid                   = 0;
                spiritKingsControlerGuid        = 0;

                qinxiGuid                       = 0;
                janxiGuid                       = 0;

                stoneGuardGUIDs.clear();
                fengStatuesGUIDs.clear();
                spiritKingsGUIDs.clear();
                achievementGuids.clear();

                m_AuraToClear.clear();
                m_AuraToClear.push_back(116541); ///< SPELL_TILES_AURA_EFFECT from stone guard
            }

            void OnCreatureCreate(Creature* creature)
            {
                switch (creature->GetEntry())
                {
                    case NPC_STONE_GUARD_CONTROLER:
                        stoneGuardControlerGuid = creature->GetGUID();
                        break;
                    case NPC_JASPER:
                    case NPC_JADE:
                    case NPC_AMETHYST:
                    case NPC_COBALT:
                    {
                        if (creature->isAlive())
                        {
                            stoneGuardGUIDs.push_back(creature->GetGUID());
                            guardianAliveCount++;
                        }

                        uint32 difficulty = instance->GetSpawnMode();
                        bool turnOver = (difficulty == Difficulty::Difficulty10N || difficulty == Difficulty::Difficulty10HC || difficulty == Difficulty::DifficultyRaidTool);

                        // In 10N, 10H or LFR, there are only 3 guardians
                        if (guardianAliveCount >= 4 && GetBossState(DATA_STONE_GUARD) != DONE && turnOver)
                        {
                            std::vector<uint32> l_Gardians { NPC_JADE, NPC_AMETHYST, NPC_COBALT };
                            Creature* l_Creature = instance->GetCreature(GetData64(l_Gardians[urand(0, 2)]));
                            if (l_Creature != nullptr)
                            {
                                l_Creature->DespawnOrUnsummon();
                                --guardianAliveCount;
                            }

                        }
                        break;
                    }
                    case NPC_CURSED_MOGU_SCULPTURE_2:
                        if (!cursedMogu1Guid)
                            cursedMogu1Guid = creature->GetGUID();
                        else
                            cursedMogu2Guid = creature->GetGUID();
                        break;
                    case NPC_GHOST_ESSENCE:
                        ghostEssenceGuid = creature->GetGUID();
                        break;
                    case NPC_FENG:
                        fengGuid = creature->GetGUID();
                        break;
                    case NPC_SIPHONING_SHIELD:
                        siphonShieldGuid = creature->GetGUID();
                        break;
                    case NPC_SPIRIT_GUID_CONTROLER:
                        spiritKingsControlerGuid = creature->GetGUID();
                        break;
                    case NPC_ZIAN:
                        // Will be false only if BossState = DONE or NOT_STARTED, as NOT_STARTED = 0, DONE = 0, and max value for BossState = 5
                        if (GetBossState(DATA_SPIRIT_KINGS) % DONE != NOT_STARTED)
                            SetBossState(DATA_SPIRIT_KINGS, NOT_STARTED);
                        // No break here!!!
                    case NPC_MENG:
                    case NPC_QIANG:
                    case NPC_SUBETAI:
                        spiritKingsGUIDs.push_back(creature->GetGUID());
                        break;
                    case NPC_ELEGON:
                        elegonGuid = creature->GetGUID();
                        break;
                    case NPC_INFINITE_ENERGY:
                        infiniteEnergyGuid = creature->GetGUID();
                        break;
                    case NPC_QIN_XI:
                        qinxiGuid = creature->GetGUID();
                        break;
                    case NPC_JAN_XI:
                        janxiGuid = creature->GetGUID();
                        break;
                    default:
                        break;
                }
            }

            void OnGameObjectCreate(GameObject* go)
            {
                switch (go->GetEntry())
                {
                    case GOB_STONE_GUARD_DOOR_ENTRANCE:
                    case GOB_FENG_DOOR_FENCE:
                    case GOB_FENG_DOOR_EXIT:
                    case GOB_GARAJAL_FENCE:
                    case GOB_GARAJAL_EXIT:
                    case GOB_SPIRIT_KINGS_WIND_WALL:
                    case GOB_SPIRIT_KINGS_EXIT:
                    case GOB_CELESTIAL_DOOR:
                        AddDoor(go, true);
                        break;
                    // Feng
                    case GOB_SPEAR_STATUE:
                    case GOB_FIST_STATUE:
                    case GOB_STAFF_STATUE:
                        fengStatuesGUIDs.push_back(go->GetGUID());
                        break;
                    case GOB_SHIELD_STATUE:
                    {
                        if (!instance->IsHeroic())
                            go->SetObjectScale(0.001f);
                        else
                            fengStatuesGUIDs.push_back(go->GetGUID());
                        break;
                    }
                    case GOB_STONE_GUARD_DOOR_EXIT:
                        AddDoor(go, true);
                        stoneGuardExit = go->GetGUID();
                        break;
                    case GOB_INVERSION:
                        inversionGobGuid = go->GetGUID();
                        break;
                    case GOB_CANCEL:
                        cancelGobGuid = go->GetGUID();
                        break;
                    case GOB_ENERGY_PLATFORM:
                        energyPlatformGuid = go->GetGUID();
                        go->SetGoState(GO_STATE_ACTIVE);
                        break;
                    case GOB_ELEGON_DOOR_ENTRANCE:
                        AddDoor(go, true);
                        ancientMoguDoorGuid = go->GetGUID();
                        break;
                    case GOB_WILL_OF_EMPEROR_ENTRANCE:
                        AddDoor(go, true);
                        emperorsDoorGuid = go->GetGUID();
                        break;
                    case GOB_ENERGY_TITAN_DISK:
                        titanDiskGuid = go->GetGUID();
                        break;
                    case GOB_ENERGY_TITAN_CIRCLE_1:
                    case GOB_ENERGY_TITAN_CIRCLE_2:
                    case GOB_ENERGY_TITAN_CIRCLE_3:
                        go->SetGoState(GO_STATE_ACTIVE);
                        titanCirclesGuids.push_back(go->GetGUID());
                        break;
                    case GOB_CELESTIAL_COMMAND:
                        celestialCommandGuid = go->GetGUID();
                        break;
                    case GOB_ANCIENT_CONTROL_CONSOLE:
                        ancientConsoleGuid = go->GetGUID();
                        break;
                }
            }

            void OnPlayerLeave(Player* p_Player)
            {
                for (const uint32& l_AuraID : m_AuraToClear)
                {
                    if (p_Player->HasAura(l_AuraID))
                        p_Player->RemoveAurasDueToSpell(l_AuraID);
                }
            }

            bool SetBossState(uint32 id, EncounterState state)
            {
                if (!InstanceScript::SetBossState(id, state))
                    return false;

                switch (id)
                {
                    case DATA_STONE_GUARD:
                    {
                        switch (state)
                        {
                            case IN_PROGRESS:
                                if (Creature* stoneGuardControler = instance->GetCreature(stoneGuardControlerGuid))
                                    stoneGuardControler->AI()->DoAction(ACTION_ENTER_COMBAT);

                                for (auto stoneGuardGuid: stoneGuardGUIDs)
                                    if (Creature* stoneGuard = instance->GetCreature(stoneGuardGuid))
                                        stoneGuard->AI()->DoAction(ACTION_ENTER_COMBAT);
                                break;
                            case FAIL:
                                for (auto stoneGuardGuid: stoneGuardGUIDs)
                                    if (Creature* stoneGuard = instance->GetCreature(stoneGuardGuid))
                                        stoneGuard->AI()->DoAction(ACTION_FAIL);
                                break;
                            case DONE:
                                if (Creature* stoneGuardControler = instance->GetCreature(stoneGuardControlerGuid))
                                    stoneGuardControler->CastSpell(stoneGuardControler, ACHIEVEMENT_STONE_GUARD_KILL, true);
                                break;
                            default:
                                break;
                        }
                        break;
                    }
                    case DATA_SPIRIT_KINGS:
                    {
                        switch (state)
                        {
                            case IN_PROGRESS:
                            {
                                if (Creature* spiritKingsControler = instance->GetCreature(spiritKingsControlerGuid))
                                    spiritKingsControler->AI()->DoAction(ACTION_ENTER_COMBAT);
                                break;
                            }
                            default:
                                break;
                        }
                        break;
                    }
                    case DATA_ELEGON:
                    {
                        switch (state)
                        {
                            case TO_BE_DECIDED:
                            {
                                if (GameObject* titanDisk = instance->GetGameObject(titanDiskGuid))
                                    titanDisk->SetGoState(GO_STATE_ACTIVE);

                                if (GameObject* energyPlatform = instance->GetGameObject(energyPlatformGuid))
                                    energyPlatform->SetGoState(GO_STATE_ACTIVE);

                                for (auto guid: titanCirclesGuids)
                                    if (GameObject* titanCircles = instance->GetGameObject(guid))
                                        titanCircles->SetGoState(GO_STATE_ACTIVE);

                                break;
                            }
                            case FAIL:
                            {
                                if (GameObject* titanDisk = instance->GetGameObject(titanDiskGuid))
                                    titanDisk->SetGoState(GO_STATE_READY);

                                if (GameObject* energyPlatform = instance->GetGameObject(energyPlatformGuid))
                                    energyPlatform->SetGoState(GO_STATE_READY);

                                for (auto guid: titanCirclesGuids)
                                    if (GameObject* titanCircles = instance->GetGameObject(guid))
                                        titanCircles->SetGoState(GO_STATE_READY);

                                break;
                            }
                            default:
                                break;
                        }
                        break;
                    }
                    case DATA_WILL_OF_EMPEROR:
                    {
                        switch (state)
                        {
                            case NOT_STARTED:
                            case FAIL:
                                willOfEmperorTimer              = 0;
                                nextWillOfEmperorPhase          = 0;
                                willOfEmperirLastBigAddSpawned  = 0;
                                willOfEmperorBossSpawnTimer     = 0;
                                willOfEmperorGasPhaseTimer      = 0;
                                woeIsGasPhaseActive             = false;
                                break;
                            case IN_PROGRESS:
                                willOfEmperorTimer              = 10000;
                                nextWillOfEmperorPhase          = 0;
                                willOfEmperirLastBigAddSpawned  = 0;
                                willOfEmperorBossSpawnTimer     = 90000;
                                willOfEmperorGasPhaseTimer      = 210000; // 120 + 90
                                woeIsGasPhaseActive             = false;
                                break;
                            default:
                                break;
                        }
                    }
                    default:
                        break;
                }

                return true;
            }

            void SetData(uint32 type, uint32 data)
            {
                if (type == ACHIEVEMENT_SHOWMOVES)
                    SetAchievementValid(ACHIEVEMENT_SHOWMOVES);

                return;
            }

            uint32 GetData(uint32 type)
            {
                if (type == ACHIEVEMENT_SHOWMOVES)
                    return IsAchievementValid(ACHIEVEMENT_SHOWMOVES);

                return 0;
            }

            uint64 GetData64(uint32 type)
            {
                switch (type)
                {
                    // Creature
                    // Stone Guard
                    case NPC_STONE_GUARD_CONTROLER:
                        return stoneGuardControlerGuid;
                    case NPC_CURSED_MOGU_SCULPTURE_1:
                        return cursedMogu1Guid;
                    case NPC_CURSED_MOGU_SCULPTURE_2:
                        return cursedMogu2Guid;
                    case NPC_GHOST_ESSENCE:
                        return ghostEssenceGuid;
                    case NPC_JASPER:
                    case NPC_JADE:
                    case NPC_AMETHYST:
                    case NPC_COBALT:
                    {
                        for (auto guid: stoneGuardGUIDs)
                            if (Creature* stoneGuard = instance->GetCreature(guid))
                                if (stoneGuard->GetEntry() == type)
                                    return guid;
                        break;
                    }
                    // Feng
                    case NPC_FENG:
                        return fengGuid;
                    // SiphonShield
                    case NPC_SIPHONING_SHIELD:
                        return siphonShieldGuid;
                    // Spirit Kings
                    case NPC_SPIRIT_GUID_CONTROLER:
                        return spiritKingsControlerGuid;
                    case NPC_ZIAN:
                    case NPC_MENG:
                    case NPC_QIANG:
                    case NPC_SUBETAI:
                    {
                        for (auto guid: spiritKingsGUIDs)
                            if (Creature* spiritKing = instance->GetCreature(guid))
                                if (spiritKing->GetEntry() == type)
                                    return guid;
                        break;
                    }
                    // Elegon
                    case NPC_ELEGON:
                        return elegonGuid;
                    case NPC_INFINITE_ENERGY:
                        return infiniteEnergyGuid;
                    // Will of Emperor
                    case NPC_QIN_XI:
                        return qinxiGuid;
                    case NPC_JAN_XI:
                        return janxiGuid;
                    // Gameobject
                    case GOB_SPEAR_STATUE:
                    case GOB_FIST_STATUE:
                    case GOB_SHIELD_STATUE:
                    case GOB_STAFF_STATUE:
                    {
                        for (auto guid: fengStatuesGUIDs)
                            if (GameObject* fengStatue = instance->GetGameObject(guid))
                                if (fengStatue->GetEntry() == type)
                                    return guid;
                        break;
                    }
                    case GOB_STONE_GUARD_DOOR_EXIT:
                        return stoneGuardExit;
                    case GOB_INVERSION:
                        return inversionGobGuid;
                    case GOB_CANCEL:
                        return cancelGobGuid;
                    case GOB_ENERGY_PLATFORM:
                        return energyPlatformGuid;
                    case GOB_ENERGY_TITAN_DISK:
                        return titanDiskGuid;
                    case GOB_ELEGON_DOOR_ENTRANCE:
                        return ancientMoguDoorGuid;
                    case GOB_WILL_OF_EMPEROR_ENTRANCE:
                        return emperorsDoorGuid;
                    case GOB_CELESTIAL_COMMAND:
                        return celestialCommandGuid;
                    case GOB_ANCIENT_CONTROL_CONSOLE:
                        return ancientConsoleGuid;
                    default:
                        break;
                }

                return 0;
            }

            bool IsWipe()
            {
                Map::PlayerList const& PlayerList = instance->GetPlayers();

                if (PlayerList.isEmpty())
                    return true;

                for (Map::PlayerList::const_iterator Itr = PlayerList.begin(); Itr != PlayerList.end(); ++Itr)
                {
                    Player* player = Itr->getSource();

                    if (!player)
                        continue;

                    if (player->isAlive() && !player->isGameMaster() && !player->HasAura(115877)) // Aura 115877 = Totaly Petrified
                        return false;
                }

                return true;
            }

            void Update(uint32 diff)
            {
                if (GetBossState(DATA_WILL_OF_EMPEROR) != IN_PROGRESS)
                    return;

                if (willOfEmperorTimer && !woeIsGasPhaseActive)
                {
                    if (willOfEmperorTimer <= diff)
                    {
                        switch (nextWillOfEmperorPhase)
                        {
                            case PHASE_WOE_RAGE:
                            {
                                uint8 randomPos = urand(0, 2);
                                uint8 randomPos2 = urand(0, 2);

                                while (randomPos2 == randomPos)
                                    randomPos2 = urand(0, 2);

                                instance->SummonCreature(NPC_EMPEROR_RAGE, woeRageSpawnPos[randomPos]);
                                instance->SummonCreature(NPC_EMPEROR_RAGE, woeRageSpawnPos[randomPos2]);

                                if (willOfEmperirLastBigAddSpawned == PHASE_WOE_STRENGHT)
                                {
                                    nextWillOfEmperorPhase = PHASE_WOE_COURAGE;
                                    willOfEmperorTimer = 20000;
                                }
                                else
                                {
                                    nextWillOfEmperorPhase = PHASE_WOE_STRENGHT;
                                    willOfEmperorTimer = 15000;
                                }
                                break;
                            }
                            case PHASE_WOE_STRENGHT:
                            {
                                instance->SummonCreature(NPC_EMPEROR_STRENGHT, woeSpawnPos[urand(0, 7)]);

                                nextWillOfEmperorPhase = PHASE_WOE_RAGE;
                                willOfEmperirLastBigAddSpawned = PHASE_WOE_STRENGHT;
                                willOfEmperorTimer = 15000;
                                break;
                            }
                            case PHASE_WOE_COURAGE:
                            {
                                instance->SummonCreature(NPC_EMPEROR_COURAGE, woeSpawnPos[urand(0, 7)]);

                                nextWillOfEmperorPhase = PHASE_WOE_RAGE;
                                willOfEmperirLastBigAddSpawned = PHASE_WOE_COURAGE;
                                willOfEmperorTimer = 10000;
                                break;
                            }
                        }
                    }
                    else
                        willOfEmperorTimer -= diff;
                }

                if (willOfEmperorBossSpawnTimer)
                {
                    if (willOfEmperorBossSpawnTimer <= diff)
                    {
                        instance->SummonCreature(NPC_JAN_XI, woeBossSpawnPos[0]);
                        instance->SummonCreature(NPC_QIN_XI, woeBossSpawnPos[1]);
                        willOfEmperorBossSpawnTimer = 0;
                    }
                    else
                        willOfEmperorBossSpawnTimer -= diff;
                }

                if (willOfEmperorGasPhaseTimer)
                {
                    if (willOfEmperorGasPhaseTimer <= diff)
                    {
                        if (!woeIsGasPhaseActive)
                        {
                            if (Creature* trigger = instance->SummonCreature(59481, woeMiddleSpawnPos[0], NULL, 30000))
                                trigger->CastSpell(trigger, 118320, false);
                            willOfEmperorGasPhaseTimer = 30000;
                            woeIsGasPhaseActive = true;
                        }
                        else
                        {
                            willOfEmperorGasPhaseTimer = 120000;
                            woeIsGasPhaseActive = false;
                        }
                    }
                    else
                        willOfEmperorGasPhaseTimer -= diff;
                }
            }

            bool CheckRequiredBosses(uint32 bossId, Player const* player = NULL) const
            {
                if (!InstanceScript::CheckRequiredBosses(bossId, player))
                    return false;

                switch (bossId)
                {
                    case DATA_WILL_OF_EMPEROR:
                    case DATA_ELEGON:
                    case DATA_SPIRIT_KINGS:
                    case DATA_GARAJAL:
                    case DATA_FENG:
                        if (GetBossState(bossId - 1) != DONE)
                            return false;
                    default:
                        break;
                }

                return true;
            }

            bool IsAchievementValid(uint32 id) const
            {
                if (achievementGuids.empty())
                    return false;

                for (std::list<uint32>::const_iterator iter = achievementGuids.begin(); iter != achievementGuids.end(); ++iter)
                    if ((*iter) == id)
                        return true;

                return false;
            }

            void SetAchievementValid(uint32 id)
            {
                for (std::list<uint32>::const_iterator iter = achievementGuids.begin(); iter != achievementGuids.end(); ++iter)
                    if ((*iter) == id)
                        return;

                achievementGuids.push_back(id);
                return;
            }
        };
};

void AddSC_instance_mogu_shan_vault()
{
    new instance_mogu_shan_vault();
}