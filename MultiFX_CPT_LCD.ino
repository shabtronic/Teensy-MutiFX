/*Using LVGL with Arduino requires some extra steps:
 *Be sure to read the docs here: https://docs.lvgl.io/master/get-started/platforms/arduino.html  */
#include <Audio.h>
#include <lvgl.h>
#include <string>
#define ENCODER_OPTIMIZE_INTERRUPTS
#include <Encoder.h>
using namespace std;

#include "SdFat.h"


#ifndef SDCARD_SS_PIN
const uint8_t SD_CS_PIN = SS;
#else   // SDCARD_SS_PIN

const uint8_t SD_CS_PIN = SDCARD_SS_PIN;
#endif  // SDCARD_SS_PIN

// SD_FAT_TYPE = 0 for SdFat/File as defined in SdFatConfig.h,
// 1 for FAT16/FAT32, 2 for exFAT, 3 for FAT16/FAT32 and exFAT.
#define SD_FAT_TYPE 3
SdFs sd;
int FileCount = 0;
int DirCount = 0;
void DirScan(String dir) 
{
  FsFile file;
  FsFile root;
  root.open(dir.c_str());
  while (file.openNext(&root, O_RDONLY))
   {
    char temp[256];
    file.getName(temp, 256);

    if (file.isDir()) 
    {
      DirCount++;
      String newdir = (dir + "/" + temp);
      DirScan(newdir.c_str());
    } else 
    {
      String tt = temp;
      if (tt.toUpperCase().endsWith(".MP3")) 
      {
        FileCount++;
        String ts = dir;
        String FileAndPath = (ts + "/" + temp);
        Serial.println(FileAndPath.c_str());
      //  Files.push_back(FileAndPath);
      }
    }
  }
}

AudioInputUSB            USBIn;
AudioInputI2S            I2SIn;
AudioMixer4              mixer2;
AudioMixer4              mixer1;
AudioOutputUSB           USBOut;
AudioOutputI2S           I2SOut;

AudioControlSGTL5000     sgtl5000_1;
lv_obj_t *Params[8];
float VolPots[4];
float Pots[8];
Encoder RotEnc(33, 34);

#define s8 char
class Parameter
{
  public:
  Parameter(string name,float min,float max,float def,bool isint,string format)
  {
  ValueMin=min;
  ValueMax=max;
  Value=def;
  namestr=name;
  formatstr=format;
  IsInteger=isint;
  }
  void SetParametric(float p)   { Value=(ValueMax-ValueMin)*p+ValueMin; }
  float GetParametric()         { return (Value-ValueMin)/(ValueMax-ValueMin); }

private:
  float Value,ValueMin,ValueMax,ValueDefault;
  bool IsInteger;
  string namestr;
  string formatstr;
};
class AllPass
{
  public:
  int16_t* Buf;
  int p=0;
  int size=0;
  AllPass(int len)
  {
  Buf=(int16_t*)malloc(len*2);
  size=len;
  }

  int16_t process(int16_t ins)
  {
  ins=ins+Buf[p]/-2;
  int16_t out=Buf[p]+ins/2;
  Buf[p]=ins;
  p+=1;
  if (p>=size) p=0;
  return -out;
  }
};
const float amp1=1.909;
  AllPass L1(21*amp1);
  AllPass L2(53*amp1);
  AllPass L3(173*amp1);
  
  AllPass R1(32*amp1);
  AllPass R2(75*amp1);
  AllPass R3(133*amp1);
const float apm=amp1*2.1;
  AllPass L4(21*apm);
  AllPass L5(53*apm);
  AllPass L6(173*apm);
  
  AllPass R4(32*apm);
  AllPass R5(75*apm);
  AllPass R6(133*apm);

  const float apm2=2;
  AllPass L7(11*apm2);
  AllPass L8(23*apm2);
  AllPass L9(43*apm2);
  
  AllPass R7(82*apm2);
  AllPass R8(125*apm2);
  AllPass R9(233*apm2);
  int sclamp(int s)
  {
    if (s>32767) s=32767;
    if (s<-32767) s=-32767;
    return s;
  }
