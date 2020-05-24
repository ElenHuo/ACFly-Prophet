
#include "Basic.hpp"
#include "ctrl_Main.hpp"
#include "ctrl_Attitude.hpp"
#include "ctrl_Position.hpp"
#include "MeasurementSystem.hpp"
#include "ControlSystem.hpp"
#include "drv_ADC.hpp"
#include "Parameters.hpp"

#include "FreeRTOS.h"
#include "semphr.h"

/*����ϵͳ������*/
	static SemaphoreHandle_t CtrlMutex = xSemaphoreCreateRecursiveMutex();
	bool LockCtrl( double TIMEOUT )
	{
		TickType_t TIMTOUT_Ticks;
		if( TIMEOUT >= 0 )
			TIMTOUT_Ticks = TIMEOUT*configTICK_RATE_HZ;
		else
			TIMTOUT_Ticks = portMAX_DELAY;
		if( xSemaphoreTakeRecursive( CtrlMutex, TIMTOUT_Ticks ) == pdTRUE )
			return true;
		else
			return false;
	}
	void UnlockCtrl()
	{
		xSemaphoreGiveRecursive(CtrlMutex);
	}
/*����ϵͳ������*/
	
/*����ϵͳ��ȫʱ��*/
	TIME last_XYCtrlTime;
	TIME last_ZCtrlTime;
	//��ȡ�������ϴο���ʱ��
	//̫�ò����п��ƽ�����MSafeģʽ
	bool get_lastXYCtrlTime( TIME* t, double TIMEOUT )
	{
		if( LockCtrl(TIMEOUT) )
		{
			*t = last_XYCtrlTime;
			UnlockCtrl();
			return true;
		}
		return false;
	}
	bool get_lastZCtrlTime( TIME* t, double TIMEOUT )
	{
		if( LockCtrl(TIMEOUT) )
		{
			*t = last_ZCtrlTime;
			UnlockCtrl();
			return true;
		}
		return false;
	}
	
	//���������ϴο���ʱ����Ϊ������
	//ǿ�ƽ���MSafeģʽ
	//MSafeģʽ���޷��ر�λ�ÿ�����
	//ͬʱ����XYZλ�ÿ��ƿ��˳�MSafe
	//��ˮƽ���Ʋ�����ʱ���ƽǶȣ�
	bool enter_MSafe( double TIMEOUT )
	{
		if( LockCtrl(TIMEOUT) )
		{
			last_XYCtrlTime.set_invalid();
			last_ZCtrlTime.set_invalid();
			UnlockCtrl();
			return true;
		}
		return false;
	}
	
	//ǿ��Safe����
	bool ForceMSafeCtrl = false;
	
	//��ȡ�Ƿ������MSafeģʽ
	bool is_MSafeCtrl()
	{
		return ForceMSafeCtrl;
	}
/*����ϵͳ��ȫʱ��*/

/*��ѹ������������*/
	//�˲���ѹ��V��
	static float MainBatteryVoltage_filted = 0;
	//��ʹ�ù��ģ�W*h��
	static float MainBatteryPowerUsage = 0;
	//�˲����ʣ�W��
	static float MainBatteryPower_filted = 0;
	
	float get_MainBatteryVoltage() { return Get_MainBaterry_Voltage(); }
	float get_MainBatteryVoltage_filted() { return MainBatteryVoltage_filted; }
	float get_MainBatteryPowerUsage() { return MainBatteryPowerUsage; }
	float get_MainBatteryPower_filted() { return MainBatteryPower_filted; }
	float get_MainBatteryCurrent() { return Get_MainBaterry_Current(); }
	float get_CPUTemerature() { return Get_Temperature(); }
	void get_MainBatteryInf( float* Volt, float* Current, float* PowerUsage, float* Power_filted, float* RMPercent )
	{		
		float volt = Get_MainBaterry_Voltage();
		if( Volt != 0 )
			*Volt = volt;
		if( *Current != 0 )
			*Current = Get_MainBaterry_Current();
		if( *PowerUsage != 0 )
			*PowerUsage = MainBatteryPowerUsage;
		if( *Power_filted != 0 )
			*Power_filted = MainBatteryPower_filted;
		if( *RMPercent != 0 )
		{
			BatteryCfg cfg;
			if( ReadParamGroup( "Battery", (uint64_t*)&cfg, 0 ) == PR_OK )
			{
				if( volt > cfg.STVoltage[0] + cfg.VoltP10[0] )
				{
					*RMPercent = 100;
					return;
				}
				for( int8_t i = 10; i >= 1 ; --i )
				{
					float P1 = cfg.STVoltage[0] + (&(cfg.VoltP0[0]))[(i-1)*2];					
					if( volt > P1 )
					{
						float P2 = cfg.STVoltage[0] + (&(cfg.VoltP0[0]))[(i-0)*2];
						*RMPercent = (i-1)*10 + 10*( volt - P1 ) / ( P2 - P1 );
						return;
					}
				}
				*RMPercent = 0;
			}
			else
				*RMPercent = 100;
		}
	}
/*��ѹ������������*/
	
