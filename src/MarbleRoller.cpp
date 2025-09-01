#include "main.h"
#include "debug.h"
#include "settings.h"
#include "render.h"
#include "MarbleRoller.h"

#define DEFAULT_MILLIS 75
#define MIN_MILLIS 0
#define MAX_MILLIS (4 * DEFAULT_MILLIS)

CRGB marble_colors[] = {
    CRGB::AliceBlue, CRGB::Amethyst, CRGB::AntiqueWhite, CRGB::Aqua, CRGB::Aquamarine, CRGB::Azure,
    CRGB::Beige, CRGB::Bisque, CRGB::Black, CRGB::BlanchedAlmond, CRGB::Blue, CRGB::BlueViolet,
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
    CRGB::Thistle, CRGB::Tomato, CRGB::Turquoise, CRGB::Violet, CRGB::Wheat, CRGB::White,
    CRGB::WhiteSmoke, CRGB::Yellow, CRGB::YellowGreen};
int marble_count = (sizeof(marble_colors) / sizeof(marble_colors[0])); // total number of valid marble colors in table

bool is_marble(CRGB color)
{
    for (int i = 0; i < marble_count; i++)
    {
        if (color == marble_colors[i])
            return true;
    }
    return false;
}

void mode_marbleroller()
{
    EVERY_N_MILLIS_I(timer, DEFAULT_MILLIS)
    {
        timer.setPeriod(MAX_MILLIS - map(settings.speed, MIN_SPEED, MAX_SPEED, MIN_MILLIS, MAX_MILLIS));

        // start with last LED to allow proper overlapping
        bool emptyScreen = true;
        for (int i = 0; i < NUM_STRIPS * NUM_LEDS_PER_STRIP; i++)
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
                leds[i].fadeToBlackBy(64); // Dim by 75%
            }
            else
            {
                // Fade all trailing LEDs
                leds[i].fadeToBlackBy(191); // Dim by 25%
            }
        }

        // spawn new falling marble at beginning of run
        if (random8(4 * NUM_COLS) == 0 || emptyScreen) // lower number == more frequent spawns
            leds[NUM_STRIPS * NUM_LEDS_PER_STRIP - 1] = marble_colors[random8(marble_count)];

        leds_dirty = true;
    }
}
