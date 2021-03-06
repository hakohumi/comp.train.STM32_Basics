/*
 * State.c
 *
 *  Created on: 2020/10/20
 *      Author: fuminori.hakoishi
 */

#include "State.h"

#include "Distance.h"
#include "LCD.h"
#include "LINBLE.h"
#include "Temp_ADC.h"
#include "Temp_I2C.h"
#include "UART.h"
#include "mystringfunc.h"

void State_runViewTemp(void);
void State_runViewDistance(void);
void State_runMemDump(void);
void State_runUARTRecieve(void);
void State_runDebugOutput(void);
void State_runBLE(void);
void State_runBLECentral(void);
void State_runRealtimeBLEInput(void);
void State_runRealtimeBLECentralInput(void);

// mainで宣言している
// システムの状態
static uint8_t SystemState;

// 処理の更新タイミング
bool State_UpdateFlg = false;

// 初期化
void State_Init(uint8_t i_state)
{
    SystemState = i_state;
}

// システムの状態遷移
void State_ChangeStateRoll(void)
{
    SystemState++;
    if (SystemState >= SYS_STATE_LENGTH)
    {
        SystemState = 0;
    }

    switch (SystemState)
    {
    case SYS_STATE_TEMP:
        PrintUART((uint8_t *)"\r\nSYS State TEMP\r\n");
        break;
    case SYS_STATE_DISTANCE:
        PrintUART((uint8_t *)"\r\nSYS State DISTANCE\r\n");
        break;
    case SYS_STATE_DEBUG_POINTER:
        PrintUART((uint8_t *)"\r\nSYS State DEBUG POINTER\r\n");
        break;
    case SYS_STATE_DEBUG_RECIEVE:
        PrintUART((uint8_t *)"\r\nSYS State DEBUG RECIEVE\r\n");
        break;
    case SYS_STATE_BLE:
        PrintUART((uint8_t *)"\r\nSYS_STATE_BLE\r\n");
        break;
    case SYS_STATE_BLE_CENTRAL:
        PrintUART((uint8_t *)"\r\nSYS_STATE_BLE_CENTRAL\r\n");
        break;
    case SYS_STATE_DEBUG:
        PrintUART((uint8_t *)"\r\nSYS State DEBUG\r\n");
        break;
    default:
        PrintUART((uint8_t *)"\r\nSYS State main未登録\r\n");
        break;
    }
}

void State_RunProcess(void)
{
    // 指定したタイマごとに処理をする
    if (State_UpdateFlg == true)
    {
        State_UpdateFlg = false;

        /* LCD に書き込み */
        switch (State_GetState())
        {
        case SYS_STATE_TEMP:
            State_runViewTemp();
            break;
        case SYS_STATE_DISTANCE:
            State_runViewDistance();
            break;
            // メモリダンプ
        case SYS_STATE_DEBUG_POINTER:
            State_runMemDump();
            break;
            // シリアル送受信
        case SYS_STATE_DEBUG_RECIEVE:
            State_runUARTRecieve();
            break;
        case SYS_STATE_BLE:
            State_runBLE();
            break;
        case SYS_STATE_BLE_CENTRAL:
            State_runBLECentral();
            break;
        case SYS_STATE_DEBUG:
            State_runDebugOutput();
            break;
        default:
            break;
        }
    }
}

