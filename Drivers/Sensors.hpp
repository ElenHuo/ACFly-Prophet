#pragma once

#include "Basic.hpp"
#include "vector3.hpp"
#include "map_projection.hpp"

/*IMU*/
	struct IMUConfig
	{
		double offset[3];
		double scale[3];
		double STTemperature;
		double TemperatureCoefficient[3];
	} __PACKED;
	#define IMUConfigLength 10

	/*IMU����������*/
		#define IMU_Sensors_Count 3
		
		extern const uint8_t External_Magnetometer_Index;
		extern const uint8_t Internal_Magnetometer_Index;
		struct IMU_Sensor
		{
			SName name;	//����������
			TIME last_update_time;	//�ϴθ���ʱ��
			double sample_time;	//����ʱ��
			bool calibrated;	//�Ƿ���У׼
			bool have_temperature;	//�Ƿ����¶�����
			
			double temperature;	//�¶�
			double sensitivity;	//�����ȣ�ԭʼ����->ʵ�ʵ�λ ���ݣ�rad/s ���ٶȣ�cm/s^2 �ų���gauss��
			
			bool data_error;	//�����Ƿ���󣨱����̣�
			vector3<int32_t> data_raw;	//ԭʼ����
			vector3<double> data_rawTC;	//�¶Ȳ������ԭʼ����
			vector3<double> data;	//ʵ�ʵ�λ����
		};		
	/*IMU����������*/
		
	/*IMU��������ȡ����*/
		bool GetAccelerometer( uint8_t index, IMU_Sensor* sensor, double TIMEOUT = -1 );
		bool GetGyroscope( uint8_t index, IMU_Sensor* sensor, double TIMEOUT = -1 );
		bool GetMagnetometer( uint8_t index, IMU_Sensor* sensor, double TIMEOUT = -1 );
	/*IMU��������ȡ����*/
/*IMU*/
	
/*λ�ô�����*/
	/*λ�ô���������*/
		#define Position_Sensors_Count 16
		
		extern const uint8_t default_ultrasonic_sensor_index;
		extern const uint8_t default_optical_flow_index;
		extern const uint8_t internal_baro_sensor_index;
		extern const uint8_t default_gps_sensor_index;
		
		enum Position_Sensor_Type
		{
			Position_Sensor_Type_GlobalPositioning = 0 ,	//ȫ��λ����γ�ȶ�λ����GPS��
			Position_Sensor_Type_RelativePositioning = 1 ,	//��Զ�λ������ѹ�ƣ������ﲻ��ı䣩
			Position_Sensor_Type_RangePositioning = 2 ,	//���붨λ����ඨλ���糬��������������ܻ�仯��
		};
		enum Position_Sensor_DataType
		{
			//s-λ�� v-�ٶ�
			//��sv_xy��ʾ�ô��������У�λ���ٶȵ�xy����
			Position_Sensor_DataType_s_xy = 0 ,
			Position_Sensor_DataType_s_z = 1 ,
			Position_Sensor_DataType_s_xyz = 2  ,
			
			Position_Sensor_DataType_v_xy = 8  ,
			Position_Sensor_DataType_v_z = 9  ,
			Position_Sensor_DataType_v_xyz = 10 ,
			
			Position_Sensor_DataType_sv_xy = 16  ,
			Position_Sensor_DataType_sv_z = 17  ,
			Position_Sensor_DataType_sv_xyz = 18  ,
		};
		enum Position_Sensor_frame
		{
			Position_Sensor_frame_ENU = 0 ,	//�ٶ�������ENU����ϵ��
			Position_Sensor_frame_BodyHeading = 1 ,	//�ٶ�����xΪ��ͷ���������ƽ�У���yΪ�����ͷ�󷽣������ƽ�У���zΪ�Ϸ�
		};
		struct Position_Sensor
		{
			bool available;	//�������Ƿ����
			TIME last_update_time;	//�ϴθ���ʱ��
			TIME available_status_update_time;	//�����������źŸ���ʱ��
			double delay;	//��������ʱ
			double xy_trustD;	//���ζ� 0������ζ�
			double z_trustD;	//���ζ� 0������ζ�
			double sample_time;	//����ʱ��

			Position_Sensor_Type sensor_type;	//���������ͣ���ö��ע�ͣ�
			Position_Sensor_DataType sensor_DataType;	//�������������ͣ���ö��ע�ͣ�
			Position_Sensor_frame velocity_data_frame;	//�ٶ���������ϵ����ö��ע�ͣ�
			
			vector3<double> position_Global;	//��γ��
			vector3<double> position;	//λ��(cm)
			vector3<double> velocity;	//�ٶ�(cm/s)
			
			Map_Projection mp;
		};
	/*λ�ô���������*/
	
	/*λ�ô�������ȡ����*/
		bool GetPositionSensor( uint8_t index, Position_Sensor* result_sensor, double TIMEOUT = -1 );
	/*λ�ô�������ȡ����*/
/*λ�ô�����*/