class ReverseOTron
{
  public:// User Parameters
    float pTime=1;
    float pDry=1;
    float pWet=1;
    float pGateDuck=0;
    int  pBitRes=8;
    float pStereoWidth=0;
    private:
    int TapCount=10;
    int TapSize=10*44109;
  public:

  // 264kb
    int16_t DBuf[44100*3]; // Max Time 3 secs
    int TapTimes[400];
    float   TapVols[400];
float Wet=0.5;
float Dry=0.5;
float GateDuck=0;
    int ReadP=0;
    bool Enabled=false;
    void SetParamLabels()
    {
      for (int a=0;a<8;a++)
       lv_label_set_text(Params[8], "");
    }
    void SetWet(float f)
    {
    if (f!=Wet)
      {
        Wet=f;
        char tmp[1024];

        sprintf(tmp,"Wet %d",(int)(Wet*100));
        if (Params[6])
        lv_label_set_text(Params[6], tmp);
    }
    }
    void SetGateDuck(float f)
    {
       if (f!=GateDuck)
      {
        GateDuck=f;
        char tmp[1024];

        sprintf(tmp,"GateDuck %2.1f",Wet);
        if (Params[5])
        lv_label_set_text(Params[5], tmp);
    }  
    }
    void SetDry(float f)
    {
      if (f!=Dry)
      {
        Dry=f;
        char tmp[1024];

        sprintf(tmp,"Dry %d",(int)(Dry*100));
        if (Params[6])
        lv_label_set_text(Params[7], tmp);
      }
    }
    void SetTime(float s)
    {
      s=clamp(s,0.1f,3.0f);
      if (int(s*44100)!=TapSize)
      {
        Serial.printf("Set Time is different! %d %d",int(s*44100),TapSize);
        char tmp[1024];
       
        sprintf(tmp,"Time %2.1f secs",s);
        if (Params[0])
        lv_label_set_text(Params[0], tmp);
   
        pTime=s;
        ParamsChanged();
      }
    }
    ReverseOTron()
    {
      for (int a=0;a<44100*3;a++)
      DBuf[a]=0;
      pTime=3;
      ReadP=0;
      ParamsChanged();
    }
    void ParamsChanged()
    {
      TapSize=pTime*44100;
      TapCount=pTime*30;
      for (int ix=0;ix<TapCount;ix++)
        {
        float p=float(ix)/(TapCount);
        TapTimes[ix]=powf(p,2)*TapSize;
        TapVols[ix]=powf(1-p,0.5);
        if (TapVols[ix]<0) TapVols[ix]=0;
        if (TapVols[ix]>1) TapVols[ix]=1;
        if ( TapTimes[ix]<0)  TapTimes[ix]=0;
       // Serial.printf("%d TapTimes: %d TapVol% 2.1f\n",ix,TapTimes[ix],TapVols[ix]);
        }

        Serial.printf("RevTron Tapcount %d TapSize %d\n",TapCount,TapSize);
    }
    void Process(int16_t &L,int16_t &R)
    {
    int Mono=(L+R)/(2*16);
    float lout=0;
    float rout=0;
    float mono=0;
    for  (int ix=0;ix<TapCount;ix++)
      {
      int DT=(ReadP+TapTimes[ix]);
      if (DT>=TapSize) DT-=TapSize;
      mono+=float(DBuf[DT])*TapVols[ix];
      }
   

    mono=R7.process(mono);
    mono=R8.process(mono);
    mono=R9.process(mono);
    mono=L7.process(mono);
    mono=L8.process(mono);
    mono=L9.process(mono);

    rout=R1.process(mono);
    rout=R2.process(rout);
    rout=R3.process(rout);    
    rout=R4.process(rout);
    rout=R5.process(rout);
    rout=R6.process(rout); 
 
    lout=L1.process(mono);
    lout=L2.process(lout);
    lout=L3.process(lout);   
    lout=L4.process(lout);
    lout=L5.process(lout);
    lout=L6.process(lout);   

   
    //if (rout>32767) rout=32767;
    //if (rout<-32767) rout=-32767;
    //if (lout>32767) lout=32767;
    //if (lout<-32767) lout=-32767;
 
    L=sclamp((float)L*Dry+2.5*lout*Wet);
    R=sclamp((float)R*Dry+2.5*rout*Wet);
    DBuf[ReadP]=Mono;
    ReadP+=1;
    if (ReadP>=TapSize) ReadP=0;
    }

};
static int PMCount=0;
DMAMEM ReverseOTron RevTron;
// (C) S.D.Smith 2024 - all rights reserved
class PeakMeter : public AudioStream
{
  public:
    audio_block_t *inputQueueArray[2];
    float PeakDamp;
    bool AudioSwap;
int ID;
    PeakMeter() : AudioStream(2, inputQueueArray) 
      {
        ID=PMCount++;
        PeakDamp=0;
        AudioSwap=false;
      }

