# Transfer to Explorers of Sky ("New Game+")

This mod lets you carry your **hero, partner and money** from Red Rescue Team into a new game of
Explorers of Sky, built from the [pmd-sky](https://github.com/YT-was-X/pmd-sky) decompilation with the
matching changes.

## How it works

1. Beat Red Rescue Team (watch the credits) and save.
2. On the title screen, **Transfer** now appears at the bottom of the menu.
3. It shows a 34-character **Transfer Password** along with your team's levels, carried money and bank savings.
4. In Explorers of Sky (with no save file), pick **Transfer** on the Top Menu, enter the password, and take the
   personality quiz. You become the transferred hero, and the transferred partner is offered as your partner.
   Both keep their levels, and your money and savings carry over.

The original hero and partner are used even if you changed the team leader after the story.
Pokémon are matched to Explorers of Sky by species (Castform and Deoxys forms are kept).

## Building

Build as usual with `make` (see [INSTALL.md](INSTALL.md)), or with `make MODERN=1`.
The ROM no longer matches the original, so `make compare` will fail.

The new code is placed at the end of the main ROM section in `ld_script.ld` so the rest of the ROM
moves as little as possible. If the default build misbehaves, try `make MODERN=1`.

## Files

- `src/transfer_password.c`: the password format (shared with pmd-sky, keep both in sync) and the Red → Sky species table.
- `src/sky_transfer_menu.c`: the screen that shows the password.
- `src/main_menu2.c`: adds **Transfer** to the title screen menus after the story is beaten.

pmd-sky has a reference Python implementation at `tools/transfer_password/transfer_password.py`
that can encode and decode passwords for testing.
