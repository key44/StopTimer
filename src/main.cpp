#include <Arduino.h>
#include <Wire.h>
#include <Time.h>
#include <TimeLib.h>
#include <DS1307RTC.h>
#include <SPI.h>
#include <SD.h>
#include <LiquidCrystal_I2C.h>


LiquidCrystal_I2C lcd(0x3F, 16, 2);

unsigned long time_m = 0;       //正の整数のみでカウント小数点切り捨て。スイッチが押された時間を格納
unsigned long time_n = 0;       //正の整数のみでカウント小数点切り捨て。スイッチが離された時間を格納
unsigned long time_nagasa = 0;  //正の整数のみでカウント小数点切り捨て。スイッチが押されている間の時間を格納
unsigned long time_sec_buf = 0; //正の整数のみでカウント小数点切り捨て。押されている間の時間を積算
const int chipSelect = 4;       //SDカードシールドを使用する場合必要なピン番号、シールド使用時は4番ピンは使用不可
const int sw = 7;               //リミットスイッチのピン番号
const int clock = 8;            //アナログ時計用の出力。分圧抵抗R1=470R R2=220R
int h = 0;                      //計測時間
int m = 0;                      //計測分
int s = 0;                      //計測秒
int h_log = 0;                  //積算計測時間
int m_log = 0;                  //積算計測分
int s_log = 0;                  //積算計測秒
/*
void reset()
{
  tmElements_t tm;
  RTC.read(tm); //RCTを使うときの初期設定
  h = 0;        //計測時間
  m = 0;        //計測分
  s = 0;        //計測秒
  h_log = 0;    //積算計測時間
  m_log = 0;    //積算計測分
  s_log = 0;    //積算計測秒
  lcd.setCursor(0, 1);
  lcd.print("StopTime=");
  lcd.print(h_log);
  lcd.print(":");
  lcd.print(m_log);
  lcd.print(":");
  lcd.print(s_log);
}
*/
void setup()
{
  tmElements_t tm;
  RTC.read(tm);                      //RCTを使うときの初期設定
  //attachInterrupt(0, reset, RISING); //0番ピンがOFFからONに変化したらreset関数を割り込み実行
  pinMode(sw, INPUT_PULLUP);         //arduino内でプルアップじているので直接スイッチをつける
  pinMode(clock, OUTPUT);            //アナログ時計を動かす用の出力設定
  Serial.begin(9600);
  SD.begin(chipSelect);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(2, 0);
  lcd.print("Stop Countor");
  lcd.setCursor(0, 1);
  lcd.print("made by yokokura");
  delay(3000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("2020");
  lcd.print("/");
  lcd.print(tm.Month);
  lcd.print("/");
  lcd.print(tm.Day);
  lcd.print(" ");
  lcd.print(tm.Hour);
  lcd.print(":");
  lcd.print(tm.Minute);
  lcd.setCursor(0, 1);
  lcd.print("StopTime=");
  lcd.print(h_log);
  lcd.print(":");
  lcd.print(m_log);
  lcd.print(":");
  lcd.print(s_log);
  digitalWrite(clock, LOW); //誤動作防止
}

void loop()
{
  tmElements_t tm;
  RTC.read(tm); //RCTを使うときの初期設定
  File dataFile = SD.open("datalog.csv", FILE_WRITE);

  while (digitalRead(sw) == HIGH) //スイッチが押されるまで待機
  {
  }
  Serial.print("start  ");
  Serial.print(tm.Month); //シリアルモニターに現在時刻とタレパンが停止した積算時間を表示
  Serial.print("/");
  Serial.print(tm.Day);
  Serial.print("  ");
  Serial.print(tm.Hour);
  Serial.print(":");
  Serial.println(tm.Minute);
  time_m = millis();             //スイッチが押され始めた時刻
  delay(100);                    //チャタリング対策
  digitalWrite(clock, HIGH);     //アナログ時計を起動
  while (digitalRead(sw) == LOW) //スイッチがONからOFFに変化した場合出力
  {
  }
  Serial.print("stop  ");
  Serial.print(tm.Month); //シリアルモニターに現在時刻とタレパンが停止した積算時間を表示
  Serial.print("/");
  Serial.print(tm.Day);
  Serial.print("  ");
  Serial.print(tm.Hour);
  Serial.print(":");
  Serial.println(tm.Minute);
  time_n = millis(); //スイッチが離された時刻
  delay(100);
  digitalWrite(clock, LOW);               //アナログ時計を停止
  time_nagasa = (time_n - time_m) / 1000; //スイッチが押下されて離された時間を算出(秒で表示)
  time_sec_buf += time_nagasa;            //スイッチが押下さてた時間を積算する。
  delay(100);

  while (time_nagasa >= 3600) //算出時間(秒)を時間、分、秒に変換
  {                           //時間を決定
    time_nagasa -= 3600;
    h++;
  }
  while (time_nagasa >= 60)
  { //分を決定
    time_nagasa -= 60;
    m++;
  }
  s = time_nagasa; //余りが秒

  while (time_sec_buf >= 3600) //積算された時間(秒)を時間、分、秒に変換
  {                            //時間を決定
    time_sec_buf -= 3600;
    h_log++;
  }
  while (time_sec_buf >= 60)
  {                  //分を決定
    if (m_log >= 60) //もし、分表示が60分以上になったら時間に繰り上がり
    {
      h_log++;
      m_log = 0;
    }

    time_sec_buf -= 60;
    m_log++;
  }

  s_log = time_sec_buf; //余りが秒

  Serial.print("rec ="); //シリアルモニターに積算された時間を
  Serial.print(h_log);
  Serial.print(":");
  Serial.print(m_log);
  Serial.print(":");
  Serial.println(s_log);

  lcd.clear(); //LCDにスイッチが押された時刻と総積算時間を表示(日をまたいでも、電源を落とさない限りリセットしない)
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("2020");
  lcd.print("/");
  lcd.print(tm.Month);
  lcd.print("/");
  lcd.print(tm.Day);
  lcd.print(" ");
  lcd.print(tm.Hour);
  lcd.print(":");
  lcd.print(tm.Minute);
  lcd.setCursor(0, 1);
  lcd.print("StopTime=");
  lcd.print(h_log);
  lcd.print(":");
  lcd.print(m_log);
  lcd.print(":");
  lcd.print(s_log);

  dataFile.print(tm.Month); //csvに現在時刻とタレパンが停止した時間を書き込み
  dataFile.print("/");
  dataFile.print(tm.Day);
  dataFile.print("  ");
  dataFile.print(tm.Hour);
  dataFile.print(":");
  dataFile.print(tm.Minute);
  dataFile.print(",");
  dataFile.print(h);
  dataFile.print(":");
  dataFile.print(m);
  dataFile.print(":");
  dataFile.println(s);
  dataFile.close();
}