    float ReadPeak() {return clamp(PeakDamp/32768.f,0.f,1.f);}  
    void SwapChannels(bool swap){AudioSwap=swap;}

    void update()
      {
      audio_block_t *blockL = receiveWritable(0); 
      audio_block_t *blockR = receiveWritable(1); 
      if (blockL && blockR)
        {
        for (int i=0; i< AUDIO_BLOCK_SAMPLES; i++) 
          {
         if (ID==1) RevTron.Process(blockL->data[i],blockR->data[i]);
          PeakDamp = max(PeakDamp,(float)abs(blockL->data[i]));
          PeakDamp = max(PeakDamp,(float)abs(blockR->data[i]));
          if (AudioSwap)
          {
            int16_t temp=blockL->data[i];
           blockL->data[i]=blockR->data[i];
           blockR->data[i]=temp; 
          }
          // too lazy to find fpu dnz mask
          // also too lazy to time constant this
          if (PeakDamp>(1.f))  PeakDamp*=0.9999; 
 
          }
        }
    if (blockL) { transmit(blockL,0); release(blockL); }
    if (blockR) { transmit(blockR,1); release(blockR); }

    } 
};



PeakMeter USBPeak;
PeakMeter LineInPeak;
PeakMeter OutputPeak;

AudioConnection          patchCord1(USBIn,0, USBPeak,0);
AudioConnection          patchCord2(USBIn,1, USBPeak,1);

AudioConnection          patchCord3(USBPeak, 0, mixer1, 1);
AudioConnection          patchCord4(USBPeak, 1, mixer2, 1);

AudioConnection          patchCord5(I2SIn,0, LineInPeak,0);
AudioConnection          patchCord6(I2SIn,1, LineInPeak,1);

AudioConnection          patchCord7(LineInPeak, 0, mixer1, 0);
AudioConnection          patchCord8(LineInPeak, 1, mixer2, 0);

AudioConnection          patchCord13(mixer1, 0, OutputPeak, 0);
AudioConnection          patchCord14(mixer2, 0, OutputPeak, 1);

AudioConnection          patchCord15(mixer1, 0, USBOut, 0);
AudioConnection          patchCord16(mixer2, 0, USBOut, 1);

AudioConnection          patchCord17(mixer1, 0, I2SOut, 0);
AudioConnection          patchCord18(mixer2, 0, I2SOut, 1);








#include <Arduino_GFX_Library.h>
#include <SPI.h>
#define GFX_BL DF_GFX_BL  // default backlight pin, you may replace DF_GFX_BL to actual backlight pin

#define PIN_SCK 13    // mandatory
#define PIN_MISO 255  // mandatory  (if the display has no MISO line, set this to 255 but then VSync will be disabled)
#define PIN_MOSI 11   // mandatory
#define PIN_DC 10     // mandatory, can be any pin but using pin 0 (or 38 on T4.1) provides greater performance
#define PIN_CS 255     // optional (but recommended), can be any pin.
#define PIN_RESET 255  // optional (but recommended), can be any pin.



