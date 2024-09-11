#define NEXT_PIECE_Y 0 
#define NEXT_PIECE_X 5

int tick_ctr = 0;

int width = 0;
int height = 0;
int gravity = 0; // in ticks per pixel
int score = 0;


//uint8_t current_input = 0b0000; // 4-bit flag: left, right, down, rotate

uint8_t prev_inputs[TPS] = {0}; // remember every input per tick up to one second ago

struct rect stage = { // incl, excl.
    .y0 = 6,
    .y1 = 16,
    .x0 = 0,
    .x1 = 20,
  }; // define corners of "stage", with origin being TOP-LEFT of the writable bitmap. incl, excl
bool stage_bitmap[PIXEL_HEIGHT][PIXEL_WIDTH] = {{0}}; // Starting point for meshing bitmaps. Changes when pieces "land"

#define THIS_PIECE_Y (((stage.y1 - stage.y0) / 2) + stage.y0)
#define THIS_PIECE_X 0

bool screen[PIXEL_HEIGHT][PIXEL_WIDTH] = {{0}};
bool is_screen_updated = false; // if false at end of tick logic, update screen by re-meshing bitmaps
bool is_screen_different = true; // only true if screen is different than last time it was requested

struct current_piece {
  int x = THIS_PIECE_X; // starting coords
  int y = THIS_PIECE_Y;
  int index = 0;
  int rot = 0;
  bool bitmap[4][4] = {{0}};
} current_piece;

struct next_piece {
  int index = 0;
  bool bitmap[4][4] = {{0}};
} next_piece;

void update_bitmap_cache(bool is_current_piece) {
  // pull bitmaps from prog mem, dynamic mem too small to fit all of them
  bool (*bitmap)[4];
  int index;
  int rot;
  if (is_current_piece) {
    bitmap = current_piece.bitmap;
    index = current_piece.index;
    rot = current_piece.rot;
  } else {
    bitmap = next_piece.bitmap;
    index = next_piece.index;
    rot = 0;
  }
  //Serial.println(index);
  //Serial.println(rot);
  // copy the block bitmap from progmem into ram given parameters
  for (int row = 0; row < 4; row++) {
    for (int col = 0; col < 4; col++) {
      bitmap[row][col] = pgm_read_byte_near(piece_bitmaps[index][rot][0] + (row * 4) + col);
    }
  }
}

// attempts to map child onto parent at offset coords. if conflict occurs, returns false. not safe from out of bounds err
// PASS NULL TO TARGET TO JUST TEST MESH
bool try_mesh_bitmaps(bool target[PIXEL_HEIGHT][PIXEL_WIDTH], bool parent[PIXEL_HEIGHT][PIXEL_WIDTH], bool child[4][4], int y_off, int x_off) { 
  bool result[PIXEL_HEIGHT][PIXEL_WIDTH] = {{0}};
  memcpy(&result[0][0], &parent[0][0], sizeof(result));
  

  for (int row = 0; row < 4; row++) {
    for (int col = 0; col < 4; col++) {
      
      int p_y = y_off + row;
      int p_x = x_off + col;

      bool c_bit = child[row][col];
      // if parent bit location is out of bounds, skip/return
      if (p_x < 0 || p_x > PIXEL_WIDTH - 1 || p_y < 0 || p_y > PIXEL_HEIGHT - 1) { // off by 1 cause index starts at 0 but pixel count doesn't
        if (c_bit)
          return false;

        continue;
      }
      
      bool p_bit = parent[p_y][p_x];      

      if (c_bit && p_bit)
        return false;
      
      result[p_y][p_x] = c_bit || p_bit;
    }
  }

  if (target != NULL)
    memcpy(&target[0][0], &result[0][0], sizeof(result)); // cant use sizeof on array arguments for some dumb reason.
  
  return true;
}

