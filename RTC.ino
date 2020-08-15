#include <Arduino.h>
#include <RTCZero.h>
#include <TimerTCC0.h>
#include "xaioclock/clockboard.h"

//#include <TimerTC3.h>

/* Create an rtc object */
RTCZero rtc;

/* グローバル変数 */
const unsigned char numSeg[] ={SEG_0,SEG_1,SEG_2,SEG_3,SEG_4,SEG_5,SEG_6,SEG_7,SEG_8,SEG_9};
const unsigned char monthDay[] ={JAN,FEB,MRH,APR,MAY,JUN,JLY,AUG,SEP,OCT,NOV,DEC};

unsigned char alrmFlg = 0x00;         // アラーム起動フラグ
unsigned char enAlrm  = 0x00;         // アラーム設定確認変数

unsigned char dig[4];                 // 7セグメントLED

unsigned char bz_pat;                 // ブザーパターン格納変数

unsigned long timer = 0;

unsigned long tmr_sw[3];


Outdata seg;

ctrlDig ctrl_dig;

// 時間の設定
void display_hhmm(void){
  unsigned int hours,minutes;
  
  hours = rtc.getHours();
  minutes = rtc.getMinutes();

  dig[DIG_1] = numSeg[( hours / 10)];
  dig[DIG_2]  = numSeg[(hours % 10)];
  dig[DIG_3] = numSeg[(minutes / 10)];
  dig[DIG_4]  = numSeg[(minutes % 10)];  

}

// 時間の表示
void display_mmss(void){
  
  unsigned int minutes,seconds;
  
  minutes = rtc.getMinutes();
  seconds = rtc.getSeconds();

  dig[DIG_1] = numSeg[(minutes / 10)];
  dig[DIG_2]  = numSeg[(minutes % 10)];
  dig[DIG_3] = numSeg[(seconds / 10)];
  dig[DIG_4]  = numSeg[(seconds % 10)];  
}

// 時間の表示
void display_date(void){
  int month, day;
  
  month = rtc.getMonth();
  day = rtc.getDay();

  dig[DIG_1] = numSeg[(month / 10)];
  dig[DIG_2]  = numSeg[(month % 10)];
  dig[DIG_3] = numSeg[(day / 10)];
  dig[DIG_4]  = numSeg[(day % 10)]; 

}

/*******************************************************************************
*   ShiftOut - シフトレジスタへの出力関数                                        *
*    bit    : 出力するビット数を指定                                             *
*    val    : 出力するデータ                                                    *
*    return : 戻り値なし　　　　　　　　　　　　　　　　　　　　　　　　            *
*******************************************************************************/
void ShiftOut(unsigned char bit, unsigned short val)
{
  digitalWrite(LATCHPIN, LOW); // 送信中はLATCHPINをLOWに

  // シフトレジスタにデータを送る
  for (int i = 0; i < bit; i++)
  {
    // データの設定
    digitalWrite(DATAPIN, !!(val & (1L << i)));

    //　書き込みクロック
    digitalWrite(CLOCKPIN, HIGH);
    digitalWrite(CLOCKPIN, LOW);
  }

  digitalWrite(LATCHPIN, HIGH); // 送信後はLATCHPINをHIGHに戻す
}