// POWER USAGE
// 600mhz+3.5" screen 173ma
// 600mhz   80ma
// SDS - spi set to run at 90mhz in GFX_LIbrary_for_Arduino\src\arduino_databus.h:
//// Teensy 4.x
//#elif defined(__IMXRT1052__) || defined(__IMXRT1062__) // SDS Changed
//#define SPI_DEFAULT_FREQ 90000000

Arduino_DataBus *bus = new Arduino_HWSPI(PIN_DC /* DC */, -1 /* CS */, &SPI);
Arduino_GFX *gfx = new Arduino_ILI9341(bus, PIN_RESET, 0 /* rotation */, true /* IPS */);

static const uint16_t screenWidth  = 320;
static const uint16_t screenHeight = 240;
//#define LV_COLOR_DEPTH 16
#define DRAW_BUF_SIZE (screenWidth * screenHeight / 10 * (LV_COLOR_DEPTH / 8))
//static lv_disp_draw_buf_t draw_buf;
DMAMEM static uint8_t draw_buf[DRAW_BUF_SIZE];



uint32_t bufSize;




uint32_t millis_cb(void) 
{
  //return elapsedMillis();
  return millis();
}



#include "FT6x36.H"
FT6X36 XTouch(&Wire, -1);

void RotatePoints(int &x,int &y)
{
int rot=gfx->getRotation();
if (rot==1)  {int t=x;  x=y;  y=t; }
if (rot==2)  { }
// Rotate anti clockwise 90
if (rot==3)  { int t=x;  x=screenWidth-y;  y=t; }
}



void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) 
{

    if (XTouch.touched()) 
    {

    XTouch.readData();
    int TX = XTouch._touchX[0];
    int TY = XTouch._touchY[0];
    RotatePoints( TX , TY );
    data->state =  LV_INDEV_STATE_PRESSED ;
    data->point.x = TX;
    data->point.y = TY;
    } 
  else 
    {
    data->state = LV_INDEV_STATE_RELEASED;
    }
}

static uint32_t my_tick(void)
{
    return millis();
}





/* Display flushing */
//void my_disp_flush( lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p )
void my_disp_flush( lv_display_t *disp, const lv_area_t *area, uint8_t * px_map)
{
    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);

  gfx->draw16bitRGBBitmap(area->x1, area->y1, ( uint16_t * )px_map, w, h);

  lv_disp_flush_ready(disp);
}


 lv_obj_t *Label1=0;
 lv_obj_t *USBBar=0;
 lv_obj_t *LineInBar=0; 
lv_obj_t *OutputBar=0;
 lv_obj_t *LastObj=0;
  lv_obj_t * Canvas =0;
lv_obj_t *TileView=0;
  void event_handler(lv_event_t * event) 
  {
       lv_obj_set_style_bg_color(lv_event_get_target(event),lv_color_hex(0x800000), LV_PART_MAIN);
       Serial.printf("Event Handler Called\n");
  }

