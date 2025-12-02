#include "main.h"
#include "debug.h"
#include "settings.h"
#include "render.h"
#include "life.h"
#include "displaynumbers.h"

#define DEFAULT_MILLIS 256
#define MIN_MILLIS 0
#define MAX_MILLIS (4 * DEFAULT_MILLIS)

//
// Conway's Game of Life implementation
//
typedef struct
{
    size_t width, height;
    size_t nbits;  // total cells
    size_t nbytes; // bytes needed
    uint8_t *curr; // bit-packed current generation
    uint8_t *next; // bit-packed next generation
    bool torus;    // wrap edges?
} life_t;

static inline long idx_wrap(const life_t *life, long x, long y)
{
    long w = (long)life->width;
    long h = (long)life->height;
    if (life->torus)
    {
        if (x < 0)
            x += w;
        if (x >= w)
            x -= w;
        if (y < 0)
            y += h;
        if (y >= h)
            y -= h;
    }
    else
    {
        if (x < 0 || x >= w || y < 0 || y >= h)
            return -1;
    }
    return y * w + x;
}

// Write a cell state (0/1).
static inline void life_set(life_t *life, size_t x, size_t y, bool v)
{
    size_t idx = y * life->width + x;
    size_t byte = idx >> 3;
    size_t bit = idx & 7;
    if (v)
        life->curr[byte] |= (1 << bit);
    else
        life->curr[byte] &= ~(1 << bit);
}

// Read a cell state (0/1).
static inline uint8_t life_get(const life_t *life, long x, long y)
{
    long i = idx_wrap(life, x, y);
    if (i < 0)
        return 0; // out of bounds = dead
    size_t idx = (size_t)i;
    size_t byte = idx >> 3;
    size_t bit = idx & 7;
    return (life->curr[byte] >> bit) & 1;
}

int life_load_rle(life_t *life, const char *rle, size_t x0, size_t y0)
{
    size_t x = x0, y = y0;
    size_t run = 0;
    int in_header = 1;

    for (const char *p = rle; *p; ++p)
    {
        char c = *p;

        if (in_header)
        {
            if (c == '\n')
                in_header = 0;
            continue; // skip header lines until first newline
        }

        if (isdigit((unsigned char)c))
        {
            run = run * 10 + (size_t)(c - '0');
            continue;
        }

        if (c == 'b' || c == 'o')
        {
            size_t count = (run == 0) ? 1 : run;
            bool val = (c == 'o') ? 1 : 0;
            for (size_t i = 0; i < count; ++i)
            {
                if (x < life->width && y < life->height)
                {
                    life_set(life, x, y, val);
                }
                x++;
            }
            run = 0;
        }
        else if (c == '$')
        {
            size_t count = (run == 0) ? 1 : run;
            run = 0;
            y += count;
            x = x0;
        }
        else if (c == '!')
        {
            break; // end of pattern
        }
        else if (c == '\n' || c == '\r' || c == ' ' || c == '\t')
        {
            // ignore whitespace
        }
        else if (c == '#')
        {
            // comment until newline
            while (*p && *p != '\n')
                ++p;
        }
        else
        {
            // Unknown char; ignore to be permissive
        }
    }
    return 0;
}

void life_destroy(life_t *life)
{
    if (!life)
        return;
    free(life->curr);
    free(life->next);
    free(life);
    return;
}

life_t *life_create(size_t width, size_t height, bool torus)
{
    life_t *life = (life_t *)malloc(sizeof(life_t));
    if (!life)
        return NULL;
    life->width = width;
    life->height = height;
    life->torus = torus;
    life->nbits = width * height;
    life->nbytes = (life->nbits + 7) / 8;
    life->curr = (uint8_t *)calloc(life->nbytes, 1);
    life->next = (uint8_t *)calloc(life->nbytes, 1);
    if (!life->curr || !life->next)
    {
        life_destroy(life);
        return NULL;
    }
    return life;
}

