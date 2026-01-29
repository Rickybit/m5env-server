#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedENV.h>

#define USING_ENV4

namespace {
  auto& lcd = M5.Display;
  M5Canvas canvas(&M5.Display);

  m5::unit::UnitUnified Units;

  #if defined(USING_ENV4)
  m5::unit::UnitENV4 unitENV4;
  #else
  m5::unit::UnitSHT40 unitSHT40;
  m5::unit::UnitBMP280 unitBMP280;
  #endif
  
  #if defined(USING_ENV4)
  auto& sht40  = unitENV4.sht40;
  auto& bmp280 = unitENV4.bmp280;
  #else
  auto& sht40  = unitSHT40;
  auto& bmp280 = unitBMP280;
  #endif

  // 変数
  float temp_sht40 = 0;
  float humidity = 0;
  float pressure = 0;
}

void setup() {

  M5.begin();
  Serial.begin(115200);

  //初期設定
  lcd.setRotation(1);
  lcd.setTextDatum(TL_DATUM);
  lcd.fillScreen(BLACK);

  //タイトル描画
  lcd.setTextSize(2);
  lcd.setTextColor(WHITE, BLACK);
  lcd.setCursor(10, 10);
  lcd.println("Unit ENV-IV Sensor");
  int w = lcd.width();
  lcd.drawLine(0, 40, w, 40, BLUE);

  //デバック表示
  lcd.fillScreen(BLACK);
  lcd.setCursor(10, 10);
  lcd.println("Initializing...");

  auto pin_num_sda = M5.getPin(m5::pin_name_t::port_a_sda);
  auto pin_num_scl = M5.getPin(m5::pin_name_t::port_a_scl);
  M5_LOGI("getPin: SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
    
  lcd.setCursor(10, 30);
  lcd.printf("SDA:%d SCL:%d", pin_num_sda, pin_num_scl);

  // BMP280の初期設定
  {
    using namespace m5::unit::bmp280;
    auto cfg             = bmp280.config();
    cfg.osrs_pressure    = Oversampling::X16;
    cfg.osrs_temperature = Oversampling::X2;
    cfg.filter           = Filter::Coeff16;
    cfg.standby          = Standby::Time500ms;
    bmp280.config(cfg); 
  }

  // エラー処理
  auto fail = [&](const char* msg) {
    M5_LOGE("%s", msg);
    lcd.fillScreen(RED);
    lcd.setTextColor(WHITE, RED);
    lcd.setCursor(10, 100);
    lcd.println(msg);
    while (true) delay(10000);
  };

  Wire.begin(pin_num_sda, pin_num_scl, 400000U);

  lcd.setCursor(10, 60);
  lcd.println("Initializing...");

  #if defined(USING_ENV4)

  if (!Units.add(unitENV4, Wire)) {
    fail("Failed to add ENV4!");
  }
  #else
  if (!Units.add(unitSHT40, Wire)) {
    fail("Failed to add SHT40");
  }
  if (!Units.add(unitBMP280, Wire)) {
    fail("Failed to add BMP280");
  }
  #endif

  if (!Units.begin()) {
    fail("Failed to begin!");
  }
  
  M5_LOGI("M5UnitUnified begun");
  M5_LOGI("%s", Units.debugInfo().c_str());

  lcd.fillScreen(BLACK);
  lcd.setTextColor(GREEN, BLACK);
  lcd.setCursor(10, 100);
  lcd.println("Sensor Ready!");
  delay(2000);

  canvas.createSprite(lcd.width(), lcd.height());
  canvas.setTextDatum(TL_DATUM);
  M5_LOGI("Canvas created: %d x %d", lcd.width(), lcd.height());
}

void loop()
{
  M5.update();
  Units.update();

  // 電源オフ
  if (M5.BtnPWR.wasHold()) {
    lcd.fillScreen(BLACK);
    lcd.setTextColor(WHITE, BLACK);
    lcd.setTextSize(3);
    lcd.setCursor(60, 100);
    lcd.println("Power Off...");
    delay(1000);
    M5.Power.powerOff();
  }

  // SHT40のデータの更新
  if (sht40.updated()) {
    temp_sht40 = sht40.temperature();
    humidity = sht40.humidity();
    M5.Log.printf(">SHT40Temp:%.1f\n Humidity:%.1f\n", temp_sht40, humidity);
  }

  // BMP280のデータの更新
  if (bmp280.updated()) {
    auto p = bmp280.pressure();
    pressure = p * 0.01f;
    M5.Log.printf(">pressure:%.1f\n", pressure);
  }

  // 画面の描画更新
  static uint32_t lastUpdate = 0;
  uint32_t now = millis();

  if (now - lastUpdate > 500) {
    lastUpdate = now;

    canvas.fillScreen(BLACK);

    canvas.setTextSize(2);
    canvas.setTextColor(WHITE);
    canvas.setCursor(10, 5);
    canvas.println("=== Sensor Data ===");

    int y = 35;

    // 湿度
    canvas.setTextColor(CYAN);
    canvas.setCursor(10, y);
    canvas.printf("Humidity:    %.1f %%", humidity);

    y+=25;
    // 気温
    canvas.setTextColor(ORANGE);
    canvas.setCursor(10, y);
    canvas.printf("Temperature: %.1f C", temp_sht40);

    y+=25;
    // 気圧
    canvas.setTextColor(GREENYELLOW);
    canvas.setCursor(10, y);
    canvas.printf("Pressure:  %.1f hPa", pressure);

    canvas.pushSprite(0, 0);
  }

}