void ObjXYWH(int x,int y,int w,int h,lv_obj_t* obj=LastObj)
{
    lv_obj_set_x(obj, x);
    lv_obj_set_y(obj, y);
    lv_obj_set_width(obj, w);
    lv_obj_set_height(obj, h);
}
 void setup() 
 {
  Serial.begin(115200);
  RevTron.ParamsChanged();
  Wire.begin();
  sd.begin(SdioConfig(FIFO_SDIO));
  FileCount = 0;
  DirCount = 0;
  DirScan("/music");
  Serial.printf("DirCount %d\n", DirCount);
  Serial.printf("FileCount %d\n", FileCount);

  if (!XTouch.begin(1))
    Serial.println("Error - FT6336 not found");

  AudioMemory(12); // Audio Blocks for processing connections
  
  sgtl5000_1.enable();
  sgtl5000_1.volume(0);
  sgtl5000_1.muteHeadphone();
  sgtl5000_1.muteLineout();
  sgtl5000_1.audioProcessorDisable();
 
  // Headphone volume 0.8 is max level undistorted
  sgtl5000_1.volume(0);
  
  sgtl5000_1.unmuteHeadphone();
  sgtl5000_1.unmuteLineout();

  sgtl5000_1.dacVolume(1); // float 0-1 - amps the digital stream
  sgtl5000_1.audioPostProcessorEnable();
  sgtl5000_1.enhanceBassEnable();
 
  // Init Display
  if (!gfx->begin())
    Serial.println("gfx->begin() failed!");

  gfx->setRotation(3);
  gfx->fillScreen(BLACK);

  lv_init();
  lv_tick_set_cb(my_tick);


 
  lv_display_t * disp;
    disp = lv_display_create(screenWidth, screenHeight);
    lv_display_set_flush_cb(disp, my_disp_flush);
    lv_display_set_buffers(disp, draw_buf, NULL, DRAW_BUF_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);


    lv_indev_t * indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER); /*Touchpad should have POINTER type*/
    lv_indev_set_read_cb(indev, my_touchpad_read);


 
 
 lv_obj_t *screen = lv_screen_active();

    lv_obj_set_style_bg_color(screen,lv_color_hex(0x000000),LV_PART_MAIN);

    lv_obj_set_x(screen, 0);
    lv_obj_set_y(screen, 0);
    lv_obj_set_width(screen, 320);
    lv_obj_set_height(screen, 240);



  
    LastObj = lv_btn_create(screen);
    ObjXYWH(420-160,218,60,15);
    lv_obj_set_style_bg_color(LastObj, lv_color_hex(0x808080), LV_PART_MAIN);

    LastObj = lv_label_create(LastObj);
    lv_label_set_text(LastObj, "FX List");
    lv_obj_center(LastObj);

    LastObj = lv_btn_create(screen);
    ObjXYWH(160,218,60,15);
    lv_obj_set_style_bg_color(LastObj, lv_color_hex(0x808080), LV_PART_MAIN);

    LastObj = lv_label_create(LastObj);
    lv_label_set_text(LastObj, "Config");
    lv_obj_center(LastObj);

   char tmp[1024];
