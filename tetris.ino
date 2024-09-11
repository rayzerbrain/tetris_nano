#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include "global.h"

bool do_loop = true;

int tick = 0;
bool screen_1d[PIXEL_WIDTH * PIXEL_HEIGHT];
LiquidCrystal_I2C lcd(LCD_ADDRESS, WIDTH, HEIGHT);

uint8_t get_input() {
  
  //wire such that kpin 8-5 (row 1-4) of keypad is at d3-6, and kpin 4-1 (col 1-4) at d7-10
  //input grid as follows
  // 1 2 3
  // 4 5 6
  // 7 8 9
  // "2" is rot, "4"/"6" is move left/right, "8" is down (no instant drop, yet)
  

  uint8_t result = 0;

  if (query_coords(1, 2))
    result |= I_ROT;
  if (query_coords(2, 1))
    result |= I_LEFT;
  if (query_coords(2, 2))
    result |= I_DOWN;
  if (query_coords(2, 3))
    result |= I_RIGHT;
  if (query_coords(3, 2))
    result |= I_DROP;

  return result;
}
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("Starting up...");

  Serial.println("CLEARING LCD");
  lcd.init();
  lcd.backlight();
  delay(2000);
  lcd.clear();

  for (int i = 3; i <= 6; i++) {
    pinMode(i, INPUT); // d3,4,5,6 are col getters
    digitalWrite(i, 0);
    pinMode(i+4, OUTPUT); //d7,8,9,10 are row setters
  }

  tetris_setup(GRAVITY);


  /*bool arr[4][4] = {{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1}};
  bool (*ptr)[4] = arr;
  for (int row = 0; row < 4; row++) {
    for (int col = 0; col < 4; col++) {
      Serial.println(pgm_read_byte_near(piece_bitmap[0][0][0] + (row * 4) + col));
      ptr[row][col] = pgm_read_byte_near(piece_bitmap[0][0][0] + (row * 4) + col);
      Serial.println(ptr[row][col]);
    }
  }
  Serial.println(sizeof(ptr));*/
  
  // score storing stuff
  //lcd.print(EEPROM.read(0));
  //EEPROM.write(0, 1);
}

#define TICK_MS (int)(1000.f / TPS) // ms per tick
void loop() {
  if (do_loop) {
    //Serial.print("tick ");
    //Serial.println(tick++);
    delay(TICK_MS);
    tetris_do_tick(get_input());
    if (tetris_get_screen(screen_1d)) {
      lcd_blit(&lcd, screen_1d);
    }
  }
  
  //f = false;
}
