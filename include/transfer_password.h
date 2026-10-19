#ifndef GUARD_TRANSFER_PASSWORD_H
#define GUARD_TRANSFER_PASSWORD_H

// "New Game+" transfer to Pokémon Mystery Dungeon: Explorers of Sky.
// The password format is shared with pmd-sky (src/transfer_password.c), so keep both files in sync.

#define TRANSFER_PASSWORD_LENGTH 34
#define TRANSFER_PASSWORD_VERSION 1

typedef struct TransferData
{
    /* 0x0 */ s16 heroSpecies; // Explorers of Sky species ID
    /* 0x2 */ s16 partnerSpecies; // Explorers of Sky species ID
    /* 0x4 */ u8 heroLevel;
    /* 0x5 */ u8 partnerLevel;
    /* 0x8 */ s32 money;
    /* 0xC */ s32 savings;
} TransferData;

void EncodeTransferPassword(const TransferData *data, u8 *password);
bool8 DecodeTransferPassword(const u8 *password, TransferData *data);

bool8 CanTransferToSky(void);
bool8 BuildTransferData(TransferData *data);
s16 GetSkySpeciesId(s16 species);

#endif // GUARD_TRANSFER_PASSWORD_H
