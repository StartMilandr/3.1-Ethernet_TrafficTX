#include "Eth_TrafficTX.h"

int main()
{
  //	Тактирование
  Clock_Init();
	
  //  Инициализация и запуск
  Ethernet_Init();
  Ethernet_Start();
  //	Бесконечный цикл
  Ethernet_ProcessLoop();	
}
