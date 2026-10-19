#include "global.h"
#include "constants/event_flag.h"
#include "constants/item.h"
#include "event_flag.h"
#include "items.h"
#include "memory.h"
#include "pokemon.h"
#include "transfer_password.h"

// The password is TRANSFER_PASSWORD_LENGTH characters of 5 bits each (170 bits):
//   bits   0-135: payload (17 bytes), XOR-scrambled with a keystream seeded by the checksum
//   bits 136-167: CRC-32 of the unscrambled payload
//   bits 168-169: always 0
// Payload layout (bit offset, width):
//   0  4  version (TRANSFER_PASSWORD_VERSION)
//   4  10 hero species (Explorers of Sky ID)
//   14 7  hero level
//   21 10 partner species (Explorers of Sky ID)
//   31 7  partner level
//   38 17 money carried
//   55 24 money in the bank
//   79 57 reserved, always 0

#define TRANSFER_PAYLOAD_SIZE 17
#define TRANSFER_CRC_BIT_POS (TRANSFER_PAYLOAD_SIZE * 8)
#define TRANSFER_PAD_BIT_POS (TRANSFER_CRC_BIT_POS + 32)
#define TRANSFER_BITS_SIZE ((TRANSFER_PASSWORD_LENGTH * 5 + 7) / 8)

#define TRANSFER_MAX_LEVEL 100
#define TRANSFER_MAX_SKY_SPECIES 552 // MONSTER_DECOY - 1 in Explorers of Sky

// Same alphabet as Explorers of Sky's password keyboard
static const u8 sTransferAlphabet[] = "&67NPR89F0+#STXY45MCHJ-K12=%3Q@W";

