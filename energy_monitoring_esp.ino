//#include <ThingSpeak.h>
//#include <ThingspeakClient.h>
#include <Blynk.h>
#include <ESP8266WebServer.h>
#include "Pzem004t_V3.h"
#include "icon.h"
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

#include <time.h>
#include <Ticker.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h> // ver 5.1.3
#include "ST7565_homephone_esp8266.h"

#define CS1 D3
#define CS2 D4
#define BUTTON D1
//Pzem004t_V3 pzem(Tx1, Rx1);
Pzem004t_V3 pzem(&Serial);

BlynkTimer timer;
///Wifi
void sendBlynkSensor();
char auth[] = "bz85ORZ6EwzN7dSThGuwr-o7NqJUbt4T"; // Blynk Token
//const char *ssid = "End Game";                     // SSID WiFi
//const char *password = "1qaz2wsx"; 
const char *ssid = "GHTK T7";                     // SSID 
const char *password = "vanphongtang7";           // 

// Location to check weather 

const String latitude = "20.868763"; // Lôi Khê
const String longitude = "106.225604";

const String key = "ca66835ddbaa496c9d11aee5f48fd28e"; // auth key

// clock center point
const int clockCenterX = 32;
const int clockCenterY = 32;

String dayOfWeek, thang, ngay, gio, phut, giay, nam;
int hour, minute, sec;
int temp, humi, weather_code;
int solar_radiation = 0;
/* |---------|---------|---------------|
 * |   LCD   | ESP8266 | Arduino mapping |
 * |---------|---------|---------------|
 * |  RST 32 |    D0   |     GPIO16   32 |
 * |  SCLK 22|    D5   |     GPIO14   22|
 * |  A0   31|    D6   |     GPIO12   31 |
 * |  SID  21|    D7   |     GPIO13   21 |
 * |---------|---------|---------------|
 */
ST7565 lcd(16, 14, 12, 13);

Ticker flip;
HTTPClient http;
int button_count = 1;
String StrTrangThai = "";
float solar_wH = 0.0;
float onGrid_wH = 0.0;
float load_wH = 0.0;

String IPaddress;
float voltage = 0.0;
float solar_Amp = 0.0;
float load_Amp = 0.0;
float grid_Amp = 0.0;
float power_load = 0.0;
float power_factor = 0.95;
float freq = 50.0;
float power_solar = 0.0;
float power_onGrid = 0.0;

void setup()
{
  pinMode(CS1, OUTPUT);
  pinMode(CS2, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(BUTTON), buttonCount, RISING);
  Serial.begin(115200);

  lcd.ON();
  lcd.SET(20, 0, 0, 0, 4);
  display_wellcome();

  WiFi.begin(ssid, password);
  Blynk.begin(auth, ssid, password);

  pzem.begin();
  pzem.setTimeout(100);
  timer.setInterval(1000, sendBlynkSensor);

  configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  updateTime();
  flip.attach(1, interrupTimer);                   // 
  currentWeather(latitude, longitude, key); // 

  delay(1000);
}

void loop()
{
  if (nam.toInt() < 2000) 
  { 
    updateTime();
  }
  if ( minute == 0 || minute == 30 )
  { 
  
    currentWeather(latitude, longitude, key);
  }
  
  if ((button_count % 2) == 1)
  {
    lcd.Clear();
    display_weather();
  }
  else
  {
    lcd.Clear();
    display_energy();
  }
  //Blynk.run();
  //timer.run();
}

ICACHE_RAM_ATTR void buttonCount()
{

  button_count++;
  if (button_count == 3)
    button_count = 1;
  Serial.println("Interrupt Detected" + button_count);
}