// リアルタイムで実行される
void State_RunRealtimeProcess(void)
{
    static uint8_t l_StateOld = 99;
    uint8_t l_SysState = State_GetState();

    switch (l_SysState)
    {
    case SYS_STATE_TEMP:
        break;
    case SYS_STATE_DISTANCE:
        break;
    case SYS_STATE_DEBUG_POINTER:
        break;
    case SYS_STATE_DEBUG_RECIEVE:
        UART_receiveInput(SystemState);
        break;
    case SYS_STATE_BLE:
        // 状態に入って1回だけ実行する処理
        if (l_StateOld != l_SysState)
        {
            PrintUART((uint8_t *)"BLE Command Mode\r\n");
            PrintUART((uint8_t *)"Please enter the Commond.\r\n");
            PrintUART((uint8_t *)"Command list :\r\n");
            PrintUART((uint8_t *)"\"1\" : Transition to advertised state.\r\n");
            PrintUART((uint8_t *)"\"2\" : Print LINBLE firmware version.\r\n");
            PrintUART((uint8_t *)"\"3\" : Print LINBLE Bluetooth device address.\r\n");
            PrintUART((uint8_t *)"\"4\" : Print LNBLE State.\r\n");
        }

        UART_receiveInput(SystemState);
        State_runRealtimeBLEInput();
        break;

    case SYS_STATE_BLE_CENTRAL:
        // 状態に入って1回だけ実行する処理
        if (l_StateOld != l_SysState)
        {
            PrintUART((uint8_t *)"BLE Central Command Mode\r\n");
            PrintUART((uint8_t *)"Please enter the Commond.\r\n");
            PrintUART((uint8_t *)"Command list :\r\n");
            PrintUART((uint8_t *)"\"1\" : Transition to advertised state.\r\n");
            PrintUART((uint8_t *)"\"2\" : Print LINBLE firmware version.\r\n");
            PrintUART((uint8_t *)"\"3\" : Print LINBLE Bluetooth device address.\r\n");
            PrintUART((uint8_t *)"\"4\" : Print LNBLE State.\r\n");
            PrintUART((uint8_t *)"\"5\" : Connect to Peripheral LINBLE.\r\n");
        }

        UART_receiveInput(SystemState);
        State_runRealtimeBLECentralInput();
        break;
    case SYS_STATE_DEBUG:
        break;
    default:
        break;
    }

    l_StateOld = l_SysState;
}

void State_runViewTemp(void)
{
    // I2C温度センサの温度
    int8_t l_TempI2C = 0;

    // ADC温度センサの温度
    int8_t l_TempADC = 0;

#ifndef NOUSE_I2CTEMP
    // I2C温度センサの温度の取得
    l_TempI2C = TempI2C_GetTemp();
#endif
    // ADCで温度の値を読み取る
    l_TempADC = TEMP_ADC_GetTemp();

    // I2C 温度
    LCD_ClearBuffer();
    LCD_WriteToBuffer(0, (uint8_t *)"I2C:", 4);
    LCD_WriteToBufferInt(4, l_TempI2C, 2);
    // ADC 温度
    LCD_WriteToBuffer(8, (uint8_t *)"ADC:", 4);
    LCD_WriteToBufferInt(12, l_TempADC, 2);
}

void State_runViewDistance(void)
{
    // ADC距離センサの距離(cm)
    uint16_t l_DistanceADC = 0;

    // 距離センサの値を読み取る
    l_DistanceADC = Distance_ADC_GetDistance();

    LCD_ClearBuffer();
    LCD_WriteToBuffer(0, (uint8_t *)"DISTANCE", 8);
    LCD_WriteToBufferInt(8, l_DistanceADC, 3);
    LCD_WriteToBuffer(11, (uint8_t *)"cm", 2);
}

void State_runMemDump(void)
{
    // アドレスポインタ
    static uint8_t *l_p = 0x00000000;

    LCD_ClearBuffer();
    LCD_WriteToBuffer(0, (uint8_t *)"DBG ADDR", 8);
    LCD_WriteToBufferInt(8, *l_p, 8);
    printUARTHex((uint8_t *)"Pointer Address : ", (uint32_t)l_p, 8);
    printUARTHex((uint8_t *)"Pointer Value : ", (*l_p), 2);

    l_p++;
}
void State_runUARTRecieve(void)
{
    LCD_ClearBuffer();
    LCD_WriteToBuffer(0, (uint8_t *)"RECIEVE", 7);
}

