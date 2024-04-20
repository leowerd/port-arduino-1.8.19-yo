/*******************************************************************
Плагин для ёRadio v0.9.300 и выше https://github.com/e2002/yoradio

Плагин сырой но работает... 

*******************************************************************/

#include <GyverMAX7219.h>
#include <RunningGFX.h>
#include <VolAnalyzer.h>
#include <FastLED.h>


class Tmr
{
public:
Tmr() {}
Tmr(uint16_t ms)
  {
  start(ms);
    }

void start(uint16_t ms)
  {
  _prd = ms;
  if (_prd) start();
    }
void start()
  {
  if (!_prd) return;
  _tmr = millis();
  if (!_tmr) _tmr = 1;
    }
void timerMode(bool mode)
  {
  _mode = mode;
    }
void stop()
  {
  _tmr = 0;
  }
bool state()
  {
  return _tmr;
  }
bool tick()
  {
  return (_tmr && millis() - _tmr >= _prd) ? ((_mode ? stop() : start()), true) : false;
  }
operator bool()
  {
  return tick();
  }

private:
uint32_t _tmr = 0, _prd = 0;
bool _mode = 0;
};

#define ANALYZ_WIDTH (3 * 8)

#define CLK_PIN  21// CLK max7219  
#define DATA_PIN 23// DI max7219 
#define CS_PIN   22// CS max7219
#define AMP   14	// включение усилителя на 14пин

MAX7219<5, 1, CS_PIN, DATA_PIN, CLK_PIN> mtrx;
RunningGFX run(&mtrx);

char pstr_g[100] = "ё-радио Bender";

byte vol_old;
byte num_old;
bool pulse = 0;
byte inf = 0;

#define STACK_SIZE  1024*2 /* Размер стека функции */

TaskHandle_t MaxTask;

#define NUMNEXT   display.numOfNextStation //
#define VOLUME    config.store.volume
#define WEATHER_S config.store.showweather // триггер показа погоды
#define NUMSTAT_S config.store.numplaylist // триггер показа номера станции
#define NUMSTAT   config.store.lastStation // номер станции
#define NAMESTAT  config.station.name      // имя станции
#define EYES	config.store.brightness
#define MOUTH	config.store.contrast
#define TITLE     config.station.title

VolAnalyzer sound;
Tmr square_tmr(300);
Tmr eye_tmr(150);
Tmr matrix_tmr(1000);

void yoradio_on_setup()
{
pinMode(AMP, OUTPUT);	// пин AMP как выход
mtrx.setBright(5);		// яркость 0..15
xTaskCreatePinnedToCore(loopMaxTask, "MaxTask", STACK_SIZE,  NULL,  4, &MaxTask, !xPortGetCoreID());
mtrx.begin();
run.setSpeed(30);
run.setText_P(pstr_g);
run.setWindow(0, ANALYZ_WIDTH-1, 0);
run.start();
}

void upd_bright()
{
uint8_t m = MOUTH, e = EYES;
uint8_t br[] = {m, m, m, e, e};
mtrx.setBright(br);
}

void print_val(char c, uint8_t v)
{
mtrx.rect(0, 0, ANALYZ_WIDTH - 1, 7, GFX_CLEAR);
mtrx.setCursor(8 * 0 + 2, 1);
mtrx.print(c);
mtrx.setCursor(8 * 1 + 2, 1);
mtrx.print(v / 10);
mtrx.setCursor(8 * 2 + 2, 1);
mtrx.print(v % 10);
mtrx.update();
}

// ========================= EYES =========================
void draw_eye(uint8_t i)
{
uint8_t x = ANALYZ_WIDTH + i * 8;
mtrx.rect(1 + x, 1, 6 + x, 6, GFX_FILL);
mtrx.lineV(0 + x, 2, 5);
mtrx.lineV(7 + x, 2, 5);
mtrx.lineH(0, 2 + x, 5 + x);
mtrx.lineH(7, 2 + x, 5 + x);
}

