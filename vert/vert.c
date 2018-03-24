/*

  vert.c

  Copyright 2018 Matthew T. Pandina. All rights reserved.

  This file is part of the Uzebox Mode 3 with Scrolling Guide:
  https://github.com/artcfox/uzebox-mode-3-with-scrolling-guide

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <stdlib.h>
#include <string.h>
#include <avr/pgmspace.h>
#include <uzebox.h>

#include "data/tileset.inc"

typedef struct {
  uint16_t width;
  uint16_t height;
  uint16_t offset;
  const char *data;
} __attribute__ ((packed)) LEVEL;

void Level_initFromMap(LEVEL *l, const char *map)
{
  l->width = pgm_read_byte(&map[0]);
  l->height = pgm_read_byte(&map[1]);
  l->offset = 2;
  l->data = map;
}

static inline uint8_t Level_getTileAt(LEVEL *l, uint16_t index)
{
  return pgm_read_byte(&l->data[l->offset + index]);
}

void Level_drawRow(LEVEL *l, uint8_t y, int16_t realY)
{
  if (realY < 0 || (uint16_t)realY > l->height - 1)
    return;

  uint8_t ty = y % VRAM_TILES_V;

  for (uint8_t i = 0; i < SCREEN_TILES_H; ++i) {
    uint16_t index = realY * l->width + i;
    SetTile(i, ty, Level_getTileAt(l, index));
  }
}

typedef struct {
  int16_t y;
  LEVEL *level;
} __attribute__ ((packed)) CAMERA;

void Camera_init(CAMERA *c, LEVEL *l)
{
  c->level = l;
  c->y = 0;
  Screen.scrollX = 0;
  Screen.scrollY = (uint8_t)c->y;
}

void Camera_moveTo(CAMERA *c, int16_t y)
{
  if (y < 0) {
    c->y = 0;
  } else {
    int16_t yMax = (c->level->height - SCREEN_TILES_V) * TILE_HEIGHT;
    if (y > yMax)
      c->y = yMax;
    else
      c->y = y;
  }
}

void Camera_fillVram(CAMERA *c)
{
  int16_t cyt = c->y / TILE_HEIGHT;

  for (uint8_t i = 0; i < VRAM_TILES_V - 2; ++i)
    Level_drawRow(c->level, cyt + i, cyt + i);

  Level_drawRow(c->level, cyt + 30, cyt - 2);
  Level_drawRow(c->level, cyt + 31, cyt - 1);
}

void Camera_update(CAMERA *c)
{
  uint8_t prevY = Screen.scrollY;
  Screen.scrollY = (uint8_t)c->y;

  // Have we scrolled past a tile boundary?
  if ((prevY & ~(TILE_HEIGHT - 1)) != (Screen.scrollY & ~(TILE_HEIGHT - 1))) {
    int16_t cyt = c->y / TILE_HEIGHT;
    if ((uint8_t)(Screen.scrollY - prevY) < (uint8_t)(prevY - Screen.scrollY))
      Level_drawRow(c->level, cyt + 29, cyt + 29);
    else
      Level_drawRow(c->level, cyt + 30, cyt - 2);
  }
}

int main()
{
  SetTileTable(tileset);

  LEVEL l;
  CAMERA c;

  ClearVram();
  Level_initFromMap(&l, map_vert_level);
  Camera_init(&c, &l);
  Camera_fillVram(&c);

  for (;;) {
    uint16_t held = ReadJoypad(0);
    if (held & BTN_UP)
      Camera_moveTo(&c, c.y - 1);
    else if (held & BTN_DOWN)
      Camera_moveTo(&c, c.y + 1);

    Camera_update(&c);

    WaitVsync(1);
  }
}