void State_runDebugOutput(void)
{
    /* 表示処理 */

#ifdef MYDEBUG
    // 動作確認用のカウント
    static uint16_t l_count = 0;
#endif

#ifndef NOUSE_MOVING_AVERAGE_FILTER
    // 平均算出用
    uint16_t l_value;
    static uint16_t l_avrVal = 0;
    static uint8_t l_avrCnt = 0;
#endif

    LCD_ClearBuffer();
    LCD_WriteToBuffer(0, (uint8_t *)"DEBUG:", 5);

#ifdef MYDEBUG
// シリアルに温度送信
#ifndef NOUSE_I2CTEMP
#ifdef MYDEBUG_I2CTEMP
    PrintUART((uint8_t *)"i2c tmp : ");
    PrintUARTInt(TempI2C_GetTemp());
#endif
#endif

#ifdef MYDEBUG_ADCTEMP
    // アナログ温度センサの値を送信
    PrintUART((uint8_t *)"Tim21over : adc tmp : ");
    PrintUARTInt(TEMP_ADC_GetTemp());
#endif

// ADCした値をデバッグ
#ifndef NOUSE_MOVING_AVERAGE_FILTER

    PrintUART((uint8_t *)"ADC exTemp Value :");
    PrintUARTInt(ADC_GetRawValue(MY_ADC_DATA_IDX_EXTEMP));

    // 距離センサの平均値を求める処理
    l_value = ADC_GetRawValue(MY_ADC_DATA_IDX_DISTANCE);
    l_avrVal += l_value;
    l_avrCnt++;

    PrintUART((uint8_t *)"ADC Distance sensor Value :");
    PrintUARTint(l_value);
    //		printUART((uint8_t *)"ADC Distance Average  :", l_avrVal);
    if (l_avrCnt > 9)
    {
        l_avrVal /= l_avrCnt;
        PrintUART((uint8_t *)"Average cnt : ");
        PrintUARTInt(l_avrCnt);

        PrintUART((uint8_t *)"ADC Distance 10 times Average : ");
        PrintUARTInt(l_avrVal);
        l_avrVal = 0;
        l_avrCnt = 0;
    }

#endif

#ifdef MYDEBUG_DISTANCE
    PrintUART((uint8_t *)"ADC Distance :");
    PrintUARTInt((uint16_t)Distance_ADC_GetDistance());
#endif

    // 内蔵温度センサ
    //		printUART((uint8_t *)"ADC inTemp Value :", ADC_GetRawValue(MY_ADC_DATA_IDX_INTEMP));
    // 内部電圧値
    //		printUART((uint8_t *)"ADC VREFINT Value :", ADC_GetRawValue(MY_ADC_DATA_IDX_VREFINT));

    // デバッグ用
    PrintUART((uint8_t *)"Timer :  tim21  overflow cnt : ");
    PrintUARTInt((uint16_t)l_count);

    l_count++;

#endif
}

void State_runBLE(void)
{
#ifdef MYDEBUG_BLE_BUF
    uint8_t l_strBuf[64];
    uint8_t l_strLength;
#endif

    LCD_ClearBuffer();
    LCD_WriteToBuffer(0, (uint8_t *)"BLE", 3);

// #define MYDEBUG_BLE_BUF
#ifdef MYDEBUG_BLE_BUF
    l_strLength = LINBLE_GetReceiveData(&l_strBuf, 64);
    PrintUART((uint8_t *)"LINBLE Receive data buf : ");
    PrintUART(&l_strBuf);
    PrintUART((uint8_t *)"str length : ");
    PrintUARTInt(l_strLength);
    PrintUART((uint8_t *)"\r\n");
    l_strLength = UART_GetReceiveData(&l_strBuf, 64);
    PrintUART((uint8_t *)"UART Send data buf : ");
    PrintUART(&l_strBuf);
    PrintUART((uint8_t *)"str length : ");
    PrintUARTInt(l_strLength);
    PrintUART((uint8_t *)"\r\n");
#endif

// #define MYDEBUG_BLE_STATE
#ifdef MYDEBUG_BLE_STATE
    switch (LINBLE_GetState())
    {
    case LINBLE_STATE_COMMAND:
        LINBLE_SendCmdCheckStatus();
        break;
    case LINBLE_STATE_ADVERTISE:
        LINBLE_SendCmdCheckStatus();
        break;
    }

#endif
}

void State_runBLECentral(void)
{
    LCD_ClearBuffer();
    LCD_WriteToBuffer(0, (uint8_t *)"BLE", 3);
    LCD_WriteToBuffer(8, (uint8_t *)"CENTRAL", 7);
}