/*******************************************************************************
*   setDate - 日付設定の処理関数                                                *
*    sw     : スイッチ状態格納構造体                                            *
*    return : 戻り値なし　　　　　　 　　　　　　　　　　　　　　　　　            *
*******************************************************************************/
void setDateMode(TactSw *sw) {
  unsigned char digt;
  unsigned char pos;
  unsigned char val[4],month,day,year;
  unsigned char i; 
  
  // 初期値設定
  year = rtc.getYear();
  month = rtc.getMonth();
  day = rtc.getDay();

  while(1){

    val[DIG_1] = 2;
    val[DIG_2] = 0;
    val[DIG_3] = (year / 10);
    val[DIG_4] = (year % 10); 

    for(pos=DIG_1; pos <= DIG_4;pos++){
      dig[pos] = numSeg[val[pos]];
    }

    digt = val[DIG_3] * 10 + val[DIG_4];
    
    noInterrupts();
    ctrl_dig.dig_Ctrl[DIG_3].isFlash = 0x01; 
    ctrl_dig.dig_Ctrl[DIG_4].isFlash = 0x01;
    interrupts();

    // 年の入力処理
    while(1) {
      
      checkSw(sw);          
    
      if (sw->tactSw[TACTSW_1].isState == 0x01){
        sw->tactSw[TACTSW_1].isState = 0x00; // スイッチ状態クリア
        // 数値カウントアップ
        digt++; 
      }

      if (sw->tactSw[TACTSW_2].isState == 0x01){
        sw->tactSw[TACTSW_2].isState = 0x00; // スイッチ状態クリア            
        // 数値カウントアップ
        digt += 10;
      }
      
      if(digt > 63) digt = 0;

      if (sw->tactSw[TACTSW_3].isState == 0x01){
        sw->tactSw[TACTSW_3].isState = 0x00; // スイッチ状態クリア
        year = digt;
        noInterrupts();
          ctrl_dig.dig_Ctrl[DIG_3].isFlash = 0x00; 
          ctrl_dig.dig_Ctrl[DIG_3].isState = 0x01; 
          ctrl_dig.dig_Ctrl[DIG_4].isFlash = 0x00;
          ctrl_dig.dig_Ctrl[DIG_4].isState = 0x01; 
        interrupts();
        break;  
      }
      // データの表示
      dig[DIG_1] = numSeg[val[DIG_1]]; 
      dig[DIG_2] = numSeg[val[DIG_2]]; 
      dig[DIG_3] = numSeg[digt / 10]; 
      dig[DIG_4] = numSeg[digt % 10]; 
    }      


    // 月日の処理
    val[DIG_1] = (month / 10);
    val[DIG_2] = (month % 10);
    val[DIG_3] = (  day / 10);
    val[DIG_4] = (  day % 10);

    for(pos=DIG_1; pos <= DIG_4;pos++){
      ctrl_dig.dig_Ctrl[pos].isState = 0x01;
      dig[pos] = numSeg[val[pos]];
    }

    digt = month;

    noInterrupts();  
      ctrl_dig.dig_Ctrl[DIG_1].isFlash = 0x01;
      ctrl_dig.dig_Ctrl[DIG_2].isFlash = 0x01;
    interrupts();

    for(pos = DIG_2; pos <= DIG_4;) {
      
      checkSw(sw);
      
      if (sw->tactSw[TACTSW_1].isState == 0x01){
        sw->tactSw[TACTSW_1].isState = 0x00; // スイッチ状態クリア
        // 数値カウントアップ
        digt++;
      }

      if (sw->tactSw[TACTSW_2].isState == 0x01){
        sw->tactSw[TACTSW_2].isState = 0x00; // スイッチ状態クリア
        // 数値カウントアップ
        digt += 10;
      }

      // 桁の最大値処理
      switch (pos){
        case DIG_2:   /* 2桁目 */
            if(digt > 12) digt = 1;
            val[DIG_1] = digt / 10;
            val[DIG_2] = digt % 10;        
            break;


        case DIG_4:   /* 4桁目 */
            if(month == 2){
              /* 閏年の検出 */
              if ((2000+year) % 400 == 0 || (2000+year) % 4 == 0 && (2000+year) % 100 != 0) {
                /* 閏年 */
                if(digt > monthDay[month - 1] + 1) digt = 1;
              }
              else
              {
                /* 平年 */
                if(digt > monthDay[month - 1]) digt = 1;
              }
              
            } 
            else {
              /* 2月以外*/
              if(digt > monthDay[month - 1]) digt = 1;
            }
            val[DIG_3] = digt / 10;
            val[DIG_4] = digt % 10; 
            break;
      }
      
      // データの表示
      dig[DIG_1] = numSeg[val[DIG_1]]; 
      dig[DIG_2] = numSeg[val[DIG_2]]; 
      dig[DIG_3] = numSeg[val[DIG_3]]; 
      dig[DIG_4] = numSeg[val[DIG_4]];

      /* 入力桁を移動 */
      if (sw->tactSw[TACTSW_3].isState == 0x01){
        sw->tactSw[TACTSW_3].isState = 0x00; // スイッチ状態クリア
        
        if(pos == DIG_2){
          
          noInterrupts();
            ctrl_dig.dig_Ctrl[DIG_1].isFlash = 0x00;
            ctrl_dig.dig_Ctrl[DIG_2].isFlash = 0x00;
            ctrl_dig.dig_Ctrl[DIG_1].isState = 0x01;
            ctrl_dig.dig_Ctrl[DIG_2].isState = 0x01;
            
            ctrl_dig.dig_Ctrl[DIG_3].isFlash = 0x01;
            ctrl_dig.dig_Ctrl[DIG_4].isFlash = 0x01;
          interrupts();

          month = val[DIG_1] * 10 + val[DIG_2];
          digt = day;
          pos = DIG_4;
        }
        else {
          day   = val[DIG_3] * 10 + val[DIG_4];
          break;
        }   
      }
    }
    
    /* 点滅制御設定 */
    noInterrupts();
    for(pos = DIG_1;pos <= DIG_4; pos++){
      ctrl_dig.dig_Ctrl[pos].isState = 0x01;
      ctrl_dig.dig_Ctrl[pos].isFlash = 0x01;
    }
    interrupts();

    // 最終書き込み
    while(1){
      checkSw(sw);

      if (sw->tactSw[TACTSW_1].isState == 0x01){
          sw->tactSw[TACTSW_1].isState = 0x00; // スイッチ状態クリア
      }

      if (sw->tactSw[TACTSW_2].isState == 0x01){
          sw->tactSw[TACTSW_2].isState = 0x00; // スイッチ状態クリア
          /* 点滅制御解除 */
          noInterrupts();
          for(pos = DIG_1;pos <= DIG_4; pos++){
            ctrl_dig.dig_Ctrl[pos].isFlash = 0x00;
            ctrl_dig.dig_Ctrl[pos].isState = 0x01;
          }
          interrupts();
          break;
      }  
      
      /* 月日データ格納 */            
      if (sw->tactSw[TACTSW_3].isState == 0x01){
        sw->tactSw[TACTSW_3].isState = 0x00; // スイッチ状態クリア
        rtc.setDate(day, month, year);
              
        /* 点滅制御解除 */
        noInterrupts();
        for(pos = DIG_1;pos <= DIG_4; pos++){
          ctrl_dig.dig_Ctrl[pos].isFlash = 0x00;
          ctrl_dig.dig_Ctrl[pos].isState = 0x01;
        }
        interrupts();
        
        // ブザー音設定
        bz_pat = BUZ_PTN01;

        return;
      } 
    }
  }
}


