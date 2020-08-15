/* ポート割り当て定義 */
// シフトレジスタ制御ピン
#define DATAPIN  7  // 74HC595のDSへ
#define LATCHPIN 8 // 74HC595のST_CPへ
#define CLOCKPIN 9 // 74HC595のSH_CPへ

// タクトスイッチ割り当て
#define SW1 4
#define SW2 5
#define SW3 6

/* 7セグメントLED関係 */
// 7セグメント表示パターン
#define SEG_0       0x7d
#define SEG_1       0x09
#define SEG_2       0x67
#define SEG_3       0x2f
#define SEG_4       0x1b
#define SEG_5       0x3e
#define SEG_6       0x7e
#define SEG_7       0x1d
#define SEG_8       0x7f
#define SEG_9       0x1f
#define SEG_A       0x5f
#define SEG_B       0x7a
#define SEG_C       0x74
#define SEG_D       0x6b
#define SEG_E       0x76
#define SEG_F       0x56
#define SEG_H       0x5b
#define SEG_L       0x70
#define SEG_P       0x57
#define SEG_minus   0x02
#define SEG_CRN     0x57    // 7セグメント中央のコロン

// 月の日数
#define     JAN     31
#define     FEB     28
#define     MRH     31
#define     APR     30
#define     MAY     31
#define     JUN     30
#define     JLY     31
#define     AUG     31
#define     SEP     30
#define     OCT     31
#define     NOV     30
#define     DEC     31

// ７セグメント桁番号
#define   DIG_1     0
#define   DIG_2     1
#define   DIG_3     2
#define   DIG_4     3
#define   DIG_CRN   4

// タクトスイッチ番号
#define TACTSW_1    0
#define TACTSW_2    1
#define TACTSW_3    2

// 制御定数
#define OFF 1
#define ON 0

#define FLASH_TIME  500
#define BUZ_TIME    100
#define SET_MODE    2

//ブザーパターン
#define BUZ_PTN01   0xf0;
#define BUZ_PTN02   0x55;


// セグメントデータの構造体・共用体
// シフトレジスタ出力用ビットフィールド
typedef struct ShfRegData
{
    unsigned short dig_1  : 1; // 下位ビット
    unsigned short dig_2  : 1;
    unsigned short dig_dp : 1;
    unsigned short dig_3  : 1;
    unsigned short dig_4  : 1;
    unsigned short buzz   : 1;
    unsigned short r_led  : 1;
    unsigned short y_led  : 1;
    unsigned short segData: 8; // 上位ビット
};

// 共用体
typedef union Outdata
{
    ShfRegData regData;
    unsigned short outdata;
    
};

// ７セグメント制御用構造体
typedef struct ctrlSegment
{
    unsigned char isFlash : 1;  // 0:Not Flash Mode 1: Blank Mode      
    unsigned char isState : 1;  // 0:Blank 1:Light
    unsigned char isDot   : 1;
};

typedef struct ctrlDig
{
    ctrlSegment dig_Ctrl[5];
};


// タクトスイッチ用構造体
typedef struct stateSW
{
    unsigned char isOn    : 1;  // 0: SW OFF 1: SW ON      
    unsigned char isState : 1;  // 0:スイッチ変化なし 1: SW ON->OFF
};

typedef struct TactSw
{
    stateSW tactSw[3];
};