void life_step(life_t *life)
{
    if (!life)
        return;

    const size_t W = life->width, H = life->height;
    memset(life->next, 0, life->nbytes);

    for (size_t y = 0; y < H; ++y)
    {
        for (size_t x = 0; x < W; ++x)
        {
            uint8_t neighbors =
                life_get(life, (long)x - 1, (long)y - 1) +
                life_get(life, (long)x, (long)y - 1) +
                life_get(life, (long)x + 1, (long)y - 1) +
                life_get(life, (long)x - 1, (long)y) +
                life_get(life, (long)x + 1, (long)y) +
                life_get(life, (long)x - 1, (long)y + 1) +
                life_get(life, (long)x, (long)y + 1) +
                life_get(life, (long)x + 1, (long)y + 1);

            uint8_t alive = life_get(life, x, y);
            uint8_t next_state = (alive ? (neighbors == 2 || neighbors == 3) : (neighbors == 3));

            size_t idx = y * W + x;
            size_t byte = idx >> 3;
            size_t bit = idx & 7;
            if (next_state)
                life->next[byte] |= (1 << bit);
        }
    }
    // swap buffers
    uint8_t *tmp = life->curr;
    life->curr = life->next;
    life->next = tmp;
}

//
// integrate the Conway's Game of Life mode into Marble Madness
//

//
// Conway's Game of Life patterns in RLE format
//
// Categories of patterns:
//  * Still lifes → Stable configurations that never change (e.g. block, loaf, beehive).
//  * Oscillators → Repeat in cycles (e.g. blinker, toad, beacon, pulsar).
//  * Spaceships → Move across the board while repeating (e.g. glider, LWSS).
//  * Methuselahs → Small starting patterns that evolve for many generations before stabilizing (e.g. R‑pentomino).
//  * Seeds → Patterns that eventually die out completely (no live cells remain).
//

//
//  🔎 Oscillators (repeat but stay bounded)
//

// Blinker: The simplest oscillator in Life.
// Place at (8,8) (center). Safe margin: 8 cells in each direction.
// Starts as a line of 3 cells. Period = 2.
// Alternates between vertical and horizontal line every generation.
// Fits in a 3x3 box, never grows.
const char *blinker =
    "x = 3, y = 1, rule = B3/S23\n"
    "3o!\n";

// Toad: A small period-2 oscillator of 6 cells.
// Place at (7,7). Safe margin: 7 cells around.
// Looks like two rows offset by one cell.
// Flips between two phases where the cells shift positions slightly.
// Fits in 4x4, stable and bounded.
const char *toad =
    "x = 4, y = 2, rule = B3/S23\n"
    "b3o$\n"
    "3ob!\n";

// Beacon: A period-2 oscillator made of two 2x2 blocks.
// Place at (10,10). Safe margin: 9 cells around.
// The inner corners toggle on and off, creating a flashing effect.
// Fits in 4x4, stable and bounded.
const char *beacon =
    "x = 4, y = 4, rule = B3/S23\n"
    "2o2b$\n"
    "2o2b$\n"
    "2b2o$\n"
    "2b2o!\n";

// Pulsar: A large, symmetric period-3 oscillator with 48 cells.
// Place at (3,3). Safe margin: 3 cells from each edge. Fits snugly inside 19×19.
// Its arms expand and contract in a repeating cycle.
// Fits in 13x13, well within a 19x19 board.
// Famous as one of the most iconic oscillators in Life.
const char *pulsar =
    "x = 13, y = 13, rule = B3/S23\n"
    "2b3o3b3o2b$\n"
    "bobo3bobo3b$\n"
    "bobo3bobo3b$\n"
    "2b3o3b3o2b$\n"
    "13b$\n"
    "2b3o3b3o2b$\n"
    "bobo3bobo3b$\n"
    "bobo3bobo3b$\n"
    "2b3o3b3o2b!\n";

//
//  🚀 Spaceships (move but stay contained if placed with margin)
//

// Glider: The smallest spaceship in Life.
// Place at (2,2) if you want it to travel down‑right.
// Place at (15,2) if you want it to travel down‑left. Safe margin: at least 15 cells in its travel direction.
// Starts with 5 cells. Period = 4.
// Moves diagonally across the board forever.
// Fits in 3x3, but travels — give it margin inside 19x19.
const char *glider =
    "x = 3, y = 3, rule = B3/S23\n"
    "bo2b$\n"
    "2bo$\n"
    "3o!\n";

// LWSS (Lightweight Spaceship): A small horizontal spaceship.
// Place at (2,8) if you want it to travel right.
// Place at (12,8) if you want it to travel left. Safe margin: at least 12 cells in its travel direction.
// Starts with 9 cells. Period = 4.
// Moves horizontally across the board forever.
// Fits in 5x4, safe inside 19x19 if placed with margin.
const char *lwss =
    "x = 5, y = 4, rule = B3/S23\n"
    "bo2bo$\n"
    "o4b$\n"
    "o3bo$\n"
    "4o!\n";

