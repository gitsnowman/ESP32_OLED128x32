#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ========== НАСТРОЙКИ OLED ==========
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
U8G2_FOR_ADAFRUIT_GFX u8g2;

// ========== НАСТРОЙКИ СЕТИ ==========
#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""
#define WIFI_CHANNEL 6

#define TIME_ZONE_HOURS 3
#define NTP_SERVER "pool.ntp.org"
#define WEATHER_UPDATE_INTERVAL 300000 // 5 минут

// ДОБАВИТЬ ТАЙМАУТ для WiFi
#define WIFI_TIMEOUT 15000 // 15 секунд таймаут

// ========== ДОПОЛНИТЕЛЬНЫЕ ПЕРЕМЕННЫЕ ==========
bool wifiConnected = false;
unsigned long wifiStartTime = 0;

// Для Санкт-Петербурга:
#define LATITUDE 59.9343
#define LONGITUDE 30.3351
// ==============================

#define UTC_OFFSET (TIME_ZONE_HOURS * 3600)
#define UTC_OFFSET_DST 0

float currentTemp = 0;
float currentHumidity = 0;
String weatherCondition = "Unknown";
String cityName = "St.Petersburg";
unsigned long lastWeatherUpdate = 0;
bool weatherError = false;
int displayMode = 0;
unsigned long lastDisplayChange = 0;
#define DISPLAY_CHANGE_INTERVAL 5000 // 5 секунд

void spinner() {
  static int8_t counter = 0;
  const char* glyphs = "|/-\\";
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setCursor(120, 30);
  u8g2.print(glyphs[counter++]);
  if (counter == 4) {
    counter = 0;
  }
}

// Функция для преобразования кода погоды в текст (из рабочего кода)
String getWeatherCondition(int weatherCode) {
  // Упрощенная расшифровка кодов погоды WMO
  if (weatherCode == 0) return "Clear sky";
  if (weatherCode == 1) return "Mainly clear";
  if (weatherCode == 2) return "Partly cloudy";
  if (weatherCode == 3) return "Cloudy";
  if (weatherCode >= 45 && weatherCode <= 48) return "Fog";
  if (weatherCode >= 51 && weatherCode <= 57) return "Drizzle";
  if (weatherCode >= 61 && weatherCode <= 67) return "Rain";
  if (weatherCode >= 71 && weatherCode <= 77) return "Snow";
  if (weatherCode >= 80 && weatherCode <= 82) return "Rain showrs";
  if (weatherCode >= 85 && weatherCode <= 86) return "Snow showrs";
  if (weatherCode >= 95 && weatherCode <= 99) return "Storm";
  return "Unknown";
}

// Получение данных о погоде с OpenMeteo (из рабочего кода)
void updateWeather() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    // URL для OpenMeteo API
    String url = "https://api.open-meteo.com/v1/forecast?";
    url += "latitude=" + String(LATITUDE, 6);
    url += "&longitude=" + String(LONGITUDE, 6);
    url += "&current=temperature_2m,relative_humidity_2m,weather_code";
    url += "&timezone=auto";
    url += "&forecast_days=1";
    
    Serial.println("Fetching weather from: " + url);
    Serial.println("City: " + cityName);
    
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode == 200) {
      String payload = http.getString();
      Serial.println("Response: " + payload);
      
      DynamicJsonDocument doc(2048);
      DeserializationError error = deserializeJson(doc, payload);
      
      if (!error) {
        // Получаем текущие данные
        currentTemp = doc["current"]["temperature_2m"];
        currentHumidity = doc["current"]["relative_humidity_2m"];
        int weatherCode = doc["current"]["weather_code"];
        
        // Преобразуем код погоды в текст
        weatherCondition = getWeatherCondition(weatherCode);
        
        weatherError = false;
        
        Serial.printf("City: %s\n", cityName.c_str());
        Serial.printf("Temperature: %.1f°C\n", currentTemp);
        Serial.printf("Humidity: %.0f%%\n", currentHumidity);
        Serial.printf("Weather: %s (code: %d)\n", weatherCondition.c_str(), weatherCode);
        
      } else {
        Serial.println("JSON parse error");
        weatherError = true;
      }
    } else {
      Serial.printf("HTTP error: %d\n", httpCode);
      weatherError = true;
    }
    
    http.end();
  } else {
    Serial.println("WiFi not connected");
    weatherError = true;
  }
}

void displayScreen1() {
  // Режим 1: Время, температура и влажность КРУПНО
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    char timeStr[9];
    char dateStr[11];
    
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
    strftime(dateStr, sizeof(dateStr), "%d/%m/%Y", &timeinfo);
    
    // Время - средний шрифт
    u8g2.setFont(u8g2_font_10x20_tf);
    u8g2.setCursor(0, 16);
    u8g2.print(timeStr);
    
    // ТЕМПЕРАТУРА - ОЧЕНЬ КРУПНО
    u8g2.setFont(u8g2_font_10x20_tf);
    u8g2.setCursor(90, 16);
    u8g2.print(currentTemp, 0);
    u8g2.print("C");
    
    // Дата мелким шрифтом
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setCursor(0, 31);
    u8g2.print(dateStr);
    
    // ВЛАЖНОСТЬ под температурой
    u8g2.setCursor(90, 31);
    u8g2.print(currentHumidity, 0);
    u8g2.print("%");
    
    // Индикатор обновления данных
    if (millis() - lastWeatherUpdate < 5000) {
      u8g2.setCursor(120, 28);
      u8g2.print("*");
    }
    
    // Статус ошибки
    if (weatherError) {
      u8g2.setCursor(110, 8);
      u8g2.print("ERR");
    }
  }
}

