# Design references

The pictures the round watch's interface is checked against. They are reference only: nothing here is built into
firmware or shipped. They belong to Google and their authors, and are kept here so that every design pass and every
agent works from the same source instead of a copy in someone's Downloads folder.

## `wear-os/` — Wear OS design guidelines (Material 3 Expressive)

Images and GIFs from Google's Wear OS design guides, saved from 92 pages of
[developer.android.com/design/ui/wear](https://developer.android.com/design/ui/wear/guides/get-started).
`sources.json` maps each folder to the page it came from; `manifest.json` lists every file with its size and SHA-256.

| Folder | What to look at |
|---|---|
| `get-started/` | The principles: embrace the round screen, rich colour, edge-hugging containers, shape |
| `foundations/` | Adaptive layouts (192–240 dp), common layouts, quality tiers |
| `surfaces/apps/` | App layouts: scrolling lists of full-width pill cards, title under the time, edge button at the bottom |
| `surfaces/tiles/` | Tiles: the weather tile (sky + temperature + hourly row), progress tiles, empty and error states |
| `patterns/` | Media controls, gestures, theming by artwork colour |
| `styles/` | Colour (semantic meaning, accessibility), typography |
| `m2-5/` | The older Material 2.5 guides: physical buttons, permissions, scrolling and non-scrolling GIFs |

## `pixel-watch-faces/` — Google Pixel Watch faces, all generations

Pixel Watch 1–5 faces (`Pixel-Watch-1/` … `Pixel-Watch-5/`) and the full set as shown on a Watch 4
(`All-generations-on-Watch-4/`). `manifest.json` lists every file with its size and SHA-256. They are what the face
library and the face widgets follow: rings that hug the rim with a gap, tonal discs, one hue family per face, big
condensed digits.

## How the firmware uses them

- **Lists that scroll** (`upager.h` list mode, the gallery, Weather) follow `surfaces/apps/layouts/scrolling/`: cards
  410 px wide (≈ 205 dp), full pills, 8 px apart, shrinking to 0.6 and fading at the top and bottom of the circle.
- **Weather** (`wxapp.h`) follows the weather tile in `surfaces/tiles/` (sky beside the temperature, a row of hours)
  and the progress cards in `surfaces/apps/layouts/scrolling/app-layout-scrolling-custom-cards.png`; the rim's arc
  from today's low to high is the Pixel faces' rim ring.
- **The face** (`face.h`, `widgets.h`) follows `pixel-watch-faces/`: the battery pill over the date, rings along the
  rim.

When a screen looks wrong, put it next to the nearest reference here before changing numbers.
