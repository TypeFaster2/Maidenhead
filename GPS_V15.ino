/*************************************************************
 This sketch is used to get location from GPS then display
 the MaidenHead location
     VK3FMAL
*************************************************************/

/*
v3: Originally based on sketch from the TinyGPS site. 27 Oct 2012
v13: First full working version.                      11 Nov 2012
v14: Cleaned up working version with comments.        18 Nov 2012
v15: Add "0" instead of spaces in seconds fill         2 Dec 2012


>>> Remember to update version in the startup text <<<

Action list:
Add temperature reading using a one wire sensor.
Add battery voltage monitor.
Consider a switch to turn off backlight.
*/

   #define sprnt                      // Enable all the Serial Port code to be skipped
//   #define floats                   // Enable inclusion on floating point code.
                                      // Usually off. FP code is slow and not needed

//   #define Demo                     // Enables dummy locn. Allows testing with no GPS
                                      // The dummy sites need to be updated in the Locn and
                                      // Maidenhead routines.

// Libraries
#include <SoftwareSerial.h>           // Load the Serial library
#include <LiquidCrystal.h>            // Load the standard library
#include <TinyGPS.h>                  // Load the TinyGPS library

/*  Software serial multiple serial test
 
 Receives from the hardware serial, sends to software serial.
 Receives from software serial, sends to hardware serial.
 
 The circuit:
 * RX is digital pin 10 (connect to TX of other device)
 * TX is digital pin 11 (connect to RX of other device)
 
 Note:
 Not all pins on the Mega and Mega 2560 support change interrupts,
 so only the following can be used for RX:
 10, 11, 12, 13, 50, 51, 52, 53, 62, 63, 64, 65, 66, 67, 68, 69
 
 Not all pins on the Leonardo support change interrupts,
 so only the following can be used for RX:
 8, 9, 10, 11, 14 (MISO), 15 (SCK), 16 (MOSI).
 */

LiquidCrystal lcd(8, 7, 6, 5, 4, 3);     // Set up for the LCD.
#define BackLight  9                     // PWM pin for back light

#define RXPIN 10                         // GPS RX
#define TXPIN 11                         // GPS TX
unsigned long fix_age;
SoftwareSerial GPS(RXPIN, TXPIN);        //Set up pins for GPS comms

TinyGPS gps;
 void gpsdump(TinyGPS &gps);
bool feedgps();
void getGPS();
long lat, lon;
unsigned long CycleTimer = 0;

#ifdef floats
float LAT, LON;
#endif

/*  +++ These two variables need to be manually set for the current time and location +++
   Local_Offset of 10 adds 10 hours to GMT, valid for Australian Eastern Standard time.
   DST of 1 adds an extra hour in summer.  */
int DST          =  1;                // Daylight Savings Offset
int Local_Offset = 10;                // Hours local time is offset from GMT

// **********************************************************
void setup(){
GPS.begin(9600);                                 // Speed of GPS serial port
#ifdef sprnt
Serial.begin(115200);
Serial.println("Start");
#endif

  lcd.begin(16, 4);                              // Set size of LCD display
  analogWrite(BackLight, 80);                    // Set the backlight brightness

  lcd.setCursor(0,0);                            // set the preamble text
  //                         *                       *               * 
  //         012345678901234567890123456789012345678901234567890123456789012345678901
  lcd.print(" Amateur Radio    Decoder by            GPS Grid Square  VK3FMAL    V15 ");  
  #ifdef Demo
  lcd.setCursor(0,0);                            // set the preamble text
  lcd.print(" Amateur Radio    Decoder by            GPS Grid Square VK3FMAL DemoMode");  
  delay(5000);
  #endif
   
 unsigned long settle = millis();                // Wait to settle
  while ((settle + 5000) > millis()){
     feedgps();                                  // May as well start the GPS    
     getGPS();   }                               // Get data from the GPS
  
#ifdef sprnt  
  Serial.println("Run");
#endif  

  lcd.setCursor(0,0);                            // ensure screen is clear
  lcd.print("                                                                        "); 
}

