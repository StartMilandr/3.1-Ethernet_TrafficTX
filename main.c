#include "Eth_TrafficTX.h"

int main()
{
	//	������������
	Clock_Init();
		
	//  ������������� � ������
	Ethernet_Init();
  Ethernet_Start();
	//	����������� ����
  Ethernet_ProcessLoop();	
}	