//
//  🛑 Methuselahs (Small starting patterns that evolve for many generations before stabilizing)
//

// Acorn: A famous Methuselah of 7 cells.
// Place at (6,6) (center). Safe margin: 6 cells around.
// Evolves for 5206 generations before vanishing.
// Produces hundreds of transient cells along the way.
// Fits in 7x3, safe for 19x19. Long-lived extinction case.
const char *acorn =
    "x = 7, y = 3, rule = B3/S23\n"
    "bo6b$\n"
    "3bo4b$\n"
    "2o2b3o!\n";

// R-pentomino: The most famous Methuselah, 5 cells.
// Place at (8,8) (center). Safe margin: 8 cells around. Note: chaotic but contained within 19×19.
// Evolves chaotically for 1103 generations before stabilizing.
// Produces a zoo of oscillators and still lifes.
// Fits in 3x3, safe for 19x19. Classic stress test.
const char *r_pentomino =
    "x = 3, y = 3, rule = B3/S23\n"
    "bo2b$\n"
    "2oo$\n"
    "2o!\n";

//
//  🌱 Seeds (die out completely)
//

// Diehard: A seed of 7 cells.
// Place at (6,8). Safe margin: 6 cells around.
// Lives for exactly 130 generations before vanishing completely.
// Fits in 7x3, safe for 19x19. Great extinction test case.
const char *diehard =
    "x = 7, y = 3, rule = B3/S23\n"
    "6bo$\n"
    "2o5b$\n"
    "b2o3b!\n";

//
// Gosper glider gun (RLE)
// This is a compact, canonical, infinite‑growth seed. Place it near the upper left on a sufficiently large board.
//

const char *gosper =
    "x = 36, y = 9, rule = B3/S23\n"
    "24bo11b$\n"
    "22bobo11b$\n"
    "12bo8b2o3bobo11b$\n"
    "11bobo6bo2bo3bo2bo10b$\n"
    "2bo8b2o6bo2bo3bo2bo10b$\n"
    "bobo6bo2bo6bo2bo3b2o3bo6b$\n"
    "bobo6bo2bo6bo2bo8bobo5b$\n"
    "2bo8b2o8b2o9bo6b$\n"
    "12bo23b!\n";

//
// 10‑cell infinite growth (block‑laying switch engine)
// A minimal seed discovered by Paul Callahan; also yields unbounded growth:
// small blocks are laid down as the pattern expands.
//
const char *blocklaying =
    "x = 10, y = 5, rule = B3/S23\n"
    "bo3bo2b$\n"
    "b2obo2b$\n"
    "2o2b2o2b$\n"
    "3bo3b$\n"
    "2b2o2b!\n";

const char *block =
    "x = 2, y = 2, rule = B3/S23\n"
    "2o$\n"
    "2o!\n";

// Beehive (6x5 bbox but 6x4 active)
const char *beehive =
    "x = 6, y = 5, rule = B3/S23\n"
    "2b2o2b$\n"
    "bo4b$\n"
    "bo4b$\n"
    "2b2o2b$\n"
    "6b!\n";

// Loaf (canonical compact loaf, 4x4 bbox)
const char *loaf =
    "x = 4, y = 4, rule = B3/S23\n"
    "b2ob$\n"
    "o2bo$\n"
    "bobo$\n"
    "2b2o!\n";

// Boat (5x4 bbox)
const char *boat =
    "x = 5, y = 4, rule = B3/S23\n"
    "bo2b2$\n"
    "bobo2$\n"
    "2bo2b$\n"
    "2b2o!\n";

const char *clock_pattern =
    "x = 6, y = 6, rule = B3/S23\n"
    "2bo2b2$\n"
    "b3o2b$\n"
    "2o3b$\n"
    "2b3o$\n"
    "2b2ob2$\n"
    "2b2o2b!\n";

const char *pentadecathlon =
    "x = 18, y = 11, rule = B3/S23\n"
    "6b2o2b2o6b$\n"
    "6b2o2b2o6b$\n"
    "18b$\n"
    "6b2o2b2o6b$\n"
    "6b2o2b2o6b$\n"
    "18b$\n"
    "6b2o2b2o6b$\n"
    "6b2o2b2o6b$\n"
    "18b$\n"
    "6b2o2b2o6b$\n"
    "6b2o2b2o6b!\n";

