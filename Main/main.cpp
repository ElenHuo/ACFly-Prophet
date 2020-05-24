#include "Basic.hpp"
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "drv_Main.hpp"
#include "MS_Main.hpp"
#include "Parameters.hpp"
#include "FlashIO.h"
#include "Modes.hpp"
#include "MSafe.hpp"
#include "ctrl_Main.hpp"
#include "drv_PWMOut.hpp"
#include "AC_Math.hpp"
#include "Missions.hpp"

#include "debug.hpp"

#if 1 //���û����Σ�����Ҫ��targetѡ����ѡ��ʹ��USE microLIB

	__asm(".global __use_no_semihosting\n\t") ;//ע�ͱ���, ����1
	extern "C"
	{
//		struct __FILE {
//		int handle;
//		};
//		std::FILE __stdout;

		void _sys_exit(int x)
		{
			x = x;
		}

		//__use_no_semihosting was requested, but _ttywrch was referenced, �������º���, ����2
		void _ttywrch(int ch)
		{
			ch = ch;
		}
		
		char *_sys_command_string(char *cmd, int len)
		{
				return 0;
		}
 
	}
#endif
	


void DriverInit_task(void* pvParameters)
{
	//��ʼ���豸����
	init_drv_Main();
	//��ʼ������ϵͳ
	init_MS_Main();
	init_Debug();
	init_Modes();
	init_MSafe();
	init_ControlSystem();	
	
	//��ɳ�ʼ��
	//��ɺ����ٽ��г�ʼ������
	while( getInitializationCompleted() == false )
	{
		setInitializationCompleted();
		os_delay(0.1);
	}

	//���������ʼ��
	init_Missions();
	
	//ɾ��������
	vTaskDelete(0);
}
	
int main(void)
{	
	//��ʼ��оƬʱ��
	//ʱ���׼�Ȼ�������
  init_Basic();
			
	//������ʼ�����񲢽������������
	xTaskCreate( DriverInit_task , "Init" ,8192,NULL,3,NULL);
	vTaskStartScheduler();
	while(1);
}

extern "C" void HardFault_Handler()
{
	//�����ж������������
	PWM_PullDownAll();
}

extern "C" void vApplicationStackOverflowHook( TaskHandle_t xTask, signed char *pcTaskName )
{
	static char StackOvTaskName[20];
	strcpy( StackOvTaskName, (char*)pcTaskName );
}