static const s16 sSkySpeciesIds[MONSTER_MAX] = {
    0, // MONSTER_NONE
    1, // MONSTER_BULBASAUR
    2, // MONSTER_IVYSAUR
    3, // MONSTER_VENUSAUR
    4, // MONSTER_CHARMANDER
    5, // MONSTER_CHARMELEON
    6, // MONSTER_CHARIZARD
    7, // MONSTER_SQUIRTLE
    8, // MONSTER_WARTORTLE
    9, // MONSTER_BLASTOISE
    10, // MONSTER_CATERPIE
    11, // MONSTER_METAPOD
    12, // MONSTER_BUTTERFREE
    13, // MONSTER_WEEDLE
    14, // MONSTER_KAKUNA
    15, // MONSTER_BEEDRILL
    16, // MONSTER_PIDGEY
    17, // MONSTER_PIDGEOTTO
    18, // MONSTER_PIDGEOT
    19, // MONSTER_RATTATA
    20, // MONSTER_RATICATE
    21, // MONSTER_SPEAROW
    22, // MONSTER_FEAROW
    23, // MONSTER_EKANS
    24, // MONSTER_ARBOK
    25, // MONSTER_PIKACHU
    26, // MONSTER_RAICHU
    27, // MONSTER_SANDSHREW
    28, // MONSTER_SANDSLASH
    29, // MONSTER_NIDORAN_F
    30, // MONSTER_NIDORINA
    31, // MONSTER_NIDOQUEEN
    32, // MONSTER_NIDORAN_M
    33, // MONSTER_NIDORINO
    34, // MONSTER_NIDOKING
    35, // MONSTER_CLEFAIRY
    36, // MONSTER_CLEFABLE
    37, // MONSTER_VULPIX
    38, // MONSTER_NINETALES
    39, // MONSTER_JIGGLYPUFF
    40, // MONSTER_WIGGLYTUFF
    41, // MONSTER_ZUBAT
    42, // MONSTER_GOLBAT
    43, // MONSTER_ODDISH
    44, // MONSTER_GLOOM
    45, // MONSTER_VILEPLUME
    46, // MONSTER_PARAS
    47, // MONSTER_PARASECT
    48, // MONSTER_VENONAT
    49, // MONSTER_VENOMOTH
    50, // MONSTER_DIGLETT
    51, // MONSTER_DUGTRIO
    52, // MONSTER_MEOWTH
    53, // MONSTER_PERSIAN
    54, // MONSTER_PSYDUCK
    55, // MONSTER_GOLDUCK
    56, // MONSTER_MANKEY
    57, // MONSTER_PRIMEAPE
    58, // MONSTER_GROWLITHE
    59, // MONSTER_ARCANINE
    60, // MONSTER_POLIWAG
    61, // MONSTER_POLIWHIRL
    62, // MONSTER_POLIWRATH
    63, // MONSTER_ABRA
    64, // MONSTER_KADABRA
    65, // MONSTER_ALAKAZAM
    66, // MONSTER_MACHOP
    67, // MONSTER_MACHOKE
    68, // MONSTER_MACHAMP
    69, // MONSTER_BELLSPROUT
    70, // MONSTER_WEEPINBELL
    71, // MONSTER_VICTREEBEL
    72, // MONSTER_TENTACOOL
    73, // MONSTER_TENTACRUEL
    74, // MONSTER_GEODUDE
    75, // MONSTER_GRAVELER
    76, // MONSTER_GOLEM
    77, // MONSTER_PONYTA
    78, // MONSTER_RAPIDASH
    79, // MONSTER_SLOWPOKE
    80, // MONSTER_SLOWBRO
    81, // MONSTER_MAGNEMITE
    82, // MONSTER_MAGNETON
    83, // MONSTER_FARFETCHD
    84, // MONSTER_DODUO
    85, // MONSTER_DODRIO
    86, // MONSTER_SEEL
    87, // MONSTER_DEWGONG
    88, // MONSTER_GRIMER
    89, // MONSTER_MUK
    90, // MONSTER_SHELLDER
    91, // MONSTER_CLOYSTER
    92, // MONSTER_GASTLY
    93, // MONSTER_HAUNTER
    94, // MONSTER_GENGAR
    95, // MONSTER_ONIX
    96, // MONSTER_DROWZEE
    97, // MONSTER_HYPNO
    98, // MONSTER_KRABBY
    99, // MONSTER_KINGLER
    100, // MONSTER_VOLTORB
    101, // MONSTER_ELECTRODE
    102, // MONSTER_EXEGGCUTE
    103, // MONSTER_EXEGGUTOR
    104, // MONSTER_CUBONE
    105, // MONSTER_MAROWAK
    106, // MONSTER_HITMONLEE
    107, // MONSTER_HITMONCHAN
    108, // MONSTER_LICKITUNG
    109, // MONSTER_KOFFING
    110, // MONSTER_WEEZING
    111, // MONSTER_RHYHORN
    112, // MONSTER_RHYDON
    113, // MONSTER_CHANSEY
    114, // MONSTER_TANGELA
    115, // MONSTER_KANGASKHAN
    116, // MONSTER_HORSEA
    117, // MONSTER_SEADRA
    118, // MONSTER_GOLDEEN
    119, // MONSTER_SEAKING
    120, // MONSTER_STARYU
    121, // MONSTER_STARMIE
    122, // MONSTER_MR_MIME
    123, // MONSTER_SCYTHER
    124, // MONSTER_JYNX
    125, // MONSTER_ELECTABUZZ
    126, // MONSTER_MAGMAR
    127, // MONSTER_PINSIR
    128, // MONSTER_TAUROS
    129, // MONSTER_MAGIKARP
    130, // MONSTER_GYARADOS
    131, // MONSTER_LAPRAS
    132, // MONSTER_DITTO
    133, // MONSTER_EEVEE
    134, // MONSTER_VAPOREON
    135, // MONSTER_JOLTEON
    136, // MONSTER_FLAREON
    137, // MONSTER_PORYGON
    138, // MONSTER_OMANYTE
    139, // MONSTER_OMASTAR
    140, // MONSTER_KABUTO
    141, // MONSTER_KABUTOPS
    142, // MONSTER_AERODACTYL
    143, // MONSTER_SNORLAX
    144, // MONSTER_ARTICUNO
    145, // MONSTER_ZAPDOS
    146, // MONSTER_MOLTRES
    147, // MONSTER_DRATINI
    148, // MONSTER_DRAGONAIR
    149, // MONSTER_DRAGONITE
    150, // MONSTER_MEWTWO
    151, // MONSTER_MEW
    152, // MONSTER_CHIKORITA
    153, // MONSTER_BAYLEEF
    154, // MONSTER_MEGANIUM
    155, // MONSTER_CYNDAQUIL
    156, // MONSTER_QUILAVA
    157, // MONSTER_TYPHLOSION
    158, // MONSTER_TOTODILE
    159, // MONSTER_CROCONAW
    160, // MONSTER_FERALIGATR
    161, // MONSTER_SENTRET
    162, // MONSTER_FURRET
    163, // MONSTER_HOOTHOOT
    164, // MONSTER_NOCTOWL
    165, // MONSTER_LEDYBA
    166, // MONSTER_LEDIAN
    167, // MONSTER_SPINARAK
    168, // MONSTER_ARIADOS
    169, // MONSTER_CROBAT
    170, // MONSTER_CHINCHOU
    171, // MONSTER_LANTURN
    172, // MONSTER_PICHU
    173, // MONSTER_CLEFFA
    174, // MONSTER_IGGLYBUFF
    175, // MONSTER_TOGEPI
    176, // MONSTER_TOGETIC
    177, // MONSTER_NATU
    178, // MONSTER_XATU
    179, // MONSTER_MAREEP
    180, // MONSTER_FLAAFFY
    181, // MONSTER_AMPHAROS
    182, // MONSTER_BELLOSSOM
    183, // MONSTER_MARILL
    184, // MONSTER_AZUMARILL
    185, // MONSTER_SUDOWOODO
    186, // MONSTER_POLITOED
    187, // MONSTER_HOPPIP
    188, // MONSTER_SKIPLOOM
    189, // MONSTER_JUMPLUFF
    190, // MONSTER_AIPOM
    191, // MONSTER_SUNKERN
    192, // MONSTER_SUNFLORA
    193, // MONSTER_YANMA
    194, // MONSTER_WOOPER
    195, // MONSTER_QUAGSIRE
    196, // MONSTER_ESPEON
    197, // MONSTER_UMBREON
    198, // MONSTER_MURKROW
    199, // MONSTER_SLOWKING
    200, // MONSTER_MISDREAVUS
    201, // MONSTER_UNOWN_A
    202, // MONSTER_UNOWN_B
    203, // MONSTER_UNOWN_C
    204, // MONSTER_UNOWN_D
    205, // MONSTER_UNOWN_E
    206, // MONSTER_UNOWN_F
    207, // MONSTER_UNOWN_G
    208, // MONSTER_UNOWN_H
    209, // MONSTER_UNOWN_I
    210, // MONSTER_UNOWN_J
    211, // MONSTER_UNOWN_K
    212, // MONSTER_UNOWN_L
    213, // MONSTER_UNOWN_M
    214, // MONSTER_UNOWN_N
    215, // MONSTER_UNOWN_O
    216, // MONSTER_UNOWN_P
    217, // MONSTER_UNOWN_Q
    218, // MONSTER_UNOWN_R
    219, // MONSTER_UNOWN_S
    220, // MONSTER_UNOWN_T
    221, // MONSTER_UNOWN_U
    222, // MONSTER_UNOWN_V
    223, // MONSTER_UNOWN_W
    224, // MONSTER_UNOWN_X
    225, // MONSTER_UNOWN_Y
    226, // MONSTER_UNOWN_Z
    229, // MONSTER_WOBBUFFET
    230, // MONSTER_GIRAFARIG
    231, // MONSTER_PINECO
    232, // MONSTER_FORRETRESS
    233, // MONSTER_DUNSPARCE
    234, // MONSTER_GLIGAR
    235, // MONSTER_STEELIX
    236, // MONSTER_SNUBBULL
    237, // MONSTER_GRANBULL
    238, // MONSTER_QWILFISH
    239, // MONSTER_SCIZOR
    240, // MONSTER_SHUCKLE
    241, // MONSTER_HERACROSS
    242, // MONSTER_SNEASEL
    243, // MONSTER_TEDDIURSA
    244, // MONSTER_URSARING
    245, // MONSTER_SLUGMA
    246, // MONSTER_MAGCARGO
    247, // MONSTER_SWINUB
    248, // MONSTER_PILOSWINE
    249, // MONSTER_CORSOLA
    250, // MONSTER_REMORAID
    251, // MONSTER_OCTILLERY
    252, // MONSTER_DELIBIRD
    253, // MONSTER_MANTINE
    254, // MONSTER_SKARMORY
    255, // MONSTER_HOUNDOUR
    256, // MONSTER_HOUNDOOM
    257, // MONSTER_KINGDRA
    258, // MONSTER_PHANPY
    259, // MONSTER_DONPHAN
    260, // MONSTER_PORYGON2
    261, // MONSTER_STANTLER
    262, // MONSTER_SMEARGLE
    263, // MONSTER_TYROGUE
    264, // MONSTER_HITMONTOP
    265, // MONSTER_SMOOCHUM
    266, // MONSTER_ELEKID
    267, // MONSTER_MAGBY
    268, // MONSTER_MILTANK
    269, // MONSTER_BLISSEY
    270, // MONSTER_RAIKOU
    271, // MONSTER_ENTEI
    272, // MONSTER_SUICUNE
    273, // MONSTER_LARVITAR
    274, // MONSTER_PUPITAR
    275, // MONSTER_TYRANITAR
    276, // MONSTER_LUGIA
    277, // MONSTER_HO_OH
    278, // MONSTER_CELEBI
    280, // MONSTER_TREECKO
    281, // MONSTER_GROVYLE
    282, // MONSTER_SCEPTILE
    283, // MONSTER_TORCHIC
    284, // MONSTER_COMBUSKEN
    285, // MONSTER_BLAZIKEN
    286, // MONSTER_MUDKIP
    287, // MONSTER_MARSHTOMP
    288, // MONSTER_SWAMPERT
    289, // MONSTER_POOCHYENA
    290, // MONSTER_MIGHTYENA
    291, // MONSTER_ZIGZAGOON
    292, // MONSTER_LINOONE
    293, // MONSTER_WURMPLE
    294, // MONSTER_SILCOON
    295, // MONSTER_BEAUTIFLY
    296, // MONSTER_CASCOON
    297, // MONSTER_DUSTOX
    298, // MONSTER_LOTAD
    299, // MONSTER_LOMBRE
    300, // MONSTER_LUDICOLO
    301, // MONSTER_SEEDOT
    302, // MONSTER_NUZLEAF
    303, // MONSTER_SHIFTRY
    304, // MONSTER_TAILLOW
    305, // MONSTER_SWELLOW
    306, // MONSTER_WINGULL
    307, // MONSTER_PELIPPER
    308, // MONSTER_RALTS
    309, // MONSTER_KIRLIA
    310, // MONSTER_GARDEVOIR
    311, // MONSTER_SURSKIT
    312, // MONSTER_MASQUERAIN
    313, // MONSTER_SHROOMISH
    314, // MONSTER_BRELOOM
    315, // MONSTER_SLAKOTH
    316, // MONSTER_VIGOROTH
    317, // MONSTER_SLAKING
    318, // MONSTER_NINCADA
    319, // MONSTER_NINJASK
    320, // MONSTER_SHEDINJA
    321, // MONSTER_WHISMUR
    322, // MONSTER_LOUDRED
    323, // MONSTER_EXPLOUD
    324, // MONSTER_MAKUHITA
    325, // MONSTER_HARIYAMA
    326, // MONSTER_AZURILL
    327, // MONSTER_NOSEPASS
    328, // MONSTER_SKITTY
    329, // MONSTER_DELCATTY
    330, // MONSTER_SABLEYE
    331, // MONSTER_MAWILE
    332, // MONSTER_ARON
    333, // MONSTER_LAIRON
    334, // MONSTER_AGGRON
    335, // MONSTER_MEDITITE
    336, // MONSTER_MEDICHAM
    337, // MONSTER_ELECTRIKE
    338, // MONSTER_MANECTRIC
    339, // MONSTER_PLUSLE
    340, // MONSTER_MINUN
    341, // MONSTER_VOLBEAT
    342, // MONSTER_ILLUMISE
    343, // MONSTER_ROSELIA
    344, // MONSTER_GULPIN
    345, // MONSTER_SWALOT
    346, // MONSTER_CARVANHA
    347, // MONSTER_SHARPEDO
    348, // MONSTER_WAILMER
    349, // MONSTER_WAILORD
    350, // MONSTER_NUMEL
    351, // MONSTER_CAMERUPT
    352, // MONSTER_TORKOAL
    353, // MONSTER_SPOINK
    354, // MONSTER_GRUMPIG
    355, // MONSTER_SPINDA
    356, // MONSTER_TRAPINCH
    357, // MONSTER_VIBRAVA
    358, // MONSTER_FLYGON
    359, // MONSTER_CACNEA
    360, // MONSTER_CACTURNE
    361, // MONSTER_SWABLU
    362, // MONSTER_ALTARIA
    363, // MONSTER_ZANGOOSE
    364, // MONSTER_SEVIPER
    365, // MONSTER_LUNATONE
    366, // MONSTER_SOLROCK
    367, // MONSTER_BARBOACH
    368, // MONSTER_WHISCASH
    369, // MONSTER_CORPHISH
    370, // MONSTER_CRAWDAUNT
    371, // MONSTER_BALTOY
    372, // MONSTER_CLAYDOL
    373, // MONSTER_LILEEP
    374, // MONSTER_CRADILY
    375, // MONSTER_ANORITH
    376, // MONSTER_ARMALDO
    377, // MONSTER_FEEBAS
    378, // MONSTER_MILOTIC
    379, // MONSTER_CASTFORM
    380, // MONSTER_CASTFORM_SNOWY
    381, // MONSTER_CASTFORM_SUNNY
    382, // MONSTER_CASTFORM_RAINY
    383, // MONSTER_KECLEON
    385, // MONSTER_SHUPPET
    386, // MONSTER_BANETTE
    387, // MONSTER_DUSKULL
    388, // MONSTER_DUSCLOPS
    389, // MONSTER_TROPIUS
    390, // MONSTER_CHIMECHO
    391, // MONSTER_ABSOL
    392, // MONSTER_WYNAUT
    393, // MONSTER_SNORUNT
    394, // MONSTER_GLALIE
    395, // MONSTER_SPHEAL
    396, // MONSTER_SEALEO
    397, // MONSTER_WALREIN
    398, // MONSTER_CLAMPERL
    399, // MONSTER_HUNTAIL
    400, // MONSTER_GOREBYSS
    401, // MONSTER_RELICANTH
    402, // MONSTER_LUVDISC
    403, // MONSTER_BAGON
    404, // MONSTER_SHELGON
    405, // MONSTER_SALAMENCE
    406, // MONSTER_BELDUM
    407, // MONSTER_METANG
    408, // MONSTER_METAGROSS
    409, // MONSTER_REGIROCK
    410, // MONSTER_REGICE
    411, // MONSTER_REGISTEEL
    412, // MONSTER_LATIAS
    413, // MONSTER_LATIOS
    414, // MONSTER_KYOGRE
    415, // MONSTER_GROUDON
    416, // MONSTER_RAYQUAZA
    417, // MONSTER_JIRACHI
    418, // MONSTER_DEOXYS_NORMAL
    227, // MONSTER_UNOWN_EMARK
    228, // MONSTER_UNOWN_QMARK
    419, // MONSTER_DEOXYS_ATTACK
    420, // MONSTER_DEOXYS_DEFENSE
    421, // MONSTER_DEOXYS_SPEED
    488, // MONSTER_MUNCHLAX
    0, // MONSTER_DECOY
    0, // MONSTER_STATUE
    416, // MONSTER_RAYQUAZA_CUTSCENE
};