const char *blinker_vertical =
    "x = 1, y = 3, rule = B3/S23\n"
    "o$\n"
    "o$\n"
    "o!\n";

// Toad (canonical 4x2)
const char *toad_compact =
    "x = 4, y = 2, rule = B3/S23\n"
    "b3o$\n"
    "o3b!\n";

// Beacon (canonical 6x6)
const char *beacon_6x6 =
    "x = 6, y = 6, rule = B3/S23\n"
    "2o2b2o$\n"
    "2o2b2o$\n"
    "6b$\n"
    "2o2b2o$\n"
    "2o2b2o!\n";

life_t *life;
int generation_max, generation;

void LoadRandomPattern(life_t *life)
{
// select a random pattern to load
#ifdef DEBUG
    const char *pattern_names[] = {
        //        "blinker",
        "toad",
        "beacon",
        "pulsar",
        "glider",
        "lwss",
        "acorn",
        "r_pentomino",
        "diehard",
        "gosper",
        "blocklaying",
        "block",
        "beehive",
        "loaf",
        "boat",
        "clock_pattern",
        "pentadecathlon",
        //        "blinker_vertical",
        "toad_compact",
        "beacon_6x6"};
#endif // DEBUG
    const char *patterns[] = {
        //        blinker,
        toad,
        beacon,
        pulsar,
        glider,
        lwss,
        acorn,
        r_pentomino,
        diehard,
        gosper,
        blocklaying,
        block,
        beehive,
        loaf,
        boat,
        clock_pattern,
        pentadecathlon,
        //        blinker_vertical,
        toad_compact,
        beacon_6x6};
    int index = random16(sizeof(patterns) / sizeof(patterns[0]));
    const char *pattern = patterns[index];

    // reset the board
    memset(life->curr, 0, life->nbytes);
    memset(life->next, 0, life->nbytes);

    // load the pattern at the center of the board
    size_t x0 = life->width / 2;
    size_t y0 = life->height / 2;
    life_load_rle(life, pattern, 3, 3);
#ifdef DEBUG
    DB_PRINTF("Loaded pattern: %s\n", pattern_names[index]);
#endif // DEBUG

    generation_max = 100;
    generation = 0;
}

static void setLED(int x, int y)
{
    int index = XY(x, y);
    leds[index] = blend(leds[index], settings.clockColor, 128);
}

void draw_counter(int count)
{
    // display the generation count in the lower right corner
    int n1 = (count / 1000) % 10;
    int n2 = (count / 100) % 10;
    int n3 = (count / 10) % 10;
    int n4 = count % 10;
    const int startY = (NUM_ROWS - 5);
    int x = NUM_COLS - 15 + 8;

    //    drawDigit3x5(n1, x, startY, setLED); x += 3; x += 1;
    //    drawDigit3x5(n2, x, startY, setLED); x += 3; x += 1;
    drawDigit3x5(n3, x, startY, setLED);
    x += 3;
    x += 1;
    drawDigit3x5(n4, x, startY, setLED);
}

void life_enter()
{
    DB_PRINTLN("Entering Life mode");
    life = life_create(NUM_COLS, NUM_ROWS, false);
    if (!life)
        return;

    LoadRandomPattern(life);
}

void life_loop()
{
    EVERY_N_MILLIS_I(timer, DEFAULT_MILLIS)
    {
        timer.setPeriod(MAX_MILLIS - map(settings.speed, MIN_SPEED, MAX_SPEED, MIN_MILLIS, MAX_MILLIS));

        // update LED matrix from life state
        for (size_t y = 0; y < life->height; ++y)
        {
            for (size_t x = 0; x < life->width; ++x)
            {
                leds[XY(x, y)] = life_get(life, x, y) ? CRGB::White : CRGB::Black;
            }
        }

        life_step(life);
        generation++;
        draw_counter(generation);

        // detect when we should reset the patterns
        if ((generation >= generation_max) || !memcmp(life->curr, life->next, life->nbytes))
        {
            LoadRandomPattern(life);
        }
        static uint8_t prev_checksum = 0;
    }

    leds_dirty = true;
}

void life_leave()
{
    DB_PRINTLN("Leaving Life mode");

    life_destroy(life);
    life = NULL;
}