void draw_eyeb(uint8_t i, int x, int y, int w = 2)
{
x += ANALYZ_WIDTH + i * 8;
mtrx.rect(x, y, x + w - 1, y + w - 1, GFX_CLEAR);
}

void eye_ch()
{
upd_bright();
draw_eye(0);
draw_eye(1);
draw_eyeb(0, 4, 3, 3);
draw_eyeb(1, 1, 3, 3);
mtrx.update();
}

void anim_search()
{
static int8_t pos = 4, dir = 1;
static Tmr tmr(50);
if (tmr)
	{
	pos += dir;
	if (pos >= 6) dir = -1;
	if (pos <= 0) dir = 1;
	mtrx.rect(ANALYZ_WIDTH, 2, ANALYZ_WIDTH + 16 - 1, 5, GFX_FILL);
	draw_eyeb(0, pos, 3);
	draw_eyeb(1, pos, 3);
	mtrx.update();
	}
}

void Show_time(uint8_t h, uint8_t m)
{
drawNumber(h/10, ANALYZ_WIDTH+0, 1); // Первая цифра часа
drawNumber(h%10, ANALYZ_WIDTH+4, 1); // вторая цифра часа
drawNumber(m/10, ANALYZ_WIDTH+9, 1); // Первая цифра минут
drawNumber(m%10, ANALYZ_WIDTH+13, 1); // вторая цифра минут  nowTm.minute()	
mtrx.update();
}

void square_state()
{
upd_bright();
square_tmr.start(400);
draw_eye(0);
draw_eye(1);
draw_eyeb(0, 2, 2, 4);
draw_eyeb(1, 2, 2, 4);
mtrx.update();
}


void angry_state()
{
square_tmr.start(400);
draw_eye(0);
draw_eye(1);
draw_eyeb(0, 3, 3);
draw_eyeb(1, 3, 3);
mtrx.lineH(0, ANALYZ_WIDTH, ANALYZ_WIDTH + 16 - 1, GFX_CLEAR);
mtrx.lineH(1, ANALYZ_WIDTH + 5, ANALYZ_WIDTH + 5 + 6 - 1, GFX_CLEAR);
mtrx.lineH(2, ANALYZ_WIDTH + 6, ANALYZ_WIDTH + 6 + 4 - 1, GFX_CLEAR);
mtrx.lineH(3, ANALYZ_WIDTH + 7, ANALYZ_WIDTH + 7 + 2 - 1, GFX_CLEAR);
mtrx.update();
}

void change_state()
{
square_tmr.start(1000);
mtrx.setBright(5);
draw_eye(0);
draw_eye(1);
mtrx.rect(ANALYZ_WIDTH, 0, ANALYZ_WIDTH + 16 - 1, 3, GFX_CLEAR);
draw_eyeb(0, 3, 5);
draw_eyeb(1, 3, 5);
mtrx.update();
}

void run_str()
{
strcpy(pstr_g, TITLE);
run.setText(pstr_g);
run.start();
}

void drawNumber(int num, int x, int y)
{
 // Обновленные паттерны для каждой цифры с увеличенной высотой
byte patterns[10][6][3] = {
	{{1,1,1}, {1,0,1}, {1,0,1}, {1,0,1}, {1,0,1}, {1,1,1}}, // 0
	{{0,1,0}, {1,1,0}, {0,1,0}, {0,1,0}, {0,1,0}, {1,1,1}}, // 1
	{{1,1,1}, {0,0,1}, {0,0,1}, {1,1,1}, {1,0,0}, {1,1,1}}, // 2
	{{1,1,1}, {0,0,1}, {1,1,1}, {0,0,1}, {0,0,1}, {1,1,1}}, // 3
	{{1,0,1}, {1,0,1}, {1,0,1}, {1,1,1}, {0,0,1}, {0,0,1}}, // 4
	{{1,1,1}, {1,0,0}, {1,1,1}, {0,0,1}, {0,0,1}, {1,1,1}}, // 5
	{{1,1,1}, {1,0,0}, {1,0,0}, {1,1,1}, {1,0,1}, {1,1,1}}, // 6
	{{1,1,1}, {0,0,1}, {0,0,1}, {0,0,1}, {0,0,1}, {0,0,1}}, // 7
	{{1,1,1}, {1,0,1}, {1,1,1}, {1,0,1}, {1,0,1}, {1,1,1}}, // 8
	{{1,1,1}, {1,0,1}, {1,1,1}, {0,0,1}, {0,0,1}, {1,1,1}}};// 9

 // Рисуем цифру в соответствии с паттерном
for (int i = 0; i < 6; i++)
	{
	for (int j = 0; j < 3; j++)
		{
		mtrx.dot(x + j, y + i, patterns[num][i][j]);	
	//	if (patterns[num][i][j] == 1) mtrx.dot(x + j, y + i);
		}
	}
}