/*******************************************************************************
*   setTime - 時間設定の処理関数                                                *
*    sw     : スイッチ状態格納構造体                                            *
*    return : 戻り値なし　　　　　　 　　　　　　　　　　　　　　　　　            *
*******************************************************************************/
void setTime(TactSw *sw, unsigned char *hours,unsigned char *minutes) {
  unsigned char digt;
  unsigned char pos;
  unsigned char val[4]; 

  // 時間入力処理
  while(1){
    
    /* 時間の格納*/
    val[DIG_1] = ( *hours / 10);
    val[DIG_2] = ( *hours % 10);
    val[DIG_3] = ( *minutes / 10);
    val[DIG_4] = ( *minutes % 10);  
    
    /* 処理変数の初期化*/
    digt = val[DIG_1] * 10 + val[DIG_2];

    /* 桁の点滅処理をON */
    noInterrupts();
    ctrl_dig.dig_Ctrl[DIG_1].isFlash = 0x01;          
    ctrl_dig.dig_Ctrl[DIG_2].isFlash = 0x01; 
    interrupts();

    /* 時間の設定処理*/   
    for(pos=DIG_2; pos <= DIG_4;) {
      
      // スイッチ確認
      checkSw(sw);

      /* スイッチ1が押された時 -> 1カウントアップ */
      if (sw->tactSw[TACTSW_1].isState == 0x01){
        sw->tactSw[TACTSW_1].isState = 0x00; // スイッチ状態クリア
        // 数値カウントアップ
        digt++;
      }
      /* スイッチ2が押された時 -> 10カウントアップ*/
      if (sw->tactSw[TACTSW_2].isState == 0x01){
        sw->tactSw[TACTSW_2].isState = 0x00; // スイッチ状態クリア
        digt += 10;
      }
      
      // 桁の最大値処理
      switch (pos){
        case DIG_2:     /* 2桁目 */
          if(digt > 23 ) digt = 0;
          // 値更新
            val[DIG_1] = digt / 10;
            val[DIG_2] = digt % 10;
          break;

        case DIG_4:   /* 4桁目 */
          if(digt > 59) digt = 0;
            val[DIG_3] = digt / 10;
            val[DIG_4] = digt % 10; 
          break;

        default:
          break;
      }

      /* 設定処理 */            
      if (sw->tactSw[TACTSW_3].isState == 0x01){
        sw->tactSw[TACTSW_3].isState = 0x00; // スイッチ状態クリア
        
        /* 入力桁を移動 */
        noInterrupts();
          ctrl_dig.dig_Ctrl[pos-1].isFlash = 0x00;
          ctrl_dig.dig_Ctrl[pos-1].isState = 0x01;
          ctrl_dig.dig_Ctrl[pos].isFlash = 0x00;
          ctrl_dig.dig_Ctrl[pos].isState = 0x01;
          ctrl_dig.dig_Ctrl[DIG_CRN].isFlash = 0x00;
          ctrl_dig.dig_Ctrl[DIG_CRN].isState = 0x01;
        interrupts();

        pos += 2;
        
        /* 分までの設定が終われば最終確認へ */
        if(pos > DIG_4) break; 
        else { 
          digt = val[DIG_3] * 10 + val[DIG_4];
          noInterrupts();
          ctrl_dig.dig_Ctrl[DIG_CRN].isFlash = 0x01;
          ctrl_dig.dig_Ctrl[pos-1].isFlash = 0x01;
          ctrl_dig.dig_Ctrl[pos].isFlash = 0x01;
          interrupts();
        }
      
      }
      // データの表示
      dig[DIG_1] = numSeg[val[DIG_1]]; 
      dig[DIG_2] = numSeg[val[DIG_2]]; 
      dig[DIG_3] = numSeg[val[DIG_3]]; 
      dig[DIG_4] = numSeg[val[DIG_4]];
    }

    /* 点滅制御設定 */
    noInterrupts();
    for(pos = DIG_1;pos <= DIG_CRN; pos++){
      ctrl_dig.dig_Ctrl[pos].isState = 0x01;
    } 
    for(pos = DIG_1;pos <= DIG_CRN; pos++){
      ctrl_dig.dig_Ctrl[pos].isFlash = 0x01;
    }
    interrupts();
    
    *hours   = val[DIG_1] * 10 + val[DIG_2];
    *minutes = val[DIG_3] * 10 + val[DIG_4];

    /* データを書き込み */
    while(1){ 
      
      /* スイッチ確認 */
      checkSw(sw);

      /* スイッチ1が押された -> 何もなし*/
      if (sw->tactSw[TACTSW_1].isState == 0x01){
        sw->tactSw[TACTSW_1].isState = 0x00; 
      }

      /* スイッチ2が押された -> 再度設定へ */
      if (sw->tactSw[TACTSW_2].isState == 0x01){
        sw->tactSw[TACTSW_2].isState = 0x00; // スイッチ状態クリア
        noInterrupts();
        ctrl_dig.dig_Ctrl[DIG_1].isFlash = 0x01;
        ctrl_dig.dig_Ctrl[DIG_2].isFlash = 0x01;
        ctrl_dig.dig_Ctrl[DIG_3].isFlash = 0x00;          
        ctrl_dig.dig_Ctrl[DIG_4].isFlash = 0x00;
        ctrl_dig.dig_Ctrl[DIG_3].isState = 0x01;          
        ctrl_dig.dig_Ctrl[DIG_4].isState = 0x01; 
        interrupts();
        break;
      }
      
      /* スイッチ3が押された -> 時間を書き込み */
      if (sw->tactSw[TACTSW_3].isState == 0x01){
        sw->tactSw[TACTSW_3].isState = 0x00; // スイッチ状態クリア
        return;
      }
    }
  }
}