void getDataPZEM()
{
  // readMillis = millis();
  if (ngay == "01" && gio == "00" && phut == "00" && giay == "00")
  { ///reset data vào đầu tháng
    pzem.resetEnergy();
  }
  digitalWrite(CS1, LOW);
  digitalWrite(CS2, HIGH);
  delay(50);
  pzem_info pzemData1 = pzem.getData();

  digitalWrite(CS1, HIGH);
  digitalWrite(CS2, LOW);
  delay(50);
  pzem_info pzemData2 = pzem.getData();


  voltage = pzemData1.volt;
  power_factor = pzemData2.powerFactor;
  freq = pzemData1.freq;

  load_Amp = pzemData1.ampe;
  load_wH = pzemData1.energy;
  power_load = pzemData1.power;

  solar_Amp = pzemData2.ampe;
  solar_wH = pzemData2.energy;
  power_solar = pzemData2.power;
  
  IPaddress = WiFi.localIP().toString();
  //voltage = random(215, 235) / 0.99;
  // solar_Amp = random(100, 300) / 135.5;                    
  // load_Amp = random(100, 900) / 111.1;
  // grid_Amp = load_Amp - solar_Amp;
  // power_load = voltage * load_Amp;
  // power_solar = voltage * solar_Amp;
  power_onGrid = power_load - power_solar;
  //solar_wH += power_solar / 3600;
  //onGrid_wH += power_onGrid / 3600;
  onGrid_wH = load_wH - solar_wH;
 // load_wH += power_load / 3600;
}
void sendBlynkSensor()
{
  getDataPZEM();
  Blynk.virtualWrite(V0, String(solar_Amp, 2) + "A - " + String(power_solar, 1) + "W - " + String(solar_wH, 1) + "Wh");
  Blynk.virtualWrite(V1, String(load_Amp, 2) + "A - " + String(power_load, 1) + "W - " + String(load_wH, 1) + "Wh");
  Blynk.virtualWrite(V2, power_solar);
  Blynk.virtualWrite(V3, solar_wH);
  Blynk.virtualWrite(V4, power_load);
  Blynk.virtualWrite(V5, power_onGrid);
  Blynk.virtualWrite(V6, voltage);
  if (power_onGrid < 0)
  {
    StrTrangThai = "Đang đẩy lưới EVN " + String(power_onGrid) + " w";
    Blynk.setProperty(V5, "color", "#ED9D00"); // Vàng
    Blynk.setProperty(V5, "label", "Đang đẩy lưới EVN");
  }
  else
  {
    StrTrangThai = "Đang dùng lưới EVN " + String(power_onGrid) + " w";
    Blynk.setProperty(V5, "color", "#D3435C"); // Đỏ
    Blynk.setProperty(V5, "label", "Đang dùng lưới EVN");
  }

  Serial.println();
  Serial.println("Solar: " + String(power_solar));
  Serial.println("Tải: " + String(power_load));
  Serial.println("Trạng Thái: " + String(StrTrangThai));
  Serial.println();
}
void display_wellcome()
{
  lcd.Clear();
  lcd.Bitmap(14, 0, 100, 43, logo, BLACK);
  writeString(32, 45, "SOLAR SYSTEM");
  writeString(30, 55, "by Nhan.pro41");
  lcd.display();
}
void display_energy()
{

  lcd.Bitmap(0, 0, 30, 30, solar, BLACK);
  lcd.Bitmap(100, 0, 30, 30, grid, BLACK);
  lcd.Bitmap(50, 0, 30, 30, home_icon, BLACK);
  lcd.Bitmap(30, 16, 20, 9, arrow_right, BLACK);
  lcd.Bitmap(80, 15, 20, 9, arrow_left, BLACK);

  writeString(28, 0, String(solar_radiation));
  
  writeString(24, 32, "W");
  writeString(75, 32, "W");
  writeString(120, 32, "W");

  writeString(24, 42, "A");
  writeString(75, 42, "A");
  writeString(120, 42, "A");

  writeString(6, 32, String(power_solar, 0));
  writeString(51, 32, String(power_load, 0));
  writeString(93, 32, String(power_onGrid, 0));

  writeString(6, 42, String(solar_Amp, 1));
  writeString(51, 42, String(load_Amp, 2));
  writeString(93, 42, String(grid_Amp, 2));

  if (solar_wH < 1000)
  {
    writeString(6, 52, String(solar_wH, 0));
    writeString(24, 52, "Wh");
  }
  else
  {
    float solar_kWh = solar_wH / 1000;
    writeString(0, 52, String(solar_kWh, 2));
    writeString(18, 52, "kWh");
  }

  if (load_wH < 1000)
  {
    writeString(51, 52, String(load_wH, 0));
    writeString(75, 52, "Wh");
  }
  else
  {
    float load_kWh = load_wH / 1000;
    writeString(51, 52, String(load_kWh, 2));
    writeString(69, 52, "kWh");
  }

  if (onGrid_wH < 1000)
  {
    writeString(93, 52, String(onGrid_wH, 0));
    writeString(116, 52, "Wh");
  }
  else
  {

    float onGrid_kWh = onGrid_wH / 1000;
    writeString(91, 52, String(onGrid_kWh, 2));
    writeString(109, 52, "kWh");
  }

  lcd.display();
}