// ========================= ANALYZ =========================

void analyz0(uint8_t vol)
{
static uint16_t offs;
offs += 20 * vol / 100;
for (uint8_t i = 0; i < ANALYZ_WIDTH; i++)
	{
	int16_t val = inoise8(i * 50, offs);
	val -= 128;
	val = val * vol / 100;
	val += 128;
	val = map(val, 45, 255 - 45, 0, 7);
	mtrx.dot(i, val);
    }	
}

int gettime()
{
#if RTCSUPPORTED
rtc.getTime(&network.timeinfo);
#endif
strftime(pstr_g , sizeof(pstr_g), "%H%M", &network.timeinfo);
return atoi(pstr_g);
}


void loopMaxTask(void * pvParameters)
{
sound.setAmpliDt(500);
sound.setTrsh(0);
sound.setPulseMin(80);
sound.setPulseMax(99); 
square_tmr.timerMode(1);

while (WiFi.status() != WL_CONNECTED)
	{
	anim_search();
	run.tick();
	if (WiFi.getMode()==WIFI_AP)
		{
		run.stop();
		strcpy(pstr_g, "Создана точка: ");
		strcat(pstr_g, apSsid);	
		strcat(pstr_g, " с паролем: ");	
		strcat(pstr_g, apPassword);	
		run.setText(pstr_g);
		run.start();
		eye_ch();
		break;
		}
	}
	
while (WiFi.getMode()==WIFI_AP)
	{
	if (run.tick()==2)
		{
		run.stop();	
		#if RTCSUPPORTED
		mtrx.clear();
		Show_time(gettime()/100, gettime()%100);
		#endif
		strcpy(pstr_g, "для настройки зайдите на http:/192.168.4.1/ ");
		run.setText(pstr_g);
		run.start();
		}
	delay(10);
	}
square_state();
vol_old = VOLUME;
num_old = NUMSTAT;

while(true)
{		// ============ LOOP ===============
square_tmr.tick();

max7219();

vTaskDelay(1);//
}		// ============ LOOP ===============
vTaskDelete( NULL );//
}	//loopMaxTask
//------------------------------------------------------