void setTimeMode(TactSw *sw){
  
  unsigned char pos, hours, minutes;
  
  noInterrupts();   
    ctrl_dig.dig_Ctrl[DIG_CRN].isState = 0x01;
    ctrl_dig.dig_Ctrl[DIG_CRN].isFlash = 0x01;
  interrupts();
          
  // 初期値設定
  hours = rtc.getHours();
  minutes = rtc.getMinutes();

  setTime(sw, &hours, &minutes);        
  rtc.setTime(hours, minutes, 0);
        
  noInterrupts();
  for(pos = DIG_1; pos <= DIG_4 ;pos++){      
    ctrl_dig.dig_Ctrl[pos].isFlash = 0x00;
    ctrl_dig.dig_Ctrl[pos].isState = 0x01;
  }
  interrupts();

  // ブザー音設定
  bz_pat = BUZ_PTN01;

  ctrl_dig.dig_Ctrl[DIG_CRN].isState = 0x00;
  ctrl_dig.dig_Ctrl[DIG_CRN].isFlash = 0x00;
          
}

void setAlarm(TactSw *sw){
  unsigned char pos, hours, minutes;

  noInterrupts();   
    ctrl_dig.dig_Ctrl[DIG_CRN].isState = 0x01;
    ctrl_dig.dig_Ctrl[DIG_CRN].isFlash = 0x01;
  interrupts();
          
  // 初期値設定
  hours = rtc.getHours();
  minutes = rtc.getMinutes();

  setTime(sw, &hours, &minutes);        
  rtc.setAlarmTime(hours, minutes, 0);
  rtc.enableAlarm(rtc.MATCH_HHMMSS);
  
  rtc.attachInterrupt(alarmMatch);
        
  noInterrupts();
  for(pos = DIG_1; pos <= DIG_4 ;pos++){      
    ctrl_dig.dig_Ctrl[pos].isFlash = 0x00;
    ctrl_dig.dig_Ctrl[pos].isState = 0x01;
  }
  interrupts();

  // ブザー音設定
  bz_pat = BUZ_PTN02;

  ctrl_dig.dig_Ctrl[DIG_CRN].isState = 0x00;
  ctrl_dig.dig_Ctrl[DIG_CRN].isFlash = 0x00;
}