static void ControlSystem_Task(void* pvParameters)
{
	//�ȴ�������ʼ�����
	while( getInitializationCompleted() == false )
		os_delay(0.1);
	
	//�ȴ���̬����ϵͳ׼�����
	while( get_Attitude_MSStatus() != MS_Ready )
		os_delay(0.1);
	
	//�ȴ�λ�ý���ϵͳ׼�����
	while( get_Altitude_MSStatus() != MS_Ready )
		os_delay(0.1);
	
	//���ó�ʼ��ѹ
	uint16_t VoltMeas_counter = 0;
	MainBatteryVoltage_filted = Get_MainBaterry_Voltage();
	//׼ȷ������ʱ
	static TickType_t xLastWakeTime;
	xLastWakeTime = xTaskGetTickCount();
	while(1)
	{
		//400hz
		vTaskDelayUntil( &xLastWakeTime, 2 );
		
		if( LockCtrl(0.02) )
		{
			ctrl_Position();
			ctrl_Attitude();
			
			UnlockCtrl();
		}
		
		/*��ѹ�����˲�*/
			if( ++VoltMeas_counter >= 0.5*CtrlRateHz )
			{
				VoltMeas_counter = 0;
				
				//��ѹ�˲�
				float MBVoltage = Get_MainBaterry_Voltage();
				float MBCurrent = Get_MainBaterry_Current();
				MainBatteryVoltage_filted += 0.1f*0.5f * ( MBVoltage - MainBatteryVoltage_filted );
				//�����˲�
				MainBatteryPower_filted += 0.1f*0.5f * ( MBVoltage*MBCurrent - MainBatteryPower_filted );
				
				//��ʹ�ù��ģ�W*h��
				MainBatteryPowerUsage += (0.5f / 60.0f) * MBVoltage*MBCurrent;
			}
		/*��ѹ�����˲�*/
	}
}

void init_ControlSystem()
{
	init_Ctrl_Attitude();
	init_Ctrl_Position();
	xTaskCreate( ControlSystem_Task, "ControlSystem", 4096, NULL, SysPriority_ControlSystem, NULL);
	
	/*ע���ز���*/
		BatteryCfg initial_cfg;
		initial_cfg.STVoltage[0] = 11.6;
		initial_cfg.VoltMKp[0] = 21;
		initial_cfg.CurrentMKp[0] = 0.02;
		initial_cfg.Capacity[0] = 300;
		initial_cfg.VoltP0[0] = -1.0;
		initial_cfg.VoltP1[0] = -0.6;
		initial_cfg.VoltP2[0] = -0.4;
		initial_cfg.VoltP3[0] = -0.2;
		initial_cfg.VoltP4[0] = -0.0;
		initial_cfg.VoltP5[0] = +0.1;
		initial_cfg.VoltP6[0] = +0.2;
		initial_cfg.VoltP7[0] = +0.3;
		initial_cfg.VoltP8[0] = +0.4;
		initial_cfg.VoltP9[0] = +0.5;
		initial_cfg.VoltP10[0] = +0.6;
	
		MAV_PARAM_TYPE param_types[] = {
			MAV_PARAM_TYPE_REAL32 ,	//STVoltage
			MAV_PARAM_TYPE_REAL32 ,	//VoltMKp
			MAV_PARAM_TYPE_REAL32 ,	//CurrentMKp
			MAV_PARAM_TYPE_REAL32 ,	//Capacity
			MAV_PARAM_TYPE_REAL32 ,	//VoltP0
			MAV_PARAM_TYPE_REAL32 ,	//VoltP1
			MAV_PARAM_TYPE_REAL32 ,	//VoltP2
			MAV_PARAM_TYPE_REAL32 ,	//VoltP3
			MAV_PARAM_TYPE_REAL32 ,	//VoltP4
			MAV_PARAM_TYPE_REAL32 ,	//VoltP5
			MAV_PARAM_TYPE_REAL32 ,	//VoltP6
			MAV_PARAM_TYPE_REAL32 ,	//VoltP7
			MAV_PARAM_TYPE_REAL32 ,	//VoltP8
			MAV_PARAM_TYPE_REAL32 ,	//VoltP9
			MAV_PARAM_TYPE_REAL32 ,	//VoltP10
		};
		SName param_names[] = {
			"Bat_STVoltage" ,	//UAV Type
			"Bat_VoltMKp" ,	//VoltMKp
			"Bat_CurrentMKp" ,	//CurrentMKp
			"Bat_Capacity" ,	//Capacity
			"Bat_VoltP0" ,	//VoltP0
			"Bat_VoltP1" ,	//VoltP1
			"Bat_VoltP2" ,	//VoltP2
			"Bat_VoltP3" ,	//VoltP3
			"Bat_VoltP4" ,	//VoltP4
			"Bat_VoltP5" ,	//VoltP5
			"Bat_VoltP6" ,	//VoltP6
			"Bat_VoltP7" ,	//VoltP7
			"Bat_VoltP8" ,	//VoltP8
			"Bat_VoltP9" ,	//VoltP9
			"Bat_VoltP10" ,	//VoltP10
		};
		ParamGroupRegister( "Battery", 1, 15, param_types, param_names, (uint64_t*)&initial_cfg );
	/*ע���ز���*/
}