// **********************************************************
void loop(){
  Serial.println("* .  *  .  *  .  *  .  *  .  *  .  *  .  *  .  *");

if(CycleTimer > millis()){                       // First cycle (Display 1)
  GPS_Locn(2);                                   // Latitude  
  GPS_Locn(0);                                   // Longitude
  GPS_Time(0);                                   // GMT = 0, Local = 1
  GPS_date(1);                                   // Date functions  
  #ifdef sprnt 
  DiagNum("First Cycle, Uptime : ",(millis() / 1000)); 
  #endif

   } else {                                      // Second cycle  (Display 2)
     if((CycleTimer + 10000) > millis()){        // This sets how long we show display 2
   GPS_Stats();                                  // Alt, HDOP, Sats 
   GPS_FixAge();  
     lcd.print("  ");                            // clean up left over characters 
   GPS_Time(1);         
     #ifdef sprnt 
      DiagNum("Second Cycle, Uptime : ",(millis() / 1000));
     #endif    
       
     } else { 
       CycleTimer = millis() + 10000;            // Reset timer. This really sets how
          #ifdef sprnt                           // long we show display 1
          DiagNum("Reset Cycle, Uptime : ",(millis() / 1000));
          #endif }
     }
                                  
 MaidenHead();

#ifdef sprnt           
 GPS_Speed();                                    // Speed wasn't used on display
#endif
 
#ifdef floats
// floater();
#endif
  
}
}         

// **********************************************************

// **********************************************************
void MaidenHead(){
/* 
Refer to:
http://en.wikipedia.org/wiki/Maidenhead_Locator_System
*/
  
feedgps();                                        
getGPS();                                        // Get data from the GPS 
  long lat, lon;                                 // GPS variables
  unsigned long fix_age;
gps.get_position(&lat, &lon, &fix_age);          // retrieves +/- lat/long in 100000ths of a degree  

#ifdef Demo
   /*  Set up the dummy location. Used for testing */
   lat = -3778886;                               //  S37 47.332 e144 57.916 
   lon = 14496526;                               //  S37.788867 E144.965267 
  
DiagNum("Lat is : ", lat);
DiagNum("Lon is : ", lon);
#endif

lon = lon + 18000000;                            // Step 1
lat = lat +  9000000;                            // Adjust so Locn AA is at the pole

char MH[6] = {'A', 'A', '0', '0', 'a', 'a'};     // Initialise our print string
  MH[0] +=  lon / 2000000;                       // Field
  MH[1] +=  lat / 1000000;
  MH[2] += (lon % 2000000) / 200000;             // Square
  MH[3] += (lat % 1000000) / 100000;
  MH[4] += (lon %  200000) /   8333;             // Subsquare .08333 is   5/60 
  MH[5] += (lat %  100000) /   4166;             //           .04166 is 2.5/60
  
String MH_txt = "";                              // Build up Maidenhead
int i = 0;                                       // into a string that's easy to print
while (i < 6){ 
  MH_txt += MH[i];
  i++; }

#ifdef sprnt
Serial.print("Maidenhead : ");
Serial.println(MH_txt);
#endif

lcd.setCursor(26, 0);                            // Where do we want this to display?
lcd.print(MH_txt);                               // corrected to be same as serial output (June 18)
 
  return;

}
// **********************************************************
void GPS_Time(int Time_Type){
  unsigned long fix_age, time, date, speed, course;   // variables for this routine

feedgps();  
  gps.get_datetime(&date, &time, &fix_age);           // time in hh:mm:ss, date in dd/mm/yy
  
  if(time > 23595959) {                               // Check we have a valid input
  //        1919221760
  #ifdef sprnt 
  DiagNum("Time to big. Time_Type is : ", Time_Type); // Too big, will give weird results
  Serial.print(time);  
  Serial.print("******* Time Error ************************************");
  #endif
  return;}

int hour   =  time / 1000000;                    // crack the time into units
int minute = (time / 10000) % 100;
int second = (time / 100) % 100;

feedgps();
 
  String Tme = "";
  if(Time_Type == 1){                             // 1 = local time, 0 = GMT
    hour = hour + DST + Local_Offset;             // Get Local time
if (hour > 24){                                   // Ensure time is valid
  hour = hour - 24;  
  }}

  if(hour < 10){Tme = "0";}                       // print out the time (GMT)
  Tme += hour;
  if(minute  > 9){Tme += ":";}
    else {Tme += ":0";}
  Tme += minute;
  if(second  > 9){Tme += ":";}
    else {Tme += ":0";}
  Tme += second;
 
  lcd.setCursor(16, 1);                           // Go to row 4 of LCD

 if(Time_Type == 1){lcd.print("LclTime ");
#ifdef sprnt   
         DiagTxt("Lcl Time is: ", Tme);          
#endif
 } else {
   lcd.setCursor(16, 0);
   lcd.print("GMT Time ");
   
   lcd.setCursor(24, 1);
#ifdef sprnt      
   DiagTxt("GMT Time is : ", Tme);
#endif
         }
  lcd.print(Tme); 
  
  return;
  
}