// Buttons!
 for (int a=0;a<4;a++)
 {
    // Top row params
    sprintf(tmp,"Param-%d",a+1);
    LastObj = lv_btn_create(screen);
    lv_obj_set_style_radius( LastObj, 8, 8 ) ;
    ObjXYWH(0+a*80,0,80,15);
    lv_obj_set_style_bg_color(LastObj,lv_color_hex(0x000080), LV_PART_MAIN);
    LastObj = lv_label_create(LastObj);
    Params[a]=LastObj;
    lv_label_set_text(LastObj, "");
    lv_obj_center(LastObj); 

    // Bottom Row params
    sprintf(tmp,"Param-%d",a+4+1);
    LastObj = lv_btn_create(screen);
    ObjXYWH(0+a*80,19,80,15);
   lv_obj_set_style_bg_color(LastObj,lv_color_hex(0x000060), LV_PART_MAIN);
    LastObj = lv_label_create(LastObj);
    Params[a+4]=LastObj;
    lv_label_set_text(LastObj, "");
    lv_obj_center(LastObj);
 }

  USBBar = lv_bar_create(screen);
  lv_bar_set_range(USBBar,0,100);
  lv_bar_set_orientation(USBBar,LV_BAR_ORIENTATION_VERTICAL);
  LastObj=USBBar;
  ObjXYWH(4,45,4,240-45-25);

  LineInBar = lv_bar_create(screen);
  lv_bar_set_range(LineInBar,0,100);
  lv_bar_set_orientation(LineInBar,LV_BAR_ORIENTATION_VERTICAL);
  LastObj=LineInBar;
  ObjXYWH(12,45,4,240-45-25);

  OutputBar = lv_bar_create(screen);
  lv_bar_set_range(OutputBar,0,100);
  lv_bar_set_orientation(OutputBar,LV_BAR_ORIENTATION_VERTICAL);
  LastObj=OutputBar;
  ObjXYWH(320-4-4,45,4,240-45-25);

  lv_obj_set_style_bg_color(USBBar, lv_color_hex(0x404040), LV_PART_MAIN);
  lv_obj_set_style_bg_color(USBBar, lv_color_hex(0x0080ff), LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(LineInBar, lv_color_hex(0x404040), LV_PART_MAIN);
  lv_obj_set_style_bg_color(LineInBar, lv_color_hex(0xff8000), LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(OutputBar, lv_color_hex(0x404040), LV_PART_MAIN);
  lv_obj_set_style_bg_color(OutputBar, lv_color_hex(0xffff00), LV_PART_INDICATOR);


// Text Label
  LastObj = lv_obj_create(screen);
  ObjXYWH(0,220,90,15);
  lv_obj_set_style_bg_color(LastObj , lv_color_hex(0xffffff),LV_PART_MAIN);

  LastObj = lv_label_create(LastObj);
  Label1=LastObj;
  lv_label_set_text(LastObj, "Test");
  lv_obj_center(LastObj);
  // Listbox
  LastObj = lv_tileview_create(screen);
  TileView=LastObj;
  //lv_obj_set_scrollbar_mode(TileView, LV_SCROLLBAR_MODE_OFF);
  ObjXYWH(20,42,320-20-12,240-45-25);
  // Tile View
   lv_obj_t *tile = lv_tileview_add_tile(TileView, 0, 0, LV_DIR_TOP | LV_DIR_RIGHT);
    // RevTron
 
    lv_obj_t * btn = lv_button_create(tile);
    lv_obj_set_style_bg_color(btn,lv_color_hex(0x008000), LV_PART_MAIN);
    lv_obj_t *label = lv_label_create(btn);
    lv_obj_add_event_cb(btn, event_handler,LV_EVENT_CLICKED, 0);
    lv_label_set_text(label, "On");
    lv_obj_set_size(btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

    label = lv_label_create(tile);
    lv_label_set_text(label, "Reverse-O-Tron");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    lv_obj_set_x(label, 80);
    lv_obj_set_y(label, 0);
    lv_obj_set_size(btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    // Loop Recorder
    tile = lv_tileview_add_tile(LastObj, 1, 0, LV_DIR_TOP | LV_DIR_RIGHT);
    btn = lv_button_create(tile);
    lv_obj_set_style_bg_color(btn,lv_color_hex(0x008000), LV_PART_MAIN);
    label = lv_label_create(btn);
    lv_obj_add_event_cb(btn, event_handler,LV_EVENT_CLICKED, 0);
    lv_label_set_text(label, "On");
    lv_obj_set_size(btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    label = lv_label_create(tile);
    lv_label_set_text(label, "Loop Recorder");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    lv_obj_set_x(label, 100);
    lv_obj_set_y(label, 0);
    lv_obj_set_size(btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    // FFT Reverb
    tile = lv_tileview_add_tile(LastObj, 2, 0, LV_DIR_TOP | LV_DIR_RIGHT);
    btn = lv_button_create(tile);
    lv_obj_set_style_bg_color(btn,lv_color_hex(0x008000), LV_PART_MAIN);
    label = lv_label_create(btn);
    lv_obj_add_event_cb(btn, event_handler,LV_EVENT_CLICKED, 0);
    lv_label_set_text(label, "On");
    lv_obj_set_size(btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    label = lv_label_create(tile);
    lv_label_set_text(label, "FFT Reverb");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    lv_obj_set_x(label, 100);
    lv_obj_set_y(label, 0);
    lv_obj_set_size(btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);


  Serial.println("Setup done");
  analogReadResolution(12);  
  analogReadAveraging(32);
}
void CanvasExample()
{

}

float clamp(float v,float min,float max)
{
  if (v<min) v=min;
  if (v>max) v=max;
  return v;
}
const int ADCShift=4;
const float ADCDiv=256;
void ReadAnaloguePins()
{
  VolPots[0]=(float)(analogRead(A0)>>ADCShift)/ADCDiv;
  VolPots[1]=(float)(analogRead(A1)>>ADCShift)/ADCDiv;
  VolPots[2]=(float)(analogRead(A2)>>ADCShift)/ADCDiv;
  
   // A0 Headphone Volume, pow curve 1/3 seems more audibly linear
   // Audio gets distorted if volume is 0.8 or above
   sgtl5000_1.volume(powf(clamp(1.0f-(VolPots[0]),0,1),1.f/2.5)*0.8f);
   // A1 Input level
   int LOV=(powf(1-clamp(1.0f-((float)VolPots[1]),0,1),1.f/2.5)* 19);
   if (LOV>=18) sgtl5000_1.muteLineout(); else sgtl5000_1.unmuteLineout();
   sgtl5000_1.lineOutLevel(LOV+12);   
   // A2 Input level
   sgtl5000_1.lineInLevel((powf(clamp(1.0f-(VolPots[2]),0,1),1.f/2.5)* 15 ));
   //Serial.printf("A0: %2.8f A1: %2.8f A2: %2.8f\n",VolPots[0],VolPots[1],VolPots[2]);
   // Top Params
  //Serial.printf("A10: %d A11: %d A12: %d A13: %d\n",analogRead(A10),analogRead(A11),analogRead(A12),analogRead(A13));
  //Serial.printf("A14: %d A15: %d A16: %d A17: %d\n",analogRead(A14),analogRead(A15),analogRead(A16),analogRead(A17));
  //Serial.printf("A10: %2.8f A11: %2.8f A12: %2.8f A13: %2.8f\n",Pots[0],Pots[1],Pots[2],Pots[3]);
  //Serial.printf("RotEnc: %d",RotEnc.read());
  Pots[0]=1-(float)(analogRead(A10)>>ADCShift)/ADCDiv;
  Pots[1]=1-(float)(analogRead(A11)>>ADCShift)/ADCDiv;
  Pots[2]=1-(float)(analogRead(A12)>>ADCShift)/ADCDiv;
  Pots[3]=1-(float)(analogRead(A13)>>ADCShift)/ADCDiv;   

  Pots[4]=1-(float)(analogRead(A14)>>ADCShift)/ADCDiv;
  Pots[5]=1-(float)(analogRead(A15)>>ADCShift)/ADCDiv;
  Pots[6]=1-(float)(analogRead(A16)>>ADCShift)/ADCDiv;
  Pots[7]=1-(float)(analogRead(A17)>>ADCShift)/ADCDiv;   

  RevTron.SetTime(Pots[0]*3.0);
  RevTron.SetWet(Pots[6]);
  RevTron.SetDry(Pots[7]);
  RevTron.SetGateDuck((Pots[5]-0.5)*2);
static int CurrentTile=0;
  static int LastRot=RotEnc.read();
  if (RotEnc.read()!=LastRot)
  {
    if (LastRot<RotEnc.read())
    {
      CurrentTile++;
      if (CurrentTile>2) CurrentTile=0;
      lv_tileview_set_tile_by_index(TileView,CurrentTile,0,LV_ANIM_ON );
    }
    else
    {
    CurrentTile--;
    if (CurrentTile<0) CurrentTile=2;
    lv_tileview_set_tile_by_index(TileView,CurrentTile,0,LV_ANIM_ON );
    }
    LastRot=RotEnc.read();
    Serial.printf("RotEnc %d\n",LastRot);
  }
}

int LoopCount=0;// Used for Various things

void loop() 
{
  LoopCount++;
  if (LoopCount%50==0)
  {
  char tmp[256];
  sprintf(tmp,"APU: %3.1f%%",AudioProcessorUsage());
  lv_label_set_text(Label1, tmp);
  }
  if (LoopCount%30==0)
    ReadAnaloguePins();
  if (LoopCount%7==0)
  {
    lv_bar_set_value(USBBar,USBPeak.ReadPeak()*100,LV_ANIM_OFF);
    lv_bar_set_value(LineInBar,LineInPeak.ReadPeak()*100,LV_ANIM_OFF);   
    lv_bar_set_value(OutputBar,OutputPeak.ReadPeak()*100,LV_ANIM_OFF);
  }
  lv_timer_handler(); /* let the GUI do its work */
  delay(5);
}