void State_runRealtimeBLEInput(void)
{
    static uint8_t l_linbleStateOld = LINBLE_STATE_COMMAND;
    uint8_t l_linbleState = LINBLE_GetState();
    uint8_t l_strBuf[64];
    uint8_t l_strLength;

    if (l_linbleState != l_linbleStateOld)
    {
        switch (l_linbleState)
        {
        case LINBLE_STATE_COMMAND:
            PrintUART((uint8_t *)"コマンド状態です。\r\n");
            break;
        case LINBLE_STATE_ADVERTISE:
            PrintUART((uint8_t *)"アドバタイズ状態です。\r\n");
            break;
        case LINBLE_STATE_ONLINE:
            PrintUART((uint8_t *)"オンライン状態です。\r\n");
            break;
        default:
            break;
        }
    }

    // LINBLEの状態を更新したい
    // リンブル（UART1）から受信があった時、
    // バッファを取りためて、
    // 現在のLINBLEの状態と、受信した文字とを考慮して、
    // 現在のLIBNLEの状態を更新する

    // 今の状態だと、ずっとLINBLEのバッファからリードすることになる
    // 受信待機フラグで1回だけ通るようにする
    // エンター押して送信したら、受信を待つようにする

    // 受信待機フラグが立っている間、受信データが揃う前からリードを試みている
    // 改善策：リザルトメッセージの最後を確認したらフラグを立て、そのフラグが立っている時のみ、メッセージの比較を行う
    // エンドラインフラグ

    // 解決 2020/10/22

    // 問題：DISCのメッセージを受取る時に、前のバッファも入って表示されてしまう
    // 原因：オンライン状態になって、終端なしの文字を受信すると、バッファの最後が終端以外の文字になる
    // 　　　すると、文字列の比較をした時に、バッファの最初から比較をした時に、ずれて比較されてしまう
    // 解決策：バッファの後ろから比較する
    // 解決策2：エンドラインフラグが立つタイミングの調整
    //            → エンドラインフラグは、リザルトメッセージの最後の\nが受信すると立つため、そこから解決策1を実行するのが良い
    // 解決策1の流れ：① エンドラインフラグが立ったとき、バッファの後ろから、探したい文字列の先頭文字をstrchrで検索し、ポインタの位置を取得
    // 　　　　　　　　② バッファのポインタに検索した文字のアドレスを足して、そこから文字列の比較(strcmp)を行う

    switch (l_linbleState)
    {
    case LINBLE_STATE_COMMAND:

        // リザルトメッセージ待機フラグが立っていたときのみ実行
        // if ((LINBLE_GetReceiveResultMesgWaitFlg() && LINBLE_GetEndLineFlg()) == true) {
        if (LINBLE_GetEndLineFlg() == true)
        {
            // コマンド状態中にBTAを入力されると、アドバタイズ状態へ遷移する
            // アドバタイズ状態へ遷移したことがわかるには、ACKN<CR><LF>
            l_strLength = LINBLE_GetReceiveData((uint8_t *)&l_strBuf, 64);

            if (l_strLength > 0)
            {
                // 受信したコマンドの表示
                PrintUART((uint8_t *)"Recevie Command : ");
                PrintUART((uint8_t *)&l_strBuf);
                PrintUART((uint8_t *)"\r\n");

                if (Mystring_FindStrFromEnd((uint8_t *)&l_strBuf, 64, (uint8_t *)"ACKN\r\n", 6) == 1)
                {
                    PrintUART((uint8_t *)"read ackn\r\n");
                    LINBLE_SetState(LINBLE_STATE_ADVERTISE);
                    // LINBLE_SetEscapeStateFlg();
                }
                else
                {
                    // 受信待機フラグをクリアする
                    LINBLE_ClrReceiveResultMesgWaitFlg();
                }

                // エンドラインフラグをクリア
                LINBLE_ClrEndLineFlg();
                // バッファカウンタのクリア
                LINBLE_BufferCountClear();
            }
            else
            {
                PrintUART((uint8_t *)"error linble run realtime ble input state command \r\n");
            }
        }
        else
        {
            // 何もしない
        }
        break;

    case LINBLE_STATE_ADVERTISE:

        // アドバタイズ状態は、受信メッセージをずっと受け付ける

        // if ((LINBLE_GetReceiveResultMesgWaitFlg() && LINBLE_GetEndLineFlg()) == true) {
        if (LINBLE_GetEndLineFlg() == true)
        {
            l_strLength = LINBLE_GetReceiveData((uint8_t *)&l_strBuf, 64);

            if (l_strLength > 0)
            {
                // 受信したコマンドの表示
                PrintUART((uint8_t *)"Recevie Command : ");
                PrintUART((uint8_t *)&l_strBuf);
                PrintUART((uint8_t *)"\r\n");

                if (Mystring_FindStrFromEnd((uint8_t *)&l_strBuf, 64, (uint8_t *)"CONN\r\n", 6) == 1)
                {
                    PrintUART((uint8_t *)"read CONN in advertise\r\n");
                    LINBLE_SetState(LINBLE_STATE_ONLINE);
                    LINBLE_ClrReceiveResultMesgWaitFlg();
                }
                else if (Mystring_FindStrFromEnd((uint8_t *)&l_strBuf, 64, (uint8_t *)"ACKN\r\n", 6) == 1)
                {
                    PrintUART((uint8_t *)"read ACKN in advertise\r\n");
                    // LINBLE_SetState(LINBLE_STATE_COMMAND);
                    // LINBLE_SetEscapeStateFlg();
                }
                else if (Mystring_FindStrFromEnd((uint8_t *)&l_strBuf, 64, (uint8_t *)"DISC\r\n", 6) == 1)
                {
                    PrintUART((uint8_t *)"read DISC in advertise\r\n");
                    LINBLE_SetState(LINBLE_STATE_COMMAND);
                    LINBLE_ClrReceiveResultMesgWaitFlg();
                }
                else
                {
                }

                // エンドラインフラグをクリア
                LINBLE_ClrEndLineFlg();
                // バッファカウンタのクリア
                LINBLE_BufferCountClear();
            }
            else
            {
                PrintUART((uint8_t *)"error linble run realtime ble input state command \r\n");
            }
        }

        break;
    case LINBLE_STATE_ONLINE:
        if (LINBLE_GetEndLineFlg() == false)
        {
            // 文字が送られて来た時
            uint8_t l_unreadCount = LINBLE_GetReceiveDataUnReadCount();
            if (l_unreadCount > 0)
            {
                // 未読バッファ数
                // PrintUARTInt(l_unreadCount);
                l_strLength = LINBLE_GetReceiveDataLast((uint8_t *)&l_strBuf, l_unreadCount + 1);
                if (l_strLength > 0)
                {
                    PrintUART((uint8_t *)"Receive Data : ");
                    PrintUART((uint8_t *)&l_strBuf);
                    PrintUART((uint8_t *)"\r\n");
                }
                else
                {
                    PrintUART((uint8_t *)"error ble linble state online\r\n");
                }
            }
        }
        // オンライン状態では、入出力をそのまま送受信する
        // ただ、"@@@"を入力するとエスケープ状態へ移行する
        // また、"DISC<CR><LF>を受信すると、アドバタイズ状態へ遷移する

        // コマンドを受信した場合
        // if ((LINBLE_GetReceiveResultMesgWaitFlg() && LINBLE_GetEndLineFlg()) == true) {
        if (LINBLE_GetEndLineFlg() == true)
        {
            l_strLength = LINBLE_GetReceiveData((uint8_t *)&l_strBuf, 64);

            if (l_strLength > 0)
            {
                // 受信したコマンドの表示
                PrintUART((uint8_t *)"Receive Command : ");
                PrintUART((uint8_t *)&l_strBuf);
                PrintUART((uint8_t *)"\r\n");

                if (Mystring_FindStrFromEnd((uint8_t *)&l_strBuf, 64, (uint8_t *)"DISC\r\n", 6) == 1)
                {
                    PrintUART((uint8_t *)"read DISC in online\r\n");
                    LINBLE_SetState(LINBLE_STATE_ADVERTISE);
                }
                else if (Mystring_FindStrFromEnd((uint8_t *)&l_strBuf, 64, (uint8_t *)"ACKN\r\n", 6) == 1)
                {
                    PrintUART((uint8_t *)"read ACKN in online\r\n");
                    // LINBLE_SetState(LINBLE_STATE_COMMAND);
                }
                else
                {
                }

                // エンドラインフラグをクリア
                LINBLE_ClrEndLineFlg();
                // バッファカウンタのクリア
                LINBLE_BufferCountClear();
            }
            else
            {
                PrintUART((uint8_t *)"error linble run realtime ble input state command \r\n");
            }
        }

        break;

    default:
        PrintUART((uint8_t *)"error unrealtime Ble input \r\n");
        break;
    }

    l_linbleStateOld = l_linbleState;
}