void check_rows() { // see if there are any full rows. if there are, shift all pixels on stage down 1
  // although tetris has "rows" from a vertical perspective, all code is done with the horizontal perspective of the lcd screen
  for (int col = stage.x1 - 1; col >= 0; col--) {
    bool col_full = true;
    Serial.print("Checking column: ");
    Serial.println(col);
    for (int row = stage.y0; row < stage.y1; row++) {
      if (stage_bitmap[row][col] == 0)
        col_full = false;
    }

    if (col_full) {
      Serial.print("COL FULL AT: ");
      Serial.println(col);
      // clear the full row
      for (int row = stage.y0; row < stage.y1; row++) {
        stage_bitmap[row][col] = 0;
      }
      Serial.println("here");
      // shift all pixel rows above the row down one
      for (int col2 = col - 1; col2 >= 0; col2--) {
        Serial.println(col2);
        for (int row = stage.y0; row < stage.y1; row++) {
          stage_bitmap[row][col2 + 1] = stage_bitmap[row][col2];
        }
      }
      Serial.println("got here");
      //check this column again, because it was shifted
      col++;
    }
  }
}

void try_shift_piece(bool is_left) {
  // if mesh fails, do nothing
  int old_y = current_piece.y;

  current_piece.y = is_left ? old_y + 1 : old_y - 1;
  if (try_mesh_bitmaps(NULL, stage_bitmap, current_piece.bitmap, current_piece.y, current_piece.x))
    is_screen_updated = false;
  else
    current_piece.y = old_y;
}

void try_drop_piece() {
  // just move piece "down" one
  // if mesh fails, place piece, and load new ones. also check for row completion
  current_piece.x++;
  is_screen_updated = false;
  //Serial.println(current_piece.x);

  if (!try_mesh_bitmaps(NULL, stage_bitmap, current_piece.bitmap, current_piece.y, current_piece.x)) {
    // "place" piece
    Serial.print("Drop mesh FAILED at ");
    Serial.print(current_piece.x);
    Serial.print(", ");
    Serial.println(current_piece.y);

    current_piece.x--;
    //delay(5000);

    //if piece can't fit even when scooted up, it must be beyond the upper limit of the stage
    if (!try_mesh_bitmaps(stage_bitmap, stage_bitmap, current_piece.bitmap, current_piece.y, current_piece.x)) {
      Serial.println("GAME OVER");
      delay(2000);
      exit(0);
    }
    
    // Load new piece
    current_piece.index = next_piece.index;
    current_piece.x = THIS_PIECE_X;
    current_piece.y = THIS_PIECE_Y;
    current_piece.rot = 0;
    update_bitmap_cache(true);

    int rand_index = 0;
    do {
      rand_index = random(0, PIECE_COUNT);
    } while (rand_index == current_piece.index);
    next_piece.index = rand_index;
    update_bitmap_cache(false);
    
    check_rows();

    is_screen_updated = false;
  }

}

void try_rotate_piece() {
  // if mesh fails, try shifting right once and trying again
  // then, try shifting left once, then twice
  int old_rot = current_piece.rot;
  current_piece.rot++;
  current_piece.rot %= 4; // limit to 0-3

  if (!try_mesh_bitmaps(NULL, stage_bitmap, current_piece.bitmap, current_piece.y, current_piece.x)) {
    current_piece.y--;
    if (!try_mesh_bitmaps(NULL, stage_bitmap, current_piece.bitmap, current_piece.y, current_piece.x)) {
      current_piece.y += 2;
      if (!try_mesh_bitmaps(NULL, stage_bitmap, current_piece.bitmap, current_piece.y, current_piece.x)) {
        current_piece.y++;
        if (!try_mesh_bitmaps(NULL, stage_bitmap, current_piece.bitmap, current_piece.y, current_piece.x)) {
          // give up
          current_piece.rot = old_rot;
          return;
        }
      }
    }
  }

  update_bitmap_cache(true);
  is_screen_updated = false;

}