void displayScreen2() {
  // 1. Заголовок "WEATHER"
  u8g2.setFont(u8g2_font_t0_13_tf); // ← ИСПРАВЛЕНО шрифт
  String header = "WEATHER";
  int headerWidth = header.length() * 8; // ← ИСПРАВЛЕНО ширина
  int headerX = (128 - headerWidth) / 2;
  u8g2.setCursor(headerX, 10);
  u8g2.print(header);
  
  // 2. Температура и влажность
  u8g2.setFont(u8g2_font_t0_16_tf); // ← ИСПРАВЛЕНО шрифт
  u8g2.setCursor(15, 21);
  u8g2.print(currentTemp, 1);
  u8g2.print("C");
  u8g2.setCursor(75, 21);
  u8g2.print(currentHumidity, 0);
  u8g2.print("%");
  
  // 3. Разделительная линия - РАСКОММЕНТИРОВАТЬ!
  // display.drawLine(10, 28, 118, 28, SSD1306_WHITE); // ← РАБОЧАЯ ЛИНИЯ
  
  // 4. Состояние погоды - МАКСИМАЛЬНО ВНИЗ
  u8g2.setFont(u8g2_font_t0_13_tf); // ← ИСПРАВЛЕНО шрифт
  String displayCondition = weatherCondition;
  if (displayCondition.length() > 12) {
    displayCondition = displayCondition.substring(0, 12);
  }
  int conditionWidth = displayCondition.length() * 8; // ← ИСПРАВЛЕНО ширина
  int conditionX = (128 - conditionWidth) / 2;
  u8g2.setCursor(conditionX, 31); // ← ИСПРАВЛЕНО: Y=31 (максимум!)
  u8g2.print(displayCondition);
}

void displayScreen3() {
  // Режим 3: Информация о системе
  u8g2.setFont(u8g2_font_7x13_tf);
  
  u8g2.setCursor(0, 10);
  u8g2.print("St.Petersburg");
  
  u8g2.setCursor(0, 22);
  u8g2.print("WiFi: ");
  u8g2.print(WiFi.status() == WL_CONNECTED ? "OK" : "OFF");
  
  // Время последнего обновления
  u8g2.setFont(u8g2_font_5x8_tf);
  u8g2.setCursor(0, 32);
  u8g2.print("Last: ");
  int minutesAgo = (millis() - lastWeatherUpdate) / 60000;
  if (minutesAgo < 1) {
    u8g2.print("<1m ago");
  } else {
    u8g2.print(minutesAgo);
    u8g2.print("m ago");
  }
  
  // Координаты
  u8g2.setCursor(70, 32);
  u8g2.print("59.93,30.34");
}

void printLocalTime() {
  // Автопереключение экранов
  if (millis() - lastDisplayChange > DISPLAY_CHANGE_INTERVAL) {
    displayMode = (displayMode + 1) % 3;
    lastDisplayChange = millis();
    display.clearDisplay();
  }
  
  // Автообновление погоды каждые 5 минут
  if (millis() - lastWeatherUpdate > WEATHER_UPDATE_INTERVAL) {
    updateWeather();
  }
  
  display.clearDisplay();
  
  switch(displayMode) {
    case 0: displayScreen1(); break;
    case 1: displayScreen2(); break;
    case 2: displayScreen3(); break;
  }
  
  display.display();
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting ESP32 Weather Station with Real Weather Data...");

  // Инициализация дисплея
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  u8g2.begin(display);
  
  // Экран загрузки
  u8g2.setFont(u8g2_font_10x20_tf);
  u8g2.setCursor(24, 16);
  u8g2.print("WEATHER");
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setCursor(37, 30);
  u8g2.print("STATION");
  display.display();
  delay(1500);

  // Подключение к WiFi (как в рабочем коде)
  display.clearDisplay();
  u8g2.setFont(u8g2_font_7x13_tf);
  u8g2.setCursor(0, 12);
  u8g2.print("Connecting to");
  u8g2.setCursor(0, 26);
  u8g2.print("WiFi");
  display.display();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    spinner();
    display.display();
  }

  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  display.clearDisplay();
  u8g2.setFont(u8g2_font_7x13_tf);
  u8g2.setCursor(0, 12);
  u8g2.print("Online");
  u8g2.setCursor(0, 26);
  u8g2.print("SPB Weather...");
  display.display();

  // Настройка времени
  configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);
  
  // Первое получение погоды (как в рабочем коде)
  updateWeather();
  lastWeatherUpdate = millis();
  
  delay(2000);
  display.clearDisplay();
  
  Serial.println("System ready - starting main loop with real weather data");
}

void loop() {
  printLocalTime();
  delay(1000);
}