// セントラル側の処理
void State_runRealtimeBLECentralInput(void)
{
    static uint8_t l_linbleStateOld = LINBLE_STATE_COMMAND;
    uint8_t l_linbleState = LINBLE_GetState();
    uint8_t l_strBuf[64];
    uint8_t l_strLength;

    if (l_linbleState != l_linbleStateOld)
    {
        switch (l_linbleState)
        {
        case LINBLE_STATE_COMMAND:
            PrintUART((uint8_t *)"コマンド状態です。\r\n");
            break;
        case LINBLE_STATE_ADVERTISE:
            PrintUART((uint8_t *)"アドバタイズ状態です。\r\n");
            break;
        case LINBLE_STATE_ONLINE:
            PrintUART((uint8_t *)"オンライン状態です。\r\n");
            break;
        default:
            break;
        }
    }

    // まず、ペリフェラルと接続をしたい
    // ① スキャン
    // ② 接続
    // ③

    switch (l_linbleState)
    {
    case LINBLE_STATE_COMMAND:

        // リザルトメッセージ待機フラグが立っていたときのみ実行
        if (LINBLE_GetEndLineFlg() == true)
        {
            // if ((LINBLE_GetReceiveResultMesgWaitFlg() == true) && LINBLE_GetEndLineFlg()) == true) {
            l_strLength = LINBLE_GetReceiveData((uint8_t *)&l_strBuf, 64);

            // 何も入力されていない場合、エラー
            if (l_strLength <= 0)
            {
                PrintUART((uint8_t *)"error linble run realtime ble central input state command \r\n");
            }
            else
            {
                // ここから、コマンド実行フラグを元にして、
                // それぞれの処理をしたい

                // 現在の問題
                // コマンドによって、同じメッセージを返してくる ACKNなど
                // レシーブメッセージごとに処理を分けるのではなく、コマンドごとに分けたほうが良い
                // そこで、コマンドごとにフラグを作成し、コマンド送信時にフラグを立てて、立ったフラグごとにレシーブメッセージを受信する

                // BTIコマンドを実行した時の処理
                if (LINBLE_GetCmdFlg(LINBLE_FLG_CMD_BTI) == true)
                {
                    LINBLE_ReceiveDataBTI((uint8_t *)&l_strBuf, l_strLength);
                }
                else if (LINBLE_GetCmdFlg(LINBLE_FLG_CMD_BTC) == true)
                { // BTCコマンドを実行した時の処理
                    LINBLE_ReceiveDataBTC((uint8_t *)&l_strBuf, l_strLength);
                }
                else
                {
                    // LINBLEから受取ったデータをコンソールへ出力
                    PrintUART((uint8_t *)"Receive LINBLE data : ");
                    PrintUART((uint8_t *)&l_strBuf);
                    PrintUART((uint8_t *)"\r\n");
                }

                // 受信待機フラグをクリアする
                LINBLE_ClrReceiveResultMesgWaitFlg();

                // LINBLE_SetEscapeStateFlg();

                // エンドラインフラグをクリア
                LINBLE_ClrEndLineFlg();
                // バッファカウンタのクリア
                LINBLE_BufferCountClear();
            }
        }
        else
        {
            // 何もしない
        }
        break;
    case LINBLE_STATE_ONLINE:
        // リザルトメッセージ待機フラグが立っていたときのみ実行
        if ((LINBLE_GetEndLineFlg()) == true)
        {
            // l_strLength = 1;
            l_strLength = LINBLE_GetReceiveData((uint8_t *)&l_strBuf, 64);
            // l_strBuf[0] = LINBLE_GetReceiveCharLast();
            // l_strBuf[1] = '\0';

            // 何も入力されていない場合、エラー
            if (l_strLength <= 0)
            {
                PrintUART((uint8_t *)"error linble run realtime ble central input state online \r\n");
            }
            else
            {
                // LINBLEから受取ったデータをコンソールへ出力
                PrintUART((uint8_t *)"Receive LINBLE data : ");
                PrintUART((uint8_t *)&l_strBuf);
                PrintUART((uint8_t *)"\r\n");

                // 受信待機フラグをクリアする
                LINBLE_ClrReceiveResultMesgWaitFlg();

                // LINBLE_SetEscapeStateFlg();

                // エンドラインフラグをクリア
                LINBLE_ClrEndLineFlg();
                // バッファカウンタのクリア
                LINBLE_BufferCountClear();
            }
        }
        else
        {
            // 何もしない
        }
        break;
    default:
        PrintUART((uint8_t *)"can't\r\n");
        break;
    }
    l_linbleStateOld = l_linbleState;
}

uint8_t State_GetState(void)
{
    return SystemState;
}

void State_SetUpdateFlg(void)
{
    State_UpdateFlg = true;
}