void setAlarmMode(TactSw *sw){
  unsigned char mode;
  unsigned char pos, hours, minutes;
  
  mode = enAlrm;

  while(1){
    
    checkSw(sw);

    if (sw->tactSw[TACTSW_1].isState == 0x01){
      sw->tactSw[TACTSW_1].isState = 0x00; // スイッチ状態クリア
      /* モード変更 */
      (mode == 1) ? mode = 0 : mode++ ;
    }

    if (sw->tactSw[TACTSW_2].isState == 0x01){
      sw->tactSw[TACTSW_2].isState = 0x00; // スイッチ状態クリア
    }
    
    if (sw->tactSw[TACTSW_3].isState == 0x01){
      sw->tactSw[TACTSW_3].isState = 0x00; // スイッチ状態クリア
      switch (mode)
      {
        /* アラームオフ */
        case 0:
          enAlrm = 0x00;
          rtc.detachInterrupt();        
          rtc.disableAlarm();
          seg.regData.r_led = 0x00;
          bz_pat = BUZ_PTN01;
          return;
          break;

        /* アラームオン */
        case 1:
          enAlrm = 0x01;
          setAlarm(sw);
          seg.regData.r_led = 0x01;
          return;
          break;
      }   
    }

    /* 表示 */ 
    dig[DIG_1] = SEG_A;
    dig[DIG_2] = SEG_L;
    dig[DIG_3] = numSeg[0];
    dig[DIG_4] = numSeg[mode];
  }
}


void setMode(TactSw *sw){
  unsigned char mode;
  
  mode = 0;
  ctrl_dig.dig_Ctrl[DIG_CRN].isState = 0x00;

  while(1){
    
    checkSw(sw);

    if (sw->tactSw[TACTSW_1].isState == 0x01){
      sw->tactSw[TACTSW_1].isState = 0x00; // スイッチ状態クリア
      /* モード変更 */
      (mode == SET_MODE) ? mode = 0 : mode++ ;
    }

    if (sw->tactSw[TACTSW_2].isState == 0x01){
      sw->tactSw[TACTSW_2].isState = 0x00; // スイッチ状態クリア
       // 表示モード切替
      switch (mode)
      {
        case 0:
          ctrl_dig.dig_Ctrl[DIG_CRN].isState = 0x00;
          setAlarmMode(sw);
          break;

        /* 時刻設定 */
        case 1:
          setTimeMode(sw);
          break;

        /* 日付設定 */
        case 2:
          ctrl_dig.dig_Ctrl[DIG_CRN].isState = 0x00;
          setDateMode(sw);

          break;
      
        default:
            break;
      }
    }
                  
    if (sw->tactSw[TACTSW_3].isState == 0x01){
      sw->tactSw[TACTSW_3].isState = 0x00; // スイッチ状態クリア
      return;
    } 
    
    /* 表示 */ 
    dig[DIG_1] = SEG_P;
    dig[DIG_2] = SEG_minus;
    dig[DIG_3] = numSeg[(mode / 10)];
    dig[DIG_4] = numSeg[(mode % 10)];
  
  }
  return;
}