// **********************************************************
void GPS_date(int Date_Type){

  unsigned long fix_age, time, date, speed, course; // variables for this routine

feedgps();  
  gps.get_datetime(&date, &time, &fix_age);         // time in hh:mm:ss, date in dd/mm/yy

  if(date > 311299) {                               // Too big, will give weird results
  Serial.println("******* Date Error ************************************");
  return ;}

int year   = date % 100;                            // crack date into units
int month  = (date / 100) % 100;
int day    =  date / 10000;

feedgps();

String Date_DMY = "";                               // Build out the date (GMT) 
  if(day < 9){Date_DMY = "0";}                      // Day / Month / Year
  Date_DMY += day;                                  // Build into a string, which is 
  if(month < 9){Date_DMY += "/0";}                  // faster to print out
    else {Date_DMY += "/";}                         // easier too
  Date_DMY += month;
  
  if(Date_Type == 0){                               // Make sure the leading zeros are right
  Date_DMY += "/";
  Date_DMY += year; }  
  Date_DMY += "  ";
  
  lcd.setCursor(16, 1);                             // where should it print?
  lcd.print(Date_DMY);

#ifdef sprnt
   DiagTxt("Date DM     : ", Date_DMY);
#endif
  
 return ;  
}

// **********************************************************
void GPS_Locn(int LatLon){

feedgps();                                        
getGPS();                                         // Get data from the GPS 
 // Home should be something like S37° 39.345' E143° 52.701'   
  
long lat, lon , LLDec;                            // GPS variables
gps.get_position(&lat, &lon, &fix_age);           // retrieves +/- lat/long in 100000ths of a degree
 
#ifdef Demo
DiagNum("Lat is : ", lat);                        // Demo mode for testing
DiagNum("Lon is : ", lon);

   lat = -3778886;                                //  S37 47.332 e144 57.916
   lon = 14496526;                                //  S37.788867 E144.965267    
#endif 
     
     String DMS = " GPS_Locn not run ";           // Reset DMS string 
/* 
1. The whole units of degrees will remain the same (i.e. in 121.135° longitude, 
    start with 121°).
2. Multiply the decimal by 60 (i.e. .135 * 60 = 8.1).
3. The whole number becomes the minutes (8').
4. Take the remaining decimal and multiply by 60. (i.e. .1 * 60 = 6).
5. The resulting number becomes the seconds (6"). Seconds can remain as a decimal.
6. Take your three sets of numbers and put them together, using the symbols for degrees (°), 
    minutes (‘), and seconds (") (i.e. 121°8'6" longitude)
*/
feedgps();
// Lon = 0 (EW) Lat = 2 (NS)

 char Heading[] = "EWNS" ;                             // Heading letter assignment
 int H = LatLon;                                       // Heading pointer
if(LatLon == 2){LLDec = lat;                           // Working with Lat            
     } else {   LLDec = lon; }                         // Working with Lon

     if(LLDec > 36000000 || LLDec < -36000000){        // Check we have a valid output
#ifdef  sprnt
  DiagNum("******* Locn Error ************************************", 0);
  DiagNum("LLDec is : ", LLDec);
#endif
  return;}    
               
if (LLDec < 0){ H = H + 1;}                             // Step 1. Are we N,S,E or W.?
   char HeadingLetter = Heading[H];                     // W or S

if (LLDec < 0)                                          // Ensure we're working with a positive number
       { LLDec = LLDec * -1;} 
                    
long Deg = (LLDec / 100000);                            // get the Degrees
long Min_i = (LLDec - (Deg * 100000)) * 60  ;           // Work out the remainder. Intermediate step
long Min = Min_i / 100000;                              // finalise minutes
long Sec = ((Min_i - (Min * 100000)) /100);             // Seconds

feedgps();                                              // This is a slow routine, if we don't
                                                        // feed the GPS we'll get checksum errors

   DMS = "";                                            // Build up a string. Printing one string is                 
   DMS += HeadingLetter;                                // faster than multiple LCD writes.
   if(Deg > 99){DMS += Deg;} else                       // Get spacing in front of Deg right
   {if(Deg > 9){DMS += " "; DMS += Deg;}
   else {DMS += "   "; DMS += Deg;}}
   
   if(Min > 9){DMS += " "; DMS += Min; }                // Get spacing on minutes right
   else {DMS += "  "; DMS += Min;}

   DMS += ".";                                          // Add decimal point

   if(Sec > 99){     DMS += Sec;}                       // Get spacing on Seconds right
   else{ if(Sec > 9){DMS += "0"; DMS += Sec;}
   else{DMS += "00"; DMS += Sec;}}
   
   DMS += "     ";                                      // Clean up any old characters left on display

   if(LatLon == 2){                                     // What is we just worked out? (Lat or Lon)
#ifdef sprnt     
   DiagTxt("Latitude    : ", DMS);  
#endif
   lcd.setCursor(0, 0);                                 // get screen locn right
   
   } else {
#ifdef sprnt     
   DiagTxt("Longitude  : ", DMS);  
#endif   
   lcd.setCursor(0, 1);}                                // get screen locn right  
    
   lcd.print(DMS);                                      // Print result to screen 

   return;

   }

