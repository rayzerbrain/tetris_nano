#define WIDTH 16
#define HEIGHT 2
#define CELL_WIDTH 5
#define CELL_HEIGHT 8

// define the characteristics of the writable bitmap using cell positions
// TOTAL USED CELLS MUST BE LESS THAN CUSTOM CHAR CAP (8)
#define Y0 0
#define Y1 1
#define X0 12
#define X1 15 
#define MAP_WIDTH X1 - X0 + 1
#define MAP_HEIGHT Y1 - Y0 + 1
#define PIXEL_WIDTH (CELL_WIDTH) * (MAP_WIDTH)
#define PIXEL_HEIGHT (CELL_HEIGHT) * (MAP_HEIGHT)

#define LCD_ADDRESS 0x27
#define EEPROM_SCORE_ADDRESS 0x0

#define TPS 30 // ticks per 1 sec
#define GRAVITY 15 // ticks per pixel

#define TPI_HELD 3 // ticks per input acknowledged when it's held down

struct rect
{
  int y0;
  int y1;
  int x0;
  int x1;
};

enum 
{
  I_DROP = 0b10000,
  I_LEFT = 0b01000,
  I_RIGHT = 0b00100,
  I_DOWN = 0b00010,
  I_ROT = 0b00001
};