static void WriteTransferBits(u8 *buffer, s32 bitPos, u32 value, s32 numBits)
{
    s32 i;

    for (i = 0; i < numBits; i++) {
        if (value & ((u32)1 << i))
            buffer[(bitPos + i) >> 3] |= 1 << ((bitPos + i) & 7);
    }
}

static u32 ReadTransferBits(const u8 *buffer, s32 bitPos, s32 numBits)
{
    s32 i;
    u32 value = 0;

    for (i = 0; i < numBits; i++) {
        if (buffer[(bitPos + i) >> 3] & (1 << ((bitPos + i) & 7)))
            value |= (u32)1 << i;
    }

    return value;
}

static u32 TransferCrc32(const u8 *data, s32 size)
{
    s32 i, j;
    u32 crc = 0xFFFFFFFF;

    for (i = 0; i < size; i++) {
        crc ^= data[i];
        for (j = 0; j < 8; j++) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
    }

    return ~crc;
}

// XOR-ing twice with the same seed restores the original payload
static void ScrambleTransferPayload(u8 *payload, u32 seed)
{
    s32 i;

    for (i = 0; i < TRANSFER_PAYLOAD_SIZE; i++) {
        seed = seed * 0x41C64E6D + 0x3039;
        payload[i] ^= seed >> 24;
    }
}