void display_weather()
{
  
  lcd.clear();

  lcd.Circle(clockCenterX, clockCenterY, 28, 1); // Vẽ vòng ngoài của đồng hồ
  lcd.Circle(clockCenterX, clockCenterY, 29, 1);
  for (int i = 0; i < 3; i++)
  { // Vẽ Tâm đồng hồ
    lcd.Circle(clockCenterX, clockCenterY, i, 1);
  }

  for (int i = 0; i < 12; i++)
  { // Vẽ 12 vị trí giờ tương ứng từ 1 đến 12
    drawMark(i);
  }
  displayCurrentWeather(62, 36, String(temp), String(humi));
  iconWeather(103, 36, weather_code);
  writeString(55, 0, String(voltage, 1) + "V");
  writeString(100, 0, String(power_factor, 2));
  writeString(75, 16, dayOfWeek);                      // Hiển thị ngày trong tuần
  writeString(67, 27, ngay + "/" + thang + "/" + nam); // Hiển thị ngày, tháng, năm
  drawHour(hour, minute);                          // Vẽ kim giờ
  drawMin(minute);                                 // Vẽ kim phút
  drawSec(sec);                                    // Vẽ kim giây

  lcd.display();
}

void interrupTimer()
{
  // Đếm thời gian để không phải cập nhật thời gian liên tục
  sec++;
  if (sec == 60)
  {
    minute++;
    sec = 0;
  }
  if (minute == 60)
  {
    hour++;
    minute = 0;
  }
  if (hour == 24)
  { // Sau 1 ngày cập nhật thời gian 1 lần
    hour = 0;
    updateTime();
  }

}

void updateTime()
{
  time_t now = time(nullptr);
  String data = ctime(&now);
  //  Serial.println(data);
  
  String ngayTrongTuan = data.substring(0, 3);
  String month = data.substring(4, 7);
  ngay = data.substring(8, 10);
  gio = data.substring(11, 13);
  phut = data.substring(14, 16);
  giay = data.substring(17, 19);
  nam = data.substring(20, 24);
  //Biến đổi
  ngayTrongTuan.toUpperCase();
  ngay.trim();
  if (ngayTrongTuan == "MON")
    dayOfWeek = "MONDAY";
  else if (ngayTrongTuan == "TUE")
    dayOfWeek = "TUESDAY";
  else if (ngayTrongTuan == "WED")
    dayOfWeek = "WEDNESDAY";
  else if (ngayTrongTuan == "THU")
    dayOfWeek = "THURSDAY";
  else if (ngayTrongTuan == "FRI")
    dayOfWeek = "FRIDAY";
  else if (ngayTrongTuan == "SAT")
    dayOfWeek = "SATURDAY";
  else if (ngayTrongTuan == "SUN")
    dayOfWeek = "SUNDAY";
  if (ngay.toInt() < 10)
  {
    ngay = "0" + ngay;
  }
  if (month == "Jan")
    thang = "01";
  else if (month == "Feb")
    thang = "02";
  else if (month == "Mar")
    thang = "03";
  else if (month == "Apr")
    thang = "04";
  else if (month == "May")
    thang = "05";
  else if (month == "Jun")
    thang = "06";
  else if (month == "Jul")
    thang = "07";
  else if (month == "Aug")
    thang = "08";
  else if (month == "Sep")
    thang = "09";
  else if (month == "Oct")
    thang = "10";
  else if (month == "Nov")
    thang = "11";
  else if (month == "Dec")
    thang = "12";
  hour = gio.toInt();
  minute = phut.toInt();
  sec = giay.toInt();
}

void writeString(int x, int y, String chuoi)
{ // 
  lcd.Asc_String(x, y, (char *)chuoi.c_str(), BLACK);
}

void drawMark(int h)
{
  float x1, y1, x2, y2;

  h = h * 30;
  h = h + 270;

  x1 = 27 * cos(h * 0.0175);
  y1 = 27 * sin(h * 0.0175);
  x2 = 24 * cos(h * 0.0175);
  y2 = 24 * sin(h * 0.0175);

  lcd.DrawLine(x1 + 32, y1 + 32, x2 + 32, y2 + 32, 1);
}

void drawSec(int s)
{
  float x1, y1, x2, y2;

  s = s * 6;
  s = s + 270;

  x1 = 27 * cos(s * 0.0175);
  y1 = 27 * sin(s * 0.0175);
  x2 = 24 * cos(s * 0.0175);
  y2 = 24 * sin(s * 0.0175);

  lcd.DrawLine(clockCenterX, clockCenterY, x2 + clockCenterX, y2 + clockCenterY, 1);
}

