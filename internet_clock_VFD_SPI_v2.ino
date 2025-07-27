// Arduino-Pin-Verbindungsmethode, die folgenden Nummern werden entsprechend dem IO des Geräts geändert
// Dieses Beispiel verwendet ESP8266-12
// In diesem Beispiel wird eine 6-stellige, 8-stellige, 12-stellige, 16-stellige 5x7-Anzeige verwendet
// https://mikroshop.ch/LED_LCD.html?gruppe=7&artikel=2546
// https://mikroshop.ch//data/VFD_SPI.zip
// extra info: https://github.com/slabua/micropython-futaba-vfd-driver?tab=readme-ov-file
// This example uses a 6-digit, 8-digit, 12-digit, and 16-digit 5x7 display.
// This copy was uploaded from https://github.com/tehniq3/VFD_SPI

/*
 * NTP clock on Futaba 000FV959IN and BOE CIG14-0604B VFD display 
 * Nicu FLORICA (niq_ro) created this NTP clock based on info from  https://thesolaruniverse.wordpress.com/2022/11/01/an-internet-synced-clock-circular-display-with-gc9a01-controller-powered-by-an-esp8266/
 * v.1 - initial version for NTP clock
 * v.1a - added date as scroll text, datee requested as at https://randomnerdtutorials.com/esp8266-nodemcu-date-time-ntp-client-server-arduino/
 * v.2 - clock scroll as date (intro and the end) + moved RESET od display to D1 (not RST)
 * v.2a - real update the seconds as real time
 */

   #include <NTPClient.h>
   #include <ESP8266WiFi.h>
   #include <WiFiUdp.h>

const char* ssid =            "bbk2";                                                            // network wifi credentials  - fill in your wifi network name
const char* password =        "internet2";                                                            // network wifi credentials  - fill in your wifi key

