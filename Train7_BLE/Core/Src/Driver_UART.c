
#include "main.h"

/*

 UARTはデバイスに1つ以上存在するため、LINBLEやPCとの通信ごとに共通できる部分は共通したい
 
 共通化するべきもの
 ・UARTの送受信(send, receive)
 ・受信バッファ処理(write, read)
 ・
 
 〇大まかなくくり

 受信割り込み
 receive -> write -> buff
 
 受信データ読み込み
 buff -> read -> マイコン

 送信
 マイコン -> send

 〇機能
 ・（受信）受信した1バイトのデータをバッファへ格納
 void SetBuffer()
 ・（送信）指定したUARTでデータとデータサイズを指定して、送信
 void SendUART(uart, data, datasize)
 ・（バッファリード）バッファに入っているデータを読みだす
uint8 ReadBuff(uart)
・バッファが空かどうか
bool IsEmptyBuff(uart)

 〇クラス同士の繋がり
 例：受信：LINBLE側からデータが入ってきた場合
 UARTから1バイトデータが入ってくると、割り込みが入る
 割り込み内で、どのUARTかを判断する
 そのUARTごとのバッファの最後尾に受信したデータを格納する

 例：送信：状態が変化して文字を送信する場合
 状態が変化して、イベントハンドラーが呼ばれる
 イベントハンドラーで、「状態が変化した」と文字列を用意し、
 表示レイアウトを管理する別の関数を通り、
 最終的に送信するデータ(文字配列、文字数)がsend関数まで渡され、UARTで送信を行う
  

 
 例：受信データ読み込み：区切り文字がある場合、
 例：受信データ読み込み：区切り文字がない場合
 例：受信データ読み込み：ターミナルで入力された文字列をバッファから取り出したい
 例：受信データ読み込み： 

 受信データは、1バイトのデータ、コマンド（文字列の終わりに区切り文字がある）
 

*/

/*
 受信バッファは、各UARTのポートごとに確保する
 マイコン初期化時に、バッファを追加する
 


// 指定したUARTにデータを送る
int8_t DriverUART_sendDATA(UART_HandleTypeDef *i_huartPtr, uint8_t *i_str);