// **********************************************************
void GPS_Stats(){
  unsigned short sentences, failed_checksum;
  unsigned long chars;
  gps.stats(&chars, &sentences, &failed_checksum);
feedgps();
  
  long alt = gps.altitude() / 100; // +/- altitude in meters
  int unsigned Sats = gps.satellites() ;
  long unsigned HDOP = gps.hdop() ;
  int ver = gps.library_version();
  
  String text = "Alt ";                                    // Altitude
  if (alt > 9999){text += "9999";}                         // Make sure we don't really long data
   else {if(alt >999){text += alt;}                        // Would apply before a good reading
   else {if(alt > 99){text += " "  ; text += alt;}
   else {if(alt >  9){text += "  " ; text += alt;} 
   else              {text += "   "; text += alt;}}}}
  text += " Sats ";                                        // Sats in view
  if (Sats > 9){text += Sats;}                             
   else {text += " "; text += Sats;}       
  lcd.setCursor(0, 0); lcd.print(text) ;                   // Alt & Sats in one string
  
  text = "HDOP ";                                          // HDOP 
  if (HDOP > 999){text += "999";}
   else {if(HDOP > 99){text += HDOP;}
   else {if(HDOP >  9){text += " " ; text += HDOP;} 
   else              {text += "  "; text += HDOP;}}}//}
  text += " FCS ";                                         // Failed Checksum
  if (failed_checksum > 999){text += "999";}
   else {if(failed_checksum > 99){text += failed_checksum;}
   else {if(failed_checksum >  9){text += " " ; text += failed_checksum;} 
   else              {text += "  "; text += failed_checksum;}}}
  lcd.setCursor(0, 1); lcd.print(text) ;                   // HDOP & Checksum in one string

#ifdef sprnt                                               // Serial print lots of stuff
  Serial.print("Sentences  : ");        Serial.println(sentences);
  Serial.print("Failed_checksum : ");   Serial.println(failed_checksum);
  Serial.print("Chars      : ");        Serial.println(chars);
  Serial.print("Satellites : ");        Serial.println(Sats);
  Serial.print("Horizontal Dilution of Precision : "); Serial.println(HDOP);  
  Serial.print("Altitude   : ");        Serial.println(alt);  
  Serial.print("TinyGPS Ver: ");        Serial.println(ver);
  feedgps();
#endif  
}  