// セットアップ
void setup()
{
  int   i;

  rtc.begin(); // initialize RTC

  // 入出力ピンの設定
  pinMode(DATAPIN, OUTPUT);
  pinMode(LATCHPIN, OUTPUT);
  pinMode(CLOCKPIN, OUTPUT);
  pinMode(SW1, INPUT_PULLUP);
  pinMode(SW2, INPUT_PULLUP);
  pinMode(SW3, INPUT_PULLUP);
  
  // アラーム停止
  rtc.disableAlarm();
  rtc.detachInterrupt();
  
  // タイマーの設定
  //TimerTc3.initialize(100000);                // 100msに設定
  //TimerTc3.attachInterrupt(timerTc3Isr);    // 割込関数の設定

  TimerTcc0.initialize(1000);               // 1msに設定
  TimerTcc0.attachInterrupt(timerTcc0Isr);  // 割込関数の設定

  // スイッチ関係の状態初期化
  for(i = DIG_1 ; i <= DIG_CRN ;i++){
    ctrl_dig.dig_Ctrl[i].isFlash = 0x00;
    ctrl_dig.dig_Ctrl[i].isState = 0x01;
  }
 
  // LEDの状態初期化
  seg.regData.r_led = 0x00;
  seg.regData.y_led = 0x00;

  // ブザー初期化
  bz_pat = 0x00;
  
}

/* メインループ関数 */
void loop()
{
  static unsigned char mode = 0x00;
  static TactSw sw;

  /* スイッチ検出 */
  checkSw(&sw);
  
  /* スイッチ1の状態検出 */
  if (sw.tactSw[TACTSW_1].isState == 0x01){
    sw.tactSw[TACTSW_1].isState = 0x00; // スイッチ状態クリア
    /* アラームが起動しているとき*/
    if (alrmFlg == 0x01) {
      alrmFlg = 0x00;
      bz_pat = 0x00;
    }
    else
    {
      /* モード変更 */
      (mode == 2) ? mode = 0 : mode++ ;
    }
  }

  
  if (sw.tactSw[TACTSW_2].isState == 0x01){
    
    sw.tactSw[TACTSW_2].isState = 0x00; // スイッチ状態クリア
    /* アラームが起動しているとき*/
    if (alrmFlg == 0x01) {
      alrmFlg = 0x00;
      bz_pat = 0x00;
    }
    else
    {
      /* モード変更 */
      (mode == 0) ? mode = 2 : mode-- ;
    }    
  }

  if (sw.tactSw[TACTSW_3].isState == 0x01){
    sw.tactSw[TACTSW_3].isState = 0x00; // スイッチ状態クリア
    
    /* アラームが起動しているとき*/
    if (alrmFlg == 0x01) {
      alrmFlg = 0x00;
      bz_pat = 0x00;
    }
    else
    {
      seg.regData.y_led = 0x01;
    
    /* モード変更 */
    setMode(&sw);
    seg.regData.y_led = 0x00;
    }
  }

  /* アラームのパターンを再設定 */
  if (alrmFlg == 0x01) {
    if (bz_pat == 0x00) bz_pat = BUZ_PTN02;
  }

  /* 表示モード */
  switch (mode)
  {
    case 0:
      ctrl_dig.dig_Ctrl[DIG_CRN].isState = 0x01;
      display_hhmm();
      break;

    case 1:
      ctrl_dig.dig_Ctrl[DIG_CRN].isState = 0x01;
      display_mmss();
      break;
    
    case 2:
        ctrl_dig.dig_Ctrl[DIG_CRN].isFlash = 0x00;
        ctrl_dig.dig_Ctrl[DIG_CRN].isState = 0x00;
        display_date();
        break;

    default:
        break;
  }
}