void max7219()
{
static int f_vol = 0;
static int f_num = 0;

// изменение номера станции   
if (NUMSTAT != num_old) 
	{
	eye_ch();
	num_old = NUMSTAT;
	print_val('s', NUMSTAT);
	f_num= 100 * 2;
	inf=1;
	}
	
if (f_num != 0) 
	{
	f_num--;
	if (f_num >0)
		{
		delay(10);
		return;  
		}
	else
		{
		run.setText(NAMESTAT);
		run.start();
		square_state();
		}
	}
// изменение номера станции end 

// громкость      
if (VOLUME != vol_old) 
	{
	if (player.status() == PLAYING)
		angry_state();
	vol_old = VOLUME;
	f_vol= 100 * 2;
	print_val('v', map(VOLUME, 0, 254, 0, 99));
	}
if (f_vol != 0) 
	{
	f_vol--;
	if (f_vol >0)
		{
		delay(10);
		return; 	
		}
	else
		{
		if (player.status() != PLAYING)
			{
			mtrx.rect(0, 0, ANALYZ_WIDTH - 1, 7, GFX_CLEAR);
			mtrx.update();
			}
		}
	}
// громкость  end 

if (display.mode()==PLAYER)
	{
	if (player.status() == PLAYING)		//-----------PLAYNG-------------------
		{
		switch (inf)
			{
			case 0:
			if (run.tick()==2)
				{
			//	eye_ch();
				digitalWrite (AMP, LOW);
				strcpy(pstr_g, "подключено с ip: ");
				strcat(pstr_g, WiFi.localIP().toString().c_str());
				strcat(pstr_g, " ");
				strcat(pstr_g, NAMESTAT);
				run.setText(pstr_g);
				run.start();
				inf=10;
				}
			break;	
				
			case 1 ... 9:		
			mtrx.clear();
			square_state();
			inf=10;
			digitalWrite (AMP, LOW);
			strcpy(pstr_g, NAMESTAT);
			run.setText(pstr_g);
			run.start();
			break;
			
			case 10:
			if (run.tick()==2)
				{
				run.stop();
				inf=11;
				}
			break;
			
			case 11:
			{
			if (strncmp(pstr_g, TITLE, 7)!=0)
				{
				run_str();
				inf=10;
				}
			
			player.getVUlevel();
			if (sound.tick(map(player.vuLeft, 0, 254, 0, 127)))	
				{
				if (sound.pulse()) pulse = 1;
				mtrx.rect(0, 0, ANALYZ_WIDTH - 1, 7, GFX_CLEAR);
				analyz0(sound.getVol());
				mtrx.update();
				}
			}
			break;
			
			}
		
		if (eye_tmr and !square_tmr.state())
			{
			draw_eye(0);
			draw_eye(1);
			static uint16_t pos;
			pos += 15;
			uint8_t x = inoise8(pos);
			uint8_t y = inoise8(pos + UINT16_MAX / 4);
			x = constrain(x, 40, 255 - 40);
			y = constrain(y, 40, 255 - 40);
			x = map(x, 40, 255 - 40, 2, 5);
			y = map(y, 40, 255 - 40, 2, 5);
			
			if (pulse)
				{
				pulse = 0;
				int8_t sx = random(-1, 1);
				int8_t sy = random(-1, 1);
				draw_eyeb(0, x + sx, y + sy, 3);	
				draw_eyeb(1, x + sx, y + sy, 3);
				}
			else
				{
				draw_eyeb(0, x, y);
				draw_eyeb(1, x, y);
				}
			mtrx.update();
			}	
		}
		else
		{			//-----------STOPPED-------------------
		switch (inf)
			{
			case 0:
			if (run.tick()==2)
				{
				change_state();	
				digitalWrite (AMP, HIGH);
				strcpy(pstr_g, "подключено с ip: ");
				strcat(pstr_g, WiFi.localIP().toString().c_str());
				run.setText(pstr_g);
				run.start();
				inf=1;
				}
			break;
			case 10:
			case 11:
			change_state();	
			inf=1;
			digitalWrite (AMP, HIGH);
			strcpy(pstr_g, TITLE);
			run.setText(pstr_g);
			run.start();
			break;
			
			case 1:	
			if (run.tick()==2)
				{
				inf=2;
				mtrx.clear();
				Show_time(gettime()/100, gettime()%100);
				}
			break;
			
			case 2:
			if (matrix_tmr)
				Show_time(gettime()/100, gettime()%100);
			break;
			}	
		}
	}
else
if (display.mode()==LOST)
	{
	if (inf>0)
		{
		inf=0;
		run.setText("Соединение потеряно");
		run.start();		
		change_state();	
		}

	if (run.tick()==2)
		{
		digitalWrite (AMP, HIGH);
		mtrx.clear();
		Show_time(gettime()/100, gettime()%100);
		}
	delay(10);		
		
	}
else
if (display.mode()==UPDATING)
	{
	if (inf>0)
		{
		inf=0;
		run.setText(const_DlgUpdate);
		run.start();		
		change_state();	
		}
	run.tick();
	}
else
	{
	change_state();
	if (inf>0)
		{
		change_state();	
		inf=0;	
		strcpy(pstr_g, TITLE);
		run.setText(pstr_g);
		run.start();
		}
	run.tick();
	}
}			//void max7219()
