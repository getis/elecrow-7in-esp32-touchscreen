#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>

// Define a class named LGFX, inheriting from the LGFX_Device class.
class LGFX : public lgfx::LGFX_Device {
public:
  // Instances for the RGB bus and panel.
  lgfx::Bus_RGB     _bus_instance;
  lgfx::Panel_RGB   _panel_instance;
  lgfx::Light_PWM   _light_instance;

  // Constructor for the LGFX class.
  LGFX(void) {

    // Configure the panel.
    {
      auto cfg = _panel_instance.config();
      cfg.memory_width  = 800;
      cfg.memory_height = 480;
      cfg.panel_width   = 800;
      cfg.panel_height  = 480;
      cfg.offset_x      = 0;
      cfg.offset_y      = 0;

      // Apply configuration to the panel instance.
      _panel_instance.config(cfg);
    }

    {
      auto cfg = _panel_instance.config_detail();

      cfg.use_psram = 1;

      _panel_instance.config_detail(cfg);
    }


    // Configure the RGB bus.
    {
      auto cfg = _bus_instance.config();
      cfg.panel = &_panel_instance;

      // Configure data pins.
      cfg.pin_d0  = GPIO_NUM_15; // B0
      cfg.pin_d1  = GPIO_NUM_7;  // B1
      cfg.pin_d2  = GPIO_NUM_6;  // B2
      cfg.pin_d3  = GPIO_NUM_5;  // B3
      cfg.pin_d4  = GPIO_NUM_4;  // B4
      
      cfg.pin_d5  = GPIO_NUM_9;  // G0
      cfg.pin_d6  = GPIO_NUM_46; // G1
      cfg.pin_d7  = GPIO_NUM_3;  // G2
      cfg.pin_d8  = GPIO_NUM_8;  // G3
      cfg.pin_d9  = GPIO_NUM_16; // G4
      cfg.pin_d10 = GPIO_NUM_1;  // G5
      
      cfg.pin_d11 = GPIO_NUM_14; // R0
      cfg.pin_d12 = GPIO_NUM_21; // R1
      cfg.pin_d13 = GPIO_NUM_47; // R2
      cfg.pin_d14 = GPIO_NUM_48; // R3
      cfg.pin_d15 = GPIO_NUM_45; // R4

      // Configure sync and clock pins.
      cfg.pin_henable = GPIO_NUM_41;
      cfg.pin_vsync   = GPIO_NUM_40;
      cfg.pin_hsync   = GPIO_NUM_39;
      cfg.pin_pclk    = GPIO_NUM_0;
      cfg.freq_write  = 14000000;

      // Configure timing parameters for horizontal and vertical sync.
      cfg.hsync_polarity    = 0;
      cfg.hsync_front_porch = 40;
      cfg.hsync_pulse_width = 48;
      cfg.hsync_back_porch  = 40;
      
      cfg.vsync_polarity    = 0;
      cfg.vsync_front_porch = 1;
      cfg.vsync_pulse_width = 31;
      cfg.vsync_back_porch  = 13;

      // Configure polarity for clock and data transmission.
      cfg.pclk_active_neg   = 1;
      cfg.de_idle_high      = 0;
      cfg.pclk_idle_high    = 0;

      // Apply configuration to the RGB bus instance.
      _bus_instance.config(cfg);
    }

    {
      auto cfg = _light_instance.config();
      cfg.pin_bl = GPIO_NUM_2;
      _light_instance.config(cfg);
    }
    _panel_instance.light(&_light_instance);

    // Set the RGB bus and panel instances.
    _panel_instance.setBus(&_bus_instance);
    setPanel(&_panel_instance);
  }
};

static LGFX lcd;
static LGFX_Sprite sprites[2];

auto lcd_width = 800;
auto lcd_height = 480;

int frameCounter = 0;

#define NUM_BLOCKS 50

class Block {
  public:
    int x;
    int y;
    int width;
    int height;
    int colour;
    int xVel;
    int yVel;

    // default constructor
    Block(){}
    
    Block(int xpos, int ypos, int bwidth, int bheight, int bcol, int bxVel, int byVel){
      x = xpos;
      y = ypos;
      width = bwidth;
      height = bheight;
      colour = bcol;
      xVel = bxVel;
      yVel = byVel;
    }