// **********************************************************
void GPS_Speed(){                     // This never made it to the LCD
long lat, lon;
unsigned long speed, course;

feedgps();
// returns speed in 100ths of a knot
speed = gps.speed();
#ifdef sprnt
DiagNum("Speed      : ", speed);
#endif
feedgps();
// course in 100ths of a degree
course = gps.course();
#ifdef sprnt
DiagNum("Course     : ", course);
#endif

}

// **********************************************************
void GPS_FixAge(){
// float flat, flon;   // returns +- latitude/longitude in degrees
 long lat, lon;        // Gets Lat and Lon without loading floating point libraries
feedgps();
unsigned long fix_age; 

gps.get_position(&lat, &lon, &fix_age);

#ifdef sprnt
if (fix_age == TinyGPS::GPS_INVALID_AGE)
  Serial.println("No fix detected");
else if (fix_age > 5000)
  Serial.println("Warning: possible stale data!");
else
  Serial.print("Data is current. Fix Age: ");Serial.println(fix_age);
#endif  

if (fix_age > 600000){fix_age = 0000;}      // Try to stop the invalid fix age overflowing LCD

String text = "Age ";                       // Leading text to the string
  if(fix_age > 9999){text += "9999";}       // Keep the 1's unit in the same place on the LCD
    else {if (fix_age > 999){text += fix_age;}
    else {if (fix_age >  99){text += " " ; text += fix_age;}
    else {if (fix_age >   9){text += "  "; text += fix_age;}}}}
    
     lcd.setCursor(16, 0);
     lcd.print(text);    

return;

}

// **********************************************************
void getGPS(){                            // This code is from the TinGPS example
bool newdata = false;
unsigned long start = millis();           // Every 1 seconds we print an update

while (millis() - start < 1000)
{
if (feedgps ()){
newdata = true;
}
}
if (newdata)
{
 gpsdump(gps);
}
}
bool feedgps(){
while (GPS.available())
{
if (gps.encode(GPS.read()))
return true;
}
return 0;
}

// **********************************************************
void gpsdump(TinyGPS &gps) {              //byte month, day, hour, minute, second, hundredths;
gps.get_position(&lat, &lon);

#ifdef floats
LAT = lat;
LON = lon;
#endif
{
feedgps();                                // If we don't feed the gps during this long routine, 
}                                         // we may drop characters and get checksum errors
}


// **********************************************************
void DiagTxt(String label, String text){              // function to print to Serial Port
    Serial.print  (label);                            // Print out the text info, usually the variable
    Serial.println(text);                             // Print out the variable value
} 

// **********************************************************
void DiagNum(String label, long num){                 // function to print to Serial Port
    Serial.print  (label);                            // Print out the text info, usually the variable
    Serial.println(num);                              // Print out the variable value
} 
 
// **********************************************************
// Floating point code. All got dropped
/*void floater(){

float flat, flon;

// returns +/- latitude/longitude in degrees
gps.f_get_position(&flat, &flon, &fix_age);
float falt = gps.f_altitude(); // +/- altitude in meters
/*float fc = gps.f_course(); // course in degrees
float fk = gps.f_speed_knots(); // speed in knots
float fmph = gps.f_speed_mph(); // speed in miles/hr
float fmps = gps.f_speed_mps(); // speed in m/sec
float fkmph = gps.f_speed_kmph(); // speed in km/hr
*/ 
/*
feedgps();
#ifdef sprnt
Serial.print(" Flat: ");Serial.print(flat);
Serial.print(" Flon: ");Serial.print(flon);
Serial.print(" Falt: ");Serial.println(falt);
#endif

long lat, lon;

// returns +/- latitude/longitude in degrees
gps.f_get_position(&flat, &flon, &fix_age);
long alt = gps.altitude(); // +/- altitude in meters
long crse = gps.course(); // course in degrees
//long knot = gps.speed_knots(); // speed in knots
//long mph = gps.speed_mph(); // speed in miles/hr
//long mps = gps.speed_mps(); // speed in m/sec
//long kmph = gps.speed_kmph(); // speed in km/hr

feedgps();
#ifdef sprnt
Serial.print(" Flat: ");Serial.print(flat);
Serial.print(" Flon: ");Serial.print(flon);
Serial.print(" Falt: ");Serial.println(falt);
#endif


}*/
