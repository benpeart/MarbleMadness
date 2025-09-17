#include "main.h"
#include "debug.h"
#include "settings.h"
#include "render.h"
#include "MarbleTrack.h"

#define DEFAULT_MILLIS 75
#define MIN_MILLIS 0
#define MAX_MILLIS (4 * DEFAULT_MILLIS)

static CRGB marble_colors[] = {
    CRGB::AliceBlue, CRGB::Amethyst, CRGB::AntiqueWhite, CRGB::Aqua, CRGB::Aquamarine, CRGB::Azure,
    CRGB::Beige, CRGB::Bisque, CRGB::BlanchedAlmond, CRGB::Blue, CRGB::BlueViolet,
    CRGB::Brown, CRGB::BurlyWood, CRGB::CadetBlue, CRGB::Chartreuse, CRGB::Chocolate, CRGB::Coral,
    CRGB::CornflowerBlue, CRGB::Cornsilk, CRGB::Crimson, CRGB::Cyan, CRGB::DarkBlue, CRGB::DarkCyan,
    CRGB::DarkGoldenrod, CRGB::DarkGray, CRGB::DarkGreen, CRGB::DarkKhaki, CRGB::DarkMagenta, CRGB::DarkOliveGreen,
    CRGB::DarkOrange, CRGB::DarkOrchid, CRGB::DarkRed, CRGB::DarkSalmon, CRGB::DarkSeaGreen, CRGB::DarkSlateBlue,
    CRGB::DarkSlateGray, CRGB::DarkTurquoise, CRGB::DarkViolet, CRGB::DeepPink, CRGB::DeepSkyBlue, CRGB::DimGray,
    CRGB::DodgerBlue, CRGB::FireBrick, CRGB::FloralWhite, CRGB::ForestGreen, CRGB::Fuchsia, CRGB::Gainsboro,
    CRGB::GhostWhite, CRGB::Gold, CRGB::Goldenrod, CRGB::Gray, CRGB::Green, CRGB::GreenYellow,
    CRGB::Honeydew, CRGB::HotPink, CRGB::IndianRed, CRGB::Indigo, CRGB::Ivory, CRGB::Khaki,
    CRGB::Lavender, CRGB::LavenderBlush, CRGB::LawnGreen, CRGB::LemonChiffon, CRGB::LightBlue, CRGB::LightCoral,
    CRGB::LightCyan, CRGB::LightGoldenrodYellow, CRGB::LightGreen, CRGB::LightGrey, CRGB::LightPink, CRGB::LightSalmon,
    CRGB::LightSeaGreen, CRGB::LightSkyBlue, CRGB::LightSlateGray, CRGB::LightSteelBlue, CRGB::LightYellow, CRGB::Lime,
    CRGB::LimeGreen, CRGB::Linen, CRGB::Magenta, CRGB::Maroon, CRGB::MediumAquamarine, CRGB::MediumBlue,
    CRGB::MediumOrchid, CRGB::MediumPurple, CRGB::MediumSeaGreen, CRGB::MediumSlateBlue, CRGB::MediumSpringGreen, CRGB::MediumTurquoise,
    CRGB::MediumVioletRed, CRGB::MidnightBlue, CRGB::MintCream, CRGB::MistyRose, CRGB::Moccasin, CRGB::NavajoWhite,
    CRGB::Navy, CRGB::OldLace, CRGB::Olive, CRGB::OliveDrab, CRGB::Orange, CRGB::OrangeRed,
    CRGB::Orchid, CRGB::PaleGoldenrod, CRGB::PaleGreen, CRGB::PaleTurquoise, CRGB::PaleVioletRed, CRGB::PapayaWhip,
    CRGB::PeachPuff, CRGB::Peru, CRGB::Pink, CRGB::Plum, CRGB::PowderBlue, CRGB::Purple,
    CRGB::Red, CRGB::RosyBrown, CRGB::RoyalBlue, CRGB::SaddleBrown, CRGB::Salmon, CRGB::SandyBrown,
    CRGB::SeaGreen, CRGB::Seashell, CRGB::Sienna, CRGB::Silver, CRGB::SkyBlue, CRGB::SlateBlue,
    CRGB::SlateGray, CRGB::Snow, CRGB::SpringGreen, CRGB::SteelBlue, CRGB::Tan, CRGB::Teal,
    CRGB::Thistle, CRGB::Tomato, CRGB::Turquoise, CRGB::Violet, CRGB::Wheat, 
    CRGB::WhiteSmoke, CRGB::Yellow, CRGB::YellowGreen};
static int marble_count = (sizeof(marble_colors) / sizeof(marble_colors[0])); // total number of valid marble colors in table

static bool is_marble(CRGB color)
{
    for (int i = 0; i < marble_count; i++)
    {
        if (color == marble_colors[i])
            return true;
    }
    return false;
}

void marbletrack_enter()
{
    DB_PRINTLN("Entering MarbleTrack mode");

    // draw the track
    /*
    x=0 -> 17, y=0
    x=0 -> 17, y=1
    x=1 -> 18, y=3
    x=0 -> 17, y=5
    x=1 -> 18, y=7
    */
    for (int y = 0; y < NUM_ROWS; y += 2)
    {
        if (y - 1 % 4 == 0)
        {
            for (int x = 0; x < NUM_COLS - 1; x++)
            {
                leds[XY(x, y)] = CRGB::White;
            }
        }
        else
        {
            for (int x = 1; x < NUM_COLS; x++)
            {
                leds[XY(x, y)] = CRGB::White;
            }
        }
    }
    leds_dirty = true;
}

void marbletrack_loop()
{
    EVERY_N_MILLIS_I(timer, DEFAULT_MILLIS)
    {
        timer.setPeriod(MAX_MILLIS - map(settings.speed, MIN_SPEED, MAX_SPEED, MIN_MILLIS, MAX_MILLIS));

        // start with last LED to allow proper overlapping
        bool emptyScreen = true;
        for (int i = 0; i < NUM_LEDS; i++)
        {
            // if this is a marble
            if (is_marble(leds[i]))
            {
                // if not on the last LED, add a new marble in the next position of the same color
                if (i > 0)
                {
                    leds[i - 1] = leds[i];
                    emptyScreen = false;
                }

                // turn the old marble position into the beginning of a trail
                leds[i].fadeToBlackBy(191); // Dim by 75%
            }
            else
            {
                // Fade all trailing LEDs
//                leds[i].fadeToBlackBy(191); // Dim by 50%
            }
        }

        // spawn new falling marble at beginning of run
        if (random8(4 * NUM_COLS) == 0 || emptyScreen) // lower number == more frequent spawns
            leds[NUM_LEDS - 1] = marble_colors[random8(marble_count)];

        leds_dirty = true;
    }
}
