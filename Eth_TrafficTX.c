#include <MDR32F9Qx_port.h>
#include <MDR32F9Qx_rst_clk.h>
#include <MDR32F9Qx_eth.h>
#include <MDR32F9Qx_eeprom.h>

//	����������� �������� ��� �������� ������
#define FR_MAC_SIZE 		12												//	����� ��� ����� � ���������
#define FR_L_SIZE   		2													//  ����� ���� Lenth/Eth Type
#define FR_HEAD_SIZE   	(FR_MAC_SIZE + FR_L_SIZE) // 	����� ���������

// ������� ��� ���������� �������� � ������������ ���������� �������, 
// ������ ������������� � ������� ������� � 0�2010_0000 ��� ����������� ������ DMA � ������ FIFO
#define  MAX_ETH_TX_DATA_SIZE 1514 / 4
#define  MAX_ETH_RX_DATA_SIZE 1514 / 4
uint8_t  FrameTx[MAX_ETH_TX_DATA_SIZE] __attribute__((section("EXECUTABLE_MEMORY_SECTION"))) __attribute__ ((aligned (4)));
uint32_t FrameRx[MAX_ETH_RX_DATA_SIZE] __attribute__((section("EXECUTABLE_MEMORY_SECTION"))) __attribute__ ((aligned (4)));

//	MAC ����� ����������������
uint8_t  MAC_SRC [] = {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc};

//	��������� ��������� ������ � ������� ������
void ETH_TaskProcess(MDR_ETHERNET_TypeDef * ETHERNETx);
//	���������� ������� FrameTx ������� �������� �����
void Ethernet_FillFrameTX(uint32_t frameL);

//	������������ ���� �� HSE, 8��� �� ����-�����
void Clock_Init(void)
{
	/* Enable HSE (High Speed External) clock */
	RST_CLK_HSEconfig(RST_CLK_HSE_ON);
	while (RST_CLK_HSEstatus() != SUCCESS);

	/* Configures the CPU_PLL clock source */
	RST_CLK_CPU_PLLconfig(RST_CLK_CPU_PLLsrcHSEdiv1, RST_CLK_CPU_PLLmul16);

	/* Enables the CPU_PLL */
	RST_CLK_CPU_PLLcmd(ENABLE);
	while (RST_CLK_CPU_PLLstatus() == ERROR);

	/* Enables the RST_CLK_PCLK_EEPROM */
	RST_CLK_PCLKcmd(RST_CLK_PCLK_EEPROM, ENABLE);
	/* Sets the code latency value */
	EEPROM_SetLatency(EEPROM_Latency_5);

	/* Select the CPU_PLL output as input for CPU_C3_SEL */
	RST_CLK_CPU_PLLuse(ENABLE);
	/* Set CPUClk Prescaler */
	RST_CLK_CPUclkPrescaler(RST_CLK_CPUclkDIV1);

	/* Select the CPU clock source */
	RST_CLK_CPUclkSelection(RST_CLK_CPUclkCPU_C3);
}