void EncodeTransferPassword(const TransferData *data, u8 *password)
{
    s32 i;
    u32 crc;
    u8 bits[TRANSFER_BITS_SIZE];

    MemoryFill8(bits, 0, sizeof(bits));
    WriteTransferBits(bits, 0, TRANSFER_PASSWORD_VERSION, 4);
    WriteTransferBits(bits, 4, data->heroSpecies, 10);
    WriteTransferBits(bits, 14, data->heroLevel, 7);
    WriteTransferBits(bits, 21, data->partnerSpecies, 10);
    WriteTransferBits(bits, 31, data->partnerLevel, 7);
    WriteTransferBits(bits, 38, data->money, 17);
    WriteTransferBits(bits, 55, data->savings, 24);

    crc = TransferCrc32(bits, TRANSFER_PAYLOAD_SIZE);
    ScrambleTransferPayload(bits, crc);
    WriteTransferBits(bits, TRANSFER_CRC_BIT_POS, crc, 32);

    for (i = 0; i < TRANSFER_PASSWORD_LENGTH; i++)
        password[i] = sTransferAlphabet[ReadTransferBits(bits, i * 5, 5)];
}

bool8 DecodeTransferPassword(const u8 *password, TransferData *data)
{
    s32 i, j;
    u32 crc;
    u8 bits[TRANSFER_BITS_SIZE];

    MemoryFill8(bits, 0, sizeof(bits));
    for (i = 0; i < TRANSFER_PASSWORD_LENGTH; i++) {
        for (j = 0; j < 32; j++) {
            if (password[i] == sTransferAlphabet[j])
                break;
        }
        if (j == 32)
            return FALSE;
        WriteTransferBits(bits, i * 5, j, 5);
    }
    if (password[TRANSFER_PASSWORD_LENGTH] != '\0')
        return FALSE;

    if (ReadTransferBits(bits, TRANSFER_PAD_BIT_POS, 2) != 0)
        return FALSE;

    crc = ReadTransferBits(bits, TRANSFER_CRC_BIT_POS, 32);
    ScrambleTransferPayload(bits, crc);
    if (TransferCrc32(bits, TRANSFER_PAYLOAD_SIZE) != crc)
        return FALSE;
    if (ReadTransferBits(bits, 0, 4) != TRANSFER_PASSWORD_VERSION)
        return FALSE;
    for (i = 79; i < TRANSFER_CRC_BIT_POS; i++) {
        if (ReadTransferBits(bits, i, 1) != 0)
            return FALSE;
    }

    data->heroSpecies = ReadTransferBits(bits, 4, 10);
    data->heroLevel = ReadTransferBits(bits, 14, 7);
    data->partnerSpecies = ReadTransferBits(bits, 21, 10);
    data->partnerLevel = ReadTransferBits(bits, 31, 7);
    data->money = ReadTransferBits(bits, 38, 17);
    data->savings = ReadTransferBits(bits, 55, 24);

    if (data->heroSpecies == 0 || data->heroSpecies > TRANSFER_MAX_SKY_SPECIES)
        return FALSE;
    if (data->partnerSpecies == 0 || data->partnerSpecies > TRANSFER_MAX_SKY_SPECIES)
        return FALSE;
    if (data->heroLevel == 0 || data->heroLevel > TRANSFER_MAX_LEVEL)
        return FALSE;
    if (data->partnerLevel == 0 || data->partnerLevel > TRANSFER_MAX_LEVEL)
        return FALSE;
    if (data->money > MAX_TEAM_MONEY || data->savings > MAX_TEAM_SAVINGS)
        return FALSE;

    return TRUE;
}

