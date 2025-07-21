// Arduino-Pin-Verbindungsmethode, die folgenden Nummern werden entsprechend dem IO des Geräts geändert
// Dieses Beispiel verwendet ESP8266-12
// In diesem Beispiel wird eine 6-stellige, 8-stellige, 12-stellige, 16-stellige 5x7-Anzeige verwendet
// https://mikroshop.ch/LED_LCD.html?gruppe=7&artikel=2546
// https://mikroshop.ch//data/VFD_SPI.zip
// extra info: https://github.com/slabua/micropython-futaba-vfd-driver?tab=readme-ov-file
// This example uses a 6-digit, 8-digit, 12-digit, and 16-digit 5x7 display.
// This copy was uploaded from https://github.com/tehniq3/VFD_SPI

uint8_t din   = 11; // DA SDI SPI Daten
uint8_t clk   = 13; // CK CLK SPI Clock
uint8_t cs    = 10; // CS SPI Chip
uint8_t Reset = 1;  // RS VFD Das Zurücksetzen des Bildschirms ist bei niedrigem Pegel aktiv und wird bei normalem Gebrauch hoch gezogen/kein Zurücksetzen, da das Modul über eine eingebaute RC-Hardware-Rücksetzschaltung verfügt
uint8_t en    = 0;  // EN VFD Der Stromversorgungsteil des Moduls ist aktiviert und der EN-Pegel ist high. Es wird empfohlen, den VFD-Initialisierungsbefehl nach 100 ms nach dem Hochsetzen zu senden, um zu vermeiden, dass Befehle gesendet werden, bevor die Stromversorgung des Moduls stabil ist. 

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
  
}

void loop() {

  VFD_cmd(0xE9); // Testbildschirm mit voller Helligkeit -> Test screen at full brightness
  delay(1000);
  
  VFD_WriteOneChar(0, 1);
  VFD_WriteOneChar(1, 2);
  VFD_WriteOneChar(2, 3);
  VFD_WriteOneChar(3, 4);
  VFD_WriteOneChar(4, 5);
  VFD_WriteOneChar(5, 6);
  VFD_WriteOneChar(6, 7);
  delay(1000);
  
  VFD_WriteStr(0, "ABCDEFGH");
  delay(1000);

 //Bitzeichen 0, Schriftart 0, benutzerdefinierter Symbol -> Bit character 0, font 0, custom symbol
  VFD_WriteUserFont(0,0,ziku_data[0]);
  VFD_WriteUserFont(1,1,ziku_data[1]);
  VFD_WriteUserFont(2,2,ziku_data[2]);
  VFD_WriteUserFont(3,3,ziku_data[3]);
  VFD_WriteUserFont(4,4,ziku_data[4]);
  VFD_WriteUserFont(5,5,ziku_data[5]);
  delay(1000);
} // end of main loop