//	������������� ����� Ethernet
void Ethernet_Init(void)
{	
	static ETH_InitTypeDef  ETH_InitStruct;
	volatile	uint32_t			ETH_Dilimiter;
	
	// ����� ������������ Ethernet
	ETH_ClockDeInit();
	
	//	��������� ���������� HSE2 = 25���
	RST_CLK_HSE2config(RST_CLK_HSE2_ON);
  while (RST_CLK_HSE2status() != SUCCESS);	
	
	// ������������ PHY �� HSE2 = 25���
	ETH_PHY_ClockConfig(ETH_PHY_CLOCK_SOURCE_HSE2, ETH_PHY_HCLKdiv1);

	// ��� ��������
	ETH_BRGInit(ETH_HCLKdiv1);

	// ��������� ������������ ����� MAC
	ETH_ClockCMD(ETH_CLK1, ENABLE);


	//	����� ��������� ����� MAC
	ETH_DeInit(MDR_ETHERNET1);

	//  ������������� �������� Ethernet �� ���������
	ETH_StructInit(&ETH_InitStruct);
	
	//	��������������� �������� PHY:
	//   - ���������� �������������, ���������� � �������� ��������
	ETH_InitStruct.ETH_PHY_Mode = ETH_PHY_MODE_AutoNegotiation;
	ETH_InitStruct.ETH_Transmitter_RST = SET;
	ETH_InitStruct.ETH_Receiver_RST = SET;
	
	//	����� ������ �������
	ETH_InitStruct.ETH_Buffer_Mode = ETH_BUFFER_MODE_LINEAR;	
	//ETH_InitStruct.ETH_Buffer_Mode = ETH_BUFFER_MODE_FIFO;	
	//ETH_InitStruct.ETH_Buffer_Mode = ETH_BUFFER_MODE_AUTOMATIC_CHANGE_POINTERS;

  // HASH - ���������� ��������� 
	ETH_InitStruct.ETH_Source_Addr_HASH_Filter = DISABLE;

	//	������� ��� ������ ����������������
	ETH_InitStruct.ETH_MAC_Address[2] = (MAC_SRC[5] << 8) | MAC_SRC[4];
	ETH_InitStruct.ETH_MAC_Address[1] = (MAC_SRC[3] << 8) | MAC_SRC[2];
	ETH_InitStruct.ETH_MAC_Address[0] = (MAC_SRC[1] << 8) | MAC_SRC[0];

	//	���������� ����� ������ �� ������ ��� ��������� � �����������
	ETH_InitStruct.ETH_Dilimiter = 0x1000;

	//	��������� ����� ������� ������ �� ���� �����, 
	//	����� �������� ������� ����� ��������
	ETH_InitStruct.ETH_Receive_All_Packets 			  = DISABLE;
	ETH_InitStruct.ETH_Short_Frames_Reception 		= ENABLE;
	ETH_InitStruct.ETH_Long_Frames_Reception 	    = DISABLE;
	ETH_InitStruct.ETH_Broadcast_Frames_Reception = DISABLE;
	ETH_InitStruct.ETH_Error_CRC_Frames_Reception = DISABLE;
	ETH_InitStruct.ETH_Control_Frames_Reception 	= DISABLE;
	ETH_InitStruct.ETH_Unicast_Frames_Reception 	= ENABLE;
	ETH_InitStruct.ETH_Source_Addr_HASH_Filter 	  = DISABLE;

	//	������������� ����� Ethernet
	ETH_Init(MDR_ETHERNET1, &ETH_InitStruct);

	// ������ ����� PHY
	ETH_PHYCmd(MDR_ETHERNET1, ENABLE);		
}

void Ethernet_Start(void)
{
	// ������ ����� Ethernet
	ETH_Start(MDR_ETHERNET1);
}

//	���� ��������� �������� �������
void Ethernet_ProcessLoop(void)
{
	while(1){
		 ETH_TaskProcess(MDR_ETHERNET1);
	}
}