const long utcOffsetInSeconds = 3*3600;                                                                        // 3600 = western europe winter time - 7200 = western europe summer time
String weekDays[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
String months[12]={"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

int monthDay;
String weekDay;
int currentMonth;
String currentMonthName;
int currentYear;
String currentDate;

String intro = "       NTP clock v.2 by niq_ro      ";

#define din D7
#define clk D5
#define cs  D8
#define Reset D1
#define en 0 


//Das Symbol sollte in der unteren linken Ecke beginnen, zuerst von unten nach oben, dann von links nach rechts
//The symbol should start in the lower left corner, first from bottom to top, then from left to right
unsigned char  ziku_data[][5]  ={ 
 0x04,0x02,0x04,0x08,0x30, // 0
 0x41,0x42,0x20,0x10,0x08, // 1
 0x41,0x22,0x10,0x08,0x07, // 2
 0x08,0x44,0x26,0x15,0x0c, // 3
 0x14,0x14,0x7f,0x0a,0x0a, // 4
 0x00,0x00,0x36,0x36,0x00, // 5
 0x41,0x22,0x10,0x08,0x07, // 7
 0x08,0x44,0x26,0x15,0x0c, // 8
 0x14,0x14,0x7f,0x0a,0x0a, // 9
 };

int ics  = 300;     //delay in ms for scroll
int vfd_lengh = 6;  // number of digits

   uint32_t targetTime = 0;                                                            // for next 1 second timeout

   int hh = 0;                                                                         // hours variable
   int mm = 0;                                                                         // minutes variable
   int ss = 0;                                                                         // seconds variable
   int ss1, ss2, ss3;

   
   WiFiUDP ntpUDP;                                                                     // define NTP client to get time
   NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);  

bool initial = 1;


void spi_write_data(unsigned char w_data)
{
  unsigned char i;
  for (i = 0; i < 8; i++)
  {
    digitalWrite(clk, LOW);
    // Wenn Sie ein MCU mit einer schnelleren Verarbeitungsgeschwindigkeit wie ESP32 verwenden, muss hier etwas verzögert werden, die VFD-SPI-Taktfrequenz beträgt wie im Handbuch beschrieben max. 0,5 MHz
    if ( (w_data & 0x01) == 0x01)
    {
      digitalWrite(din, HIGH);
    }
    else
    {
      digitalWrite(din, LOW);
    }
    w_data >>= 1;
    
    digitalWrite(clk, HIGH);
  }
}

void VFD_cmd(unsigned char command)
{
  digitalWrite(cs, LOW);
  spi_write_data(command);
  digitalWrite(cs, HIGH);
  delayMicroseconds(5);
}

void VFD_show(void)
{
  digitalWrite(cs, LOW); //Display wählen
  spi_write_data(0xe8);  //Anzeige Befehl
  digitalWrite(cs, HIGH);//Display abwählen
}

void VFD_init()
{
  //SET HOW MANY digtal numbers
  digitalWrite(cs, LOW);
  spi_write_data(0xe0);
  delayMicroseconds(5);
  spi_write_data(0x07); //6 Zeichen 0x05 // 8 Zeichen 0x07// 16 Zeichen 0x0f
  digitalWrite(cs, HIGH);
  delayMicroseconds(5);

  //Helligkeit einstellen
  digitalWrite(cs, LOW);
  spi_write_data(0xe4);
  delayMicroseconds(5);
  spi_write_data(0xff); //0xff max //0x01 min 
  digitalWrite(cs, HIGH);
  delayMicroseconds(5);
}

/******************************
  Schreiben eines Zeichens an der angegebenen Position (benutzerdefiniert, alles in CG-ROM)
   x: Position, chr: Ascii Code
*******************************/
void VFD_WriteOneChar(unsigned char x, unsigned char chr)
{
  digitalWrite(cs, LOW);     //Display wählen -> Select display
  spi_write_data(0x20 + x);  //Startposition des Adressregisters -> Start position of the address register
  spi_write_data(chr + 0x30);//Daten senden -> Send data
  digitalWrite(cs, HIGH);    //Display abwählen -> Deselect display
  VFD_show();
}

/******************************
  Gibt an der angegebenen Position einen String aus
   (nur für Englisch, Satzzeichen, Zahlen)
    x: Position; str: Wort
*******************************/
void VFD_WriteStr(unsigned char x, char *str)
{
  digitalWrite(cs, LOW);    //Display wählen -> Select display
  spi_write_data(0x20 + x); //Startposition des Adressregisters -> Start position of the address register
  while (*str)
  {
    spi_write_data(*str);   //ASCII Zeichen -> ASCII characters
    str++;
  }
  digitalWrite(cs, HIGH);  //Display abwählen -> Deselect display
  VFD_show();
}

/******************************
 Benutzerdefinierte Zeichen an der angegebenen Position schreiben
  x: Position, mit ROOM-Position *s: anzuzeigende Schriftart
*******************************/
void VFD_WriteUserFont(unsigned char x, unsigned char y,unsigned char *s)
{
  unsigned char i=0;
  unsigned char ii=0;
  digitalWrite(cs, LOW);   //Display wählen -> Select display
    spi_write_data(0x40+y);//Startposition des Adressregisters -> Start position of the address register
    for(i=0;i<7;i++)
    spi_write_data(s[i]);
  digitalWrite(cs, HIGH);  //Display abwählen -> Deselect display

  digitalWrite(cs, LOW);
  spi_write_data(0x20+x);
  spi_write_data(0x00+y);   
  digitalWrite(cs, HIGH);

  VFD_show();
}

/******************************
  Lassen Sie das Symbol an der angegebenen Position aufleuchten für ein
  12 Zeichen Modul kann die folgende Funktion verwenden.
  ad_dat: Position, on_off_flag: welches Symbol leuchtet
*******************************/
void VFD_Write_ADRAM(unsigned char ad_dat , unsigned char on_off_flag)
{
  unsigned char ad_dat_temp;
  digitalWrite(cs, LOW);  //Display wählen -> Select display
  spi_write_data(0x60 + ad_dat);  //ADRAM
  if(on_off_flag==1)     //logo
  {spi_write_data(0x02);}
  else if(on_off_flag==2)//just :
  {spi_write_data(0x01);}
  else if(on_off_flag==3)//logo + :
  {spi_write_data(0x03);}
  else if(on_off_flag==0)//nothing
  {spi_write_data(0x00);}
  digitalWrite(cs, HIGH); //Display abwählen -> Deselect display
  VFD_show();
}

/*
 * Scroll the text, added by niq_ro
 */
void VFD_Scroll(String x1, int ics1) 
{
//String x = "     21/07/2025 - Monday      ";
int y1 = x1.length();
for (int a = 0; a <= y1-vfd_lengh; a++) 
{
for (int b = 0; b < vfd_lengh; b++) 
{
  VFD_WriteOneChar(b, x1[a+b]-48);
}
delay(ics1);
}
}

/*
 * Display the text, added by niq_ro
 */
void VFD_Fix(String x1) 
{
//String x = " 23:56";
int y1 = x1.length();
for (int b = 0; b < vfd_lengh; b++) 
{
  VFD_WriteOneChar(b, x1[b]-48);
}
}

void setup() {
   Serial.begin (115200);
   Serial.println ();
   Serial.println (intro);
   WiFi.begin (ssid, password);

   while (WiFi.status() != WL_CONNECTED ) 
      {
      delay (500);
      Serial.print (".");
      }
   Serial.print ("connection with ");
   Serial.println (ssid);  
   Serial.println ("-------------------------------"); 
   
  pinMode(en, OUTPUT);
  pinMode(clk, OUTPUT);
  pinMode(din, OUTPUT);
  pinMode(cs, OUTPUT);
  pinMode(Reset, OUTPUT);
  digitalWrite(en, HIGH);
  delayMicroseconds(100);
  digitalWrite(Reset, LOW);
  delayMicroseconds(5);
  digitalWrite(Reset, HIGH); 

  VFD_init();
  VFD_cmd(0xE9); // Testbildschirm mit voller Helligkeit -> Test screen at full brightness
  delay(10);
  VFD_Scroll(intro, ics);
 
   timeClient.begin();
   timeClient.update ();

   Serial.print ("internet server time: ");   

   time_t epochTime = timeClient.getEpochTime();
   Serial.print("Epoch Time: ");
   Serial.println(epochTime);
   
   Serial.println(timeClient.getFormattedTime());
 //  Serial.print("Formatted Time: ");
 //  Serial.println(formattedTime);  

   hh = timeClient.getHours ();
   mm = timeClient.getMinutes ();
   ss = timeClient.getSeconds ();
   Serial.print(hh);
   Serial.print(":");
   Serial.print(mm);
   Serial.print(":");
   Serial.println(ss);
   
   weekDay = weekDays[timeClient.getDay()];
   Serial.print("Week Day: ");
   Serial.println(weekDay);  

  //Get a time structure
  struct tm *ptm = gmtime ((time_t *)&epochTime); 

  monthDay = ptm->tm_mday;
  Serial.print("Month day: ");
  Serial.println(monthDay);

  currentMonth = ptm->tm_mon+1;
  Serial.print("Month: ");
  Serial.println(currentMonth);

  currentMonthName = months[currentMonth-1];
  Serial.print("Month name: ");
  Serial.println(currentMonthName);

  currentYear = ptm->tm_year+1900;
  Serial.print("Year: ");
  Serial.println(currentYear);

  //Print complete date:
  currentDate = String(currentYear) + "-" + String(currentMonth) + "-" + String(monthDay);
  Serial.print("Current date: ");
  Serial.println(currentDate);

   ss = timeClient.getSeconds ();  // updte second before loop
   targetTime = millis();  // time to be updated just if it need
}

void loop() {
   if (targetTime < millis())
      {
      targetTime += 1000;
      ss++;                                                                            // advance second
      if (ss>60)
         {
         ss=ss%60;
         mm++;                                                                         // advance minute
         if(mm>59)
            {
            mm=0;
            hh++;                                                                      // advance hour
            if (hh>23) 
               {
               hh=0;
               timeClient.update ();  // update at midnight

   time_t epochTime = timeClient.getEpochTime();
   Serial.print("Epoch Time: ");
   Serial.println(epochTime);
   
   hh = timeClient.getHours ();
   mm = timeClient.getMinutes ();
   ss = timeClient.getSeconds ();
   Serial.print(hh);
   Serial.print(":");
   Serial.print(mm);
   Serial.print(":");
   Serial.println(ss);
   
   weekDay = weekDays[timeClient.getDay()];
   Serial.print("Week Day: ");
   Serial.println(weekDay);  

  //Get a time structure
  struct tm *ptm = gmtime ((time_t *)&epochTime); 

  monthDay = ptm->tm_mday;
  Serial.print("Month day: ");
  Serial.println(monthDay);

  currentMonth = ptm->tm_mon+1;
  Serial.print("Month: ");
  Serial.println(currentMonth);

  currentMonthName = months[currentMonth-1];
  Serial.print("Month name: ");
  Serial.println(currentMonthName);

  currentYear = ptm->tm_year+1900;
  Serial.print("Year: ");
  Serial.println(currentYear);

  //Print complete date:
  currentDate = String(currentYear) + "-" + String(currentMonth) + "-" + String(monthDay);
  Serial.print("Current date: ");
  Serial.println(currentDate); 
               }
            }
         }
   Serial.print("ss = ");
   Serial.println(ss);
  String ora = " ";
  if (hh/10 == 0)
    ora = ora + " ";
   else
    ora = ora + hh/10;
  ora = ora + hh%10;
  if (ss%2 == 0)
    ora = ora + ":";
   else
    ora = ora + " ";
  ora = ora + mm/10;
  ora = ora + mm%10;

 String data = "     ";
  data = data + weekDay;
  data = data + ", ";
  data = data + monthDay;
  data = data + "-";
  data = data + currentMonthName;
  data = data + "-";
  data = data + currentYear;
  data = data + "      ";

  String ora1 = ora + "      "; 

  VFD_Fix(ora);

if (ss == 35)
 {
Serial.print("ss = ");
Serial.print(ss);
ss2 = millis()/1000;
//Serial.print(", ss2 = ");
//Serial.print(ss2);
 VFD_Scroll(ora1, ics/2);
 VFD_Scroll(data, ics);
 
  String ora2 = "     ";
  ora2 = ora2 + ora;
 VFD_Scroll(ora2, ics/2);
  ss3 = millis()/1000 - ss2;
  Serial.print(", ds = ");
  Serial.print(ss3);
 // ss = ss + ss3;
  Serial.print("-> ss = ");
  Serial.println(ss);
 }
      }

delay(10);
} // end of main loop