/*******************************************************************************
* checkSw - スイッチ状態確認                                                    *
*    argument : swFlg -> スイッチ状態変数                                       *
*               00   xx  xx  xx   (xx: 00 SW OFF 01 SW ON 11:SW ON -> SW OFF)  * 
*               None SW3 SW2 SW1                                               * 
*    return   : None                                                           *
*******************************************************************************/
unsigned char checkSw(TactSw *swFlg){ 
  
  static unsigned char md_sw[3]={0,0,0};
  unsigned char i;

  for(i = SW1; i <= SW3; i++){
    switch (md_sw[i-SW1])
    {
      case 0:
        /* code */
        if(digitalRead(i) == ON) {
          md_sw[i-SW1]++; 
          tmr_sw[i-SW1] = 0;
        }
        break;

      case 1:
        if (tmr_sw[i-SW1] > 2)
        {
          if(digitalRead(i) == ON) {
              md_sw[i-SW1]++;
              swFlg->tactSw[i-SW1].isOn = 0x01;
          }
          else {
            md_sw[i-SW1] = 0;
          }
        }
        break;
      
      case 2:
        if((digitalRead(i) == OFF) && (swFlg->tactSw[i-SW1].isOn == 0x01)) {
          md_sw[i-SW1] = 0;
          swFlg->tactSw[i-SW1].isState = 0x01; 
          swFlg->tactSw[i-SW1].isOn = 0x00;
        }
        break;
        
      default:
        break;
    }
  }
}

/*******************************************************************************
* timerTcc0Isr - タイマーCC0の割り込み処理                                       *
*    argument : None                                                           *
*    return   : None                                                           *
*******************************************************************************/
void timerTcc0Isr()
{

  static unsigned char cnt = 0;
  static unsigned long tm = 0;
  static unsigned long buz_tm = 0;
  
  unsigned char dg;

   /* 7セグメント点滅制御 */
  if(tm > FLASH_TIME){ 
    for (dg = DIG_1; dg <= DIG_CRN; dg++)
    {
      if (ctrl_dig.dig_Ctrl[dg].isFlash == 0x01) 
        ctrl_dig.dig_Ctrl[dg].isState = ~( ctrl_dig.dig_Ctrl[dg].isState);
    }
    tm = 0;
  }
  
  /* ブザー制御*/
  if(buz_tm > BUZ_TIME){ 
    ((bz_pat & 0x80)) ? seg.regData.buzz = 0x01 : seg.regData.buzz = 0x00;
    bz_pat <<= 1;
    buz_tm = 0;
  }

  // タイマーカウントアップ
  tm++; buz_tm++; timer++;
  tmr_sw[TACTSW_1]++; tmr_sw[TACTSW_2]++; tmr_sw[TACTSW_3]++;
  

  // 出力データ初期化(7セグメント、セグメント表示関係)
  seg.outdata &= 0x00e0;

  switch (cnt)
  {
      case DIG_1: /* Display dig 1 of 7segment LED  */
              seg.regData.segData = dig[DIG_1];
              seg.regData.dig_1 = ctrl_dig.dig_Ctrl[DIG_1].isState;
              cnt++;
              break;

      case DIG_2: /* Display dig 2 of 7segment LED  */
              seg.regData.segData = dig[DIG_2];
              seg.regData.dig_2 = ctrl_dig.dig_Ctrl[DIG_2].isState;
              cnt++;
              break;
      
      case DIG_3: /* Display dig 3 of 7segment LED  */
              seg.regData.segData = dig[DIG_3];
              seg.regData.dig_3 = ctrl_dig.dig_Ctrl[DIG_3].isState;
              cnt++;
              break;
      
      case DIG_4: /* Display dig 4 of 7segment LED  */
              seg.regData.segData = dig[DIG_4];
              seg.regData.dig_4 = ctrl_dig.dig_Ctrl[DIG_4].isState;
              cnt++;
              break;

      case DIG_CRN: /* カンマ */
              seg.regData.segData = SEG_CRN;
              seg.regData.dig_dp = ctrl_dig.dig_Ctrl[DIG_CRN].isState;
              cnt = DIG_1;
              break;
  
      default: /* Not Pattern */
        break;
  }
  
  // シフトレジスタにデータを書き込み
  ShiftOut(16, seg.outdata);
}

/*******************************************************************************
*  alarmMatch - アラーム時のコールバック関数                                     *
*    argument : None                                                           *
*    return   : None                                                           *
*******************************************************************************/
void alarmMatch()
{
  bz_pat = BUZ_PTN02;
  alrmFlg = 0x01;
}