void drawMin(int m)
{
  float x1, y1, x2, y2, x3, y3, x4, y4;

  m = m * 6;
  m = m + 270;

  x1 = 25 * cos(m * 0.0175);
  y1 = 25 * sin(m * 0.0175);
  x2 = 3 * cos(m * 0.0175);
  y2 = 3 * sin(m * 0.0175);
  x3 = 10 * cos((m + 8) * 0.0175);
  y3 = 10 * sin((m + 8) * 0.0175);
  x4 = 10 * cos((m - 8) * 0.0175);
  y4 = 10 * sin((m - 8) * 0.0175);

  lcd.DrawLine(x1 + clockCenterX, y1 + clockCenterY, x3 + clockCenterX, y3 + clockCenterY, 1);
  lcd.DrawLine(x3 + clockCenterX, y3 + clockCenterY, x2 + clockCenterX, y2 + clockCenterY, 1);
  lcd.DrawLine(x2 + clockCenterX, y2 + clockCenterY, x4 + clockCenterX, y4 + clockCenterY, 1);
  lcd.DrawLine(x4 + clockCenterX, y4 + clockCenterY, x1 + clockCenterX, y1 + clockCenterY, 1);
}

void drawHour(int h, int m)
{
  float x1, y1, x2, y2, x3, y3, x4, y4;

  h = (h * 30) + (m / 2);
  h = h + 270;

  x1 = 20 * cos(h * 0.0175);
  y1 = 20 * sin(h * 0.0175);
  x2 = 3 * cos(h * 0.0175);
  y2 = 3 * sin(h * 0.0175);
  x3 = 8 * cos((h + 12) * 0.0175);
  y3 = 8 * sin((h + 12) * 0.0175);
  x4 = 8 * cos((h - 12) * 0.0175);
  y4 = 8 * sin((h - 12) * 0.0175);

  lcd.DrawLine(x1 + clockCenterX, y1 + clockCenterY, x3 + clockCenterX, y3 + clockCenterY, 1);
  lcd.DrawLine(x3 + clockCenterX, y3 + clockCenterY, x2 + clockCenterX, y2 + clockCenterY, 1);
  lcd.DrawLine(x2 + clockCenterX, y2 + clockCenterY, x4 + clockCenterX, y4 + clockCenterY, 1);
  lcd.DrawLine(x4 + clockCenterX, y4 + clockCenterY, x1 + clockCenterX, y1 + clockCenterY, 1);
}

void currentWeather(String latitude, String longitude, String key)
{
  http.begin("http://api.weatherbit.io/v2.0/current?&lat=" + latitude + "&lon=" + longitude + "&key=" + key);
  if (http.GET() == HTTP_CODE_OK)
  {
    String data = http.getString();
    //    Serial.println(data);
    DynamicJsonDocument jsonBuffer(1200);
    DeserializationError error = deserializeJson(jsonBuffer, (char *)data.c_str());
    temp = jsonBuffer["data"][0]["temp"].as<int>();
    humi = jsonBuffer["data"][0]["rh"].as<int>();
    weather_code = jsonBuffer["data"][0]["weather"]["code"].as<int>();
    solar_radiation = jsonBuffer["data"][0]["solar_rad"].as<int>();
  }
  http.end();
}

void displayCurrentWeather(int x, int y, String nhietdo, String doam)
{ //
  lcd.Bitmap(x, y, 12, 12, temperature, BLACK);
  lcd.Bitmap(x, y + 14, 12, 12, humidity, BLACK);
  if (nhietdo.toInt() < 10)
  { //
    writeString(x + 14, y + 2, nhietdo);
    lcd.Bitmap(x + 20, y + 2, 5, 7, tdo, BLACK);
    lcd.Asc_Char(x + 26, y + 2, 'C', BLACK);
  }
  else
  {
    writeString(x + 14, y + 2, nhietdo);
    lcd.Bitmap(x + 26, y + 2, 5, 7, tdo, BLACK);
    lcd.Asc_Char(x + 32, y + 2, 'C', BLACK);
  }
  writeString(x + 14, y + 16, doam + "%"); // Độ ẩm
}

void iconWeather(int x, int y, int code)
{ //https://www.weatherbit.io/api/codes  
  switch (code)
  {
  case 202:
    lcd.Bitmap(x, y, 24, 24, lighting_rain, BLACK);
    break;
  case 233:
    lcd.Bitmap(x, y, 24, 24, lighting, BLACK);
    break;
  case 623:
    lcd.Bitmap(x, y, 24, 24, rain, BLACK);
    break;
  case 800:
    if (hour < 18 && hour > 5)
      lcd.Bitmap(x, y, 24, 24, clear_sky, BLACK);
    else
      lcd.Bitmap(x, y, 24, 24, night, BLACK);
    break;
  case 803:
    if (hour < 18 && hour > 5)
      lcd.Bitmap(x, y, 24, 24, clear_cloudy, BLACK);
    else
      lcd.Bitmap(x, y, 24, 24, night_cloud, BLACK);
    break;
  case 804:
    lcd.Bitmap(x, y, 24, 24, cloudy, BLACK);
    break;
  }
}
