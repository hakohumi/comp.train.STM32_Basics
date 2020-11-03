# flowchart

``` mermaid
graph TD;

start --> ble_state{"BLEの状態は？"}

ble_state -- command --> flg_BTC{"BTCのフラグは<br>ONか？"}


UART1["LINBLE<br>(UART1)"] --> micon["マイコン"]
micon --> if_out{out?}
if_out --> uart1["LINBLE"]
if_out --> uart2["PC"]

```

``` mermaid

graph TD;

UART2["PC<br>(UART2)"] --> micon



```
