// Arduino-Pin-Verbindungsmethode, die folgenden Nummern werden entsprechend dem IO des Geräts geändert
// Dieses Beispiel verwendet ESP8266-12
// In diesem Beispiel wird eine 6-stellige, 8-stellige, 12-stellige, 16-stellige 5x7-Anzeige verwendet
// https://mikroshop.ch/LED_LCD.html?gruppe=7&artikel=2546
// https://mikroshop.ch//data/VFD_SPI.zip
// extra info: https://github.com/slabua/micropython-futaba-vfd-driver?tab=readme-ov-file
// This example uses a 6-digit, 8-digit, 12-digit, and 16-digit 5x7 display.
// This copy was uploaded from https://github.com/tehniq3/VFD_SPI
/*
 * Nicu FLORICA (niq_ro) added some new feature to display a text (a s string)
 */

#define din D7
#define clk D5
#define cs  D8
#define Reset 1
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

int ics  = 200;
int vfd_lengh = 6;
 
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
}

void loop() {
/*
for (int i = 0; i <= 10; i++) 
  {
  VFD_WriteStr(0, " 23:54");
  delay(500);
  VFD_WriteStr(0, " 23 54");
  delay(500);
  }
*/

String x2 = " 23:56";
for (int a = 0; a < 10; a++) 
{
VFD_Fix(x2);
delay(500);
VFD_WriteStr(3, " ");
delay(500);
}

String x = "     21/07/2025 - Monday      ";
VFD_Scroll(x, ics);


} // end of main loop