s16 GetSkySpeciesId(s16 species)
{
    if (species < 0 || species >= MONSTER_MAX)
        return 0;
    return sSkySpeciesIds[species];
}

static u8 ClampTransferLevel(s32 level)
{
    if (level < 1)
        return 1;
    if (level > TRANSFER_MAX_LEVEL)
        return TRANSFER_MAX_LEVEL;
    return level;
}

static s32 ClampTransferMoney(s32 money, s32 max)
{
    if (money < 0)
        return 0;
    if (money > max)
        return max;
    return money;
}

// The transfer unlocks once the credits have rolled
bool8 CanTransferToSky(void)
{
    TransferData data;

    if (!CheckQuest(QUEST_REACHED_POSTGAME))
        return FALSE;
    return BuildTransferData(&data);
}

// Uses the original hero and partner, even if the team leader was changed after the story
bool8 BuildTransferData(TransferData *data)
{
    Pokemon *hero = GetLeaderMon2();
    Pokemon *partner = GetPartnerMon2();

    if (hero == NULL || partner == NULL)
        return FALSE;

    data->heroSpecies = GetSkySpeciesId(hero->speciesNum);
    data->partnerSpecies = GetSkySpeciesId(partner->speciesNum);
    if (data->heroSpecies == 0 || data->partnerSpecies == 0)
        return FALSE;

    data->heroLevel = ClampTransferLevel(hero->level);
    data->partnerLevel = ClampTransferLevel(partner->level);
    data->money = ClampTransferMoney(gTeamInventoryRef->teamMoney, MAX_TEAM_MONEY);
    data->savings = ClampTransferMoney(gTeamInventoryRef->teamSavings, MAX_TEAM_SAVINGS);
    return TRUE;
}