//	��������� ��������� ������ � ������� ������
void ETH_TaskProcess(MDR_ETHERNET_TypeDef * ETHERNETx)
{
	//	���� ��������� ������ ������
	volatile ETH_StatusPacketReceptionTypeDef ETH_StatusPacketReceptionStruct;
	
	//	��������� ��� ������ � �������� �������
	uint8_t * ptr_inpFrame = (uint8_t *) &FrameRx[0];
	//	������� ��������� �� PC
	uint16_t frameL = 0;
	uint16_t frameCount = 0;
	//	���������� ����������
	uint32_t i;
	volatile uint32_t isTxBuffBusy = 0;

	//	��������� ������� � ������ ��������� ������ ��� ����������
	if(ETHERNETx->ETH_R_Head != ETHERNETx->ETH_R_Tail)
	{
		//	���������� �������� ������
		ETH_StatusPacketReceptionStruct.Status = ETH_ReceivedFrame(ETHERNETx, FrameRx);
		
		//	���������� ����� ��������� ������
		frameL = (uint16_t)((ptr_inpFrame[FR_HEAD_SIZE] << 8) | (ptr_inpFrame[FR_HEAD_SIZE + 1]));
		//  ���������� ���������� �������� �������, �������� ��� ������� ���� �����
		frameCount = (uint16_t)((ptr_inpFrame[FR_HEAD_SIZE + 2] << 8) | (ptr_inpFrame[FR_HEAD_SIZE + 3]));
		if (frameCount <= 0)
			frameCount = 1;		
		
		// 	���������� ������� FrameTx ������� ��������
		Ethernet_FillFrameTX(frameL);
		
		//	������� ������� � �����
		for (i = 0; i < frameCount; ++i)
		{
			//	��������� ������ �������� ������, � ������ ������ Payload
			//	FR_HEAD_SIZE + 4 - ��� ��������� + "���� ���������� ��������� ������"
			FrameTx[FR_HEAD_SIZE + 4] = (uint8_t)(i >> 8);
			FrameTx[FR_HEAD_SIZE + 5] = (uint8_t)(i & 0xFF);
			
			//  ������� ����������� ������ ����������� ����������
			//  ������ ������ ��������� ����� Delimeter � ����� 4�����
			//  ������������ ����� ������ ����� ���������� 1518 ����
			//	����� ������� ����������� �� �������� ���� ������� ���������� ��� ����������� ������ �������� ��������.			
			do {
				isTxBuffBusy = ETH_GetFlagStatus(ETHERNETx, ETH_MAC_FLAG_X_HALF) == SET;				
			}	
			while (isTxBuffBusy);

			//  ������� ������
			ETH_SendFrame(ETHERNETx, (uint32_t *) FrameTx, frameL);
		}
	}
}

//	���������� ������� FrameTx ������� �������� �����
void Ethernet_FillFrameTX(uint32_t frameL)
{
	uint32_t i = 0;
	//	����������� ����������� ������ � Payload
	uint32_t payloadL = frameL - FR_HEAD_SIZE;
	//	��������� �� �������� ����� ��� ����������� SrcMAC
	uint8_t * ptr_inpFrame = (uint8_t *) &FrameRx[0];
	//	��������� �� ����������� �����, 
	//  ��������� ����� "���� ���������� ��������� ������"
	uint8_t * ptr_TXFrame  = (uint8_t *) &FrameTx[4];

	//	������ "���� ���������� ��������� ������"
	//  ����������� ����� ������
	*(uint32_t *)&FrameTx[0] = frameL;
	
	//	�������� ����� PC �� �������� ������ � DestMAC
	ptr_TXFrame[0] 	= ptr_inpFrame[6];
	ptr_TXFrame[1] 	= ptr_inpFrame[7];
	ptr_TXFrame[2] 	= ptr_inpFrame[8];
	ptr_TXFrame[3] 	= ptr_inpFrame[9];
	ptr_TXFrame[4] 	= ptr_inpFrame[10];
	ptr_TXFrame[5] 	= ptr_inpFrame[11];		
	
	//	��������� SrcMAC
	ptr_TXFrame[6] 	= MAC_SRC[0];
	ptr_TXFrame[7] 	= MAC_SRC[1];
	ptr_TXFrame[8] 	= MAC_SRC[2];
	ptr_TXFrame[9] 	= MAC_SRC[3];
	ptr_TXFrame[10] = MAC_SRC[4];
	ptr_TXFrame[11] = MAC_SRC[5];	

  // ��������� ���� Length �������� Payload  � ������
	ptr_TXFrame[12] = (uint8_t)(payloadL >> 8);
	ptr_TXFrame[13] = (uint8_t)(payloadL & 0xFF);	

	//	��������� Payload ����������, �������� ���������
	for (i = 0; i < payloadL; ++i)
		ptr_TXFrame[FR_HEAD_SIZE + i] = i;	
}


