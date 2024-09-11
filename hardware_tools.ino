int lcd_blit(LiquidCrystal_I2C *lcd, bool *buffer) { 
  int index = 0;  // buffer index
  int cell_num = 0; 

  for (int row = Y0; row <= Y1 ; row++) {
    for (int col = X0; col <= X1 ; col++) { // iterate over all cells
      uint8_t cell[CELL_HEIGHT] = {0};

      // populate the cell and write it to screen
      for (int cell_row = 0; cell_row < CELL_HEIGHT; cell_row++) {
        int i = index + (PIXEL_WIDTH * cell_row); // specific index starting at beginning of cell row
        for (int cell_col = 0; cell_col < CELL_WIDTH; cell_col++) {
          cell[cell_row] |= buffer[i + cell_col] << (CELL_WIDTH - 1 - cell_col); // add each pixel to the cell in bit flag form
        }
      }

      lcd->createChar(cell_num, cell); // use up the available custom chars
      lcd->setCursor(col, row);
      lcd->write(cell_num++);

      index += CELL_WIDTH; // move to next cell
    }
    index += PIXEL_WIDTH * (CELL_HEIGHT - 1); // move to the next row of cells
  }
}

//checks if key of zrx 543 keypad is being pressed at row and col
bool query_coords(int row, int col) {
#define ROW_2_PIN(row) (11 - row)
#define COL_2_PIN(col) (7 - col)
  int cpin = COL_2_PIN(col);
  int rpin = ROW_2_PIN(row);

  pinMode(cpin, OUTPUT);
  digitalWrite(cpin, LOW); // pull low

  pinMode(cpin, INPUT);
  digitalWrite(rpin, HIGH);
  bool result = digitalRead(cpin);
  digitalWrite(rpin, LOW);

  pinMode(cpin, OUTPUT);
  digitalWrite(cpin, LOW); // pull low again, for good measure
  return result;
}