void handle_input(uint8_t flag) {
  uint8_t flag_prev = prev_inputs[0];
  uint8_t flag_held = I_LEFT & I_RIGHT & I_DOWN; // find which inputs have been held down for ~1 second or more (I_ROT and I_DROP irrelevant here)
  for (int i = 0; i < sizeof(prev_inputs) / sizeof(prev_inputs[0]); i++) {
    if (!(prev_inputs[i] & I_LEFT))
      flag_held &= ~I_LEFT;
    if (!(prev_inputs[i] & I_RIGHT))
      flag_held &= ~I_RIGHT;
    if (!(prev_inputs[i] & I_DOWN))
      flag_held &= ~I_DOWN;
  }

  bool is_action_tick = tick_ctr % TPI_HELD == 0; // is this a tick where a held input is acknowledged?

  if (flag & I_DROP && !(flag_prev & I_DROP)) {
    int old_index = current_piece.index;

    //drop the piece one row at a time until it is placed (a new piece is loaded, with a new index)
    while (current_piece.index == old_index) { 
      try_drop_piece();
    }
  }

  if (flag & I_ROT && !(flag_prev & I_ROT))
    try_rotate_piece();

  if (flag & I_LEFT && ( !(flag_prev & I_LEFT) || (flag_held & I_LEFT && is_action_tick) )) // if pressing left AND (last input wasnt left OR left is being held)
    try_shift_piece(true);

  if (flag & I_RIGHT && ( !(flag_prev & I_RIGHT) || (flag_held & I_RIGHT && is_action_tick) ))
    try_shift_piece(false);

  if (flag & I_DOWN && ( !(flag_prev & I_DOWN) || (flag_held & I_DOWN && is_action_tick) ))
    try_drop_piece();

  // if piece moves: screen_changed = true;
}

void tetris_do_tick(uint8_t input) { // recommended tps: 30?
  handle_input(input);

  for (int i = TPS - 2; i >= 0; i--) { // shift all inputs over in queue fashion. add latest input at index 0, get rid of oldest recorded input at index TPS - 1
    prev_inputs[i] = prev_inputs[i + 1];
  }
  prev_inputs[0] = input;

  // handle gravity here
  if (tick_ctr % gravity == 0) {
    try_drop_piece();
  }

  if (!is_screen_updated) {
    is_screen_different = true;

    try_mesh_bitmaps(screen, stage_bitmap, current_piece.bitmap, current_piece.y, current_piece.x);
    try_mesh_bitmaps(screen, screen, next_piece.bitmap, NEXT_PIECE_Y, NEXT_PIECE_X);

    is_screen_updated = true;
  }

  tick_ctr++;
}

void tetris_setup(int g) {
  gravity = g;

  for (int i = stage.x0; i < stage.x1; i++) {
    stage_bitmap[stage.y0 - 1][i] = 1; // create the stage bitmap by defining "top" "wall"
  }

  //randomSeed(analogRead(0)); // setup the rng with random seed
  randomSeed(analogRead(0));

  // decide piece to be dropped first and next
  int rand_index = random(0, PIECE_COUNT);
  current_piece.index = rand_index; // default rotation
  update_bitmap_cache(true);

  do {
    rand_index = random(0, PIECE_COUNT);
  } while (rand_index == current_piece.index);
  next_piece.index = rand_index;
  update_bitmap_cache(false);
}

// insert screen into the bool pointer
bool tetris_get_screen(bool result[]) { // returns bool indicating whether screen changed since last time it was requested
  if (is_screen_different) {
    is_screen_different = false;
    //bool screen_1d[PIXEL_WIDTH * PIXEL_HEIGHT];
    for (int row = 0; row < PIXEL_HEIGHT; row++) {
      for (int col = 0; col < PIXEL_WIDTH; col++) {
        
        /*Serial.print("Val at ");
        Serial.print(row);
        Serial.print(",");
        Serial.print(col);
        Serial.print(": ");
        Serial.println(screen[row][col]);*/

        result[row * PIXEL_WIDTH + col] = screen[row][col]; // stage -> screen
      }
    }
    return true;
  } else {
    //*ptr = NULL;
    return false;
  }
}