    void drawBlock(LGFX_Sprite *sprite){
      sprite->fillRect(x,y,width,height,colour);
    }

    void moveBlock(){
      x += xVel;
      if(x < 0){
        x = 0;
        xVel = -xVel;
      }
      else if(x > (lcd_width - width)) {
        x = (lcd_width - width);
        xVel = -xVel;
      }

      y += yVel;
      if(y < 0){
        y = 0;
        yVel = -yVel;
      }
      else if(y > (lcd_height - height)) {
        y = (lcd_height - height);
        yVel = -yVel;
      }
    }
};
Block blocks[NUM_BLOCKS];

void setup() {
  int blockCounter;

  frameCounter = 0;

  lcd.begin();
  lcd.startWrite();
  lcd.setColorDepth(16);
  lcd.setTextSize(10);
  if (lcd.width() < lcd.height()) lcd.setRotation(lcd.getRotation() ^ 1);

  for (std::uint32_t i = 0; i < 2; ++i)
  {
    sprites[i].setTextSize(10);
    sprites[i].setColorDepth(8);
  }

  bool fail = false;
  for (std::uint32_t i = 0; !fail && i < 2; ++i)
  {
    fail = !sprites[i].createSprite(lcd_width, lcd_height);
  }

  if (fail)
  {
    fail = false;
    for (std::uint32_t i = 0; !fail && i < 2; ++i)
    {
      sprites[i].setPsram(true);
      fail = !sprites[i].createSprite(lcd_width, lcd_height);
    }

    if (fail)
    {
      fail = false;
      if (lcd_width > 320) lcd_width = 320;
      if (lcd_height > 240) lcd_height = 240;

      for (std::uint32_t i = 0; !fail && i < 2; ++i)
      {
        sprites[i].setPsram(true);
        fail = !sprites[i].createSprite(lcd_width, lcd_height);
      }
      if (fail)
      {
        lcd.print("createSprite fail...");
        lgfx::delay(3000);
      }
    }
  }

  lcd.fillScreen(TFT_BLACK);

  for(blockCounter=0; blockCounter < NUM_BLOCKS; blockCounter ++){
    blocks[blockCounter] = Block(
      0,
      0,
      random(3,20),
      random(3,20),
      lgfx::color565(100+(rand()%155), 100+(rand()%155), 100+(rand()%155)),
      random(20) - 10,
      random(20) - 10
    );
  }

}

static void diffDraw(LGFX_Sprite* sp0, LGFX_Sprite* sp1)
{
  union
  {
    std::uint32_t* s32;
    std::uint8_t* s;
  };
  union
  {
    std::uint32_t* p32;
    std::uint8_t* p;
  };
  s32 = (std::uint32_t*)sp0->getBuffer();
  p32 = (std::uint32_t*)sp1->getBuffer();

  auto width  = sp0->width();
  auto height = sp0->height();

  auto w32 = (width+3) >> 2;
  std::int32_t y = 0;
  do
  {
    std::int32_t x32 = 0;
    do
    {
      while (s32[x32] == p32[x32] && ++x32 < w32);
      if (x32 == w32) break;

      std::int32_t xs = x32 << 2;
      while (s[xs] == p[xs]) ++xs;

      while (++x32 < w32 && s32[x32] != p32[x32]);

      std::int32_t xe = (x32 << 2) - 1;
      if (xe >= width) xe = width - 1;
      while (s[xe] == p[xe]) --xe;

      lcd.pushImage(xs, y, xe - xs + 1, 1, &s[xs]);
    } while (x32 < w32);
    s32 += w32;
    p32 += w32;
  } while (++y < height);
  lcd.display();
}



int loopTime=0;
void loop () {
  int blockCounter;
  size_t f =   frameCounter & 1;
  LGFX_Sprite *sprite = &(sprites[f]);

  auto startTime = micros();
  sprite->clear();
  for(blockCounter=0; blockCounter < NUM_BLOCKS; blockCounter ++){
    blocks[blockCounter].moveBlock();
    blocks[blockCounter].drawBlock(sprite);
  }
  sprite->setCursor(1,1);
  sprite->setTextColor(TFT_WHITE);
  sprite->printf("%d us", loopTime);
  diffDraw(&sprites[f], &sprites[!f]);
  auto endTime = micros();
  loopTime = endTime - startTime;
  frameCounter ++;
}

