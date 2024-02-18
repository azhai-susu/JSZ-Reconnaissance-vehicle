
#include "page_imu.h"
#include "app_main.h"
#include "stdio.h"
#include <stdlib.h>
#include <string.h>

#include <esp_system.h>
#include "esp_log.h"
#include "mpu6050.h"

#define TAG "mpu6050"


uint8_t imu_en = 0;
uint8_t mpu_ready = 0;

_xyz_f_st Gyro_deg = {0.0, 0.0, 0.0}, Acc_mmss = {0.0, 0.0, 0.0}, Gyro = {0.0, 0.0, 0.0}, Acc = {0.0, 0.0, 0.0};

void Imu_Task(void)
{
	imu_en = 1;
	vTaskDelay(100);
	bsp_i2c_master_init();
	vTaskDelay(100);
	mpu6050_init();
	vTaskDelay(100);
	// /* 入口处检测一次 */
	ESP_LOGI("Imu_Task", "Imu_Task uxHighWaterMark = %d", uxTaskGetStackHighWaterMark(NULL));
	
	ESP_LOGI(TAG, "connection:%d", mpu6050_test_connection());
	ESP_LOGI(TAG, "tag:%c", *mpu6050_get_tag());
	ESP_LOGI(TAG, "aux_vddio_level:%d", mpu6050_get_aux_vddio_level());
	ESP_LOGI(TAG, "rate divider:%d", mpu6050_get_rate());
	ESP_LOGI(TAG, "external_frame_sync:%d", mpu6050_get_external_frame_sync());

	ESP_LOGI(TAG, "dlpf_mode:%d", mpu6050_get_dlpf_mode());
	ESP_LOGI(TAG, "full_scale_gyro_range:%d", mpu6050_get_full_scale_gyro_range());
	//printf("\n mpu6050_test_connection:%d\n", mpu6050_test_connection());
	mpu_ready = mpu6050_test_connection();
	// 定义变量
	mpu6050_acceleration_t accel = {
		.accel_x = 0,
		.accel_y = 0,
		.accel_z = 0,
	};
	mpu6050_rotation_t gyro = {
		.gyro_x = 0,
		.gyro_y = 0,
		.gyro_z = 0,
	};

	imu_en = 1;
	//while (1)
	if(0)
	{
		if (imu_en)
		{

			mpu6050_get_motion(&accel, &gyro);
			//printf("ax:%d;ay:%d;az:%d", accel.accel_x, accel.accel_y, accel.accel_z);
			Gyro_deg.x = gyro.gyro_x * 0.0610361f; //  /65535 * 4000; +-2000度
			Gyro_deg.y = gyro.gyro_y * 0.0610361f;
			Gyro_deg.z = gyro.gyro_z * 0.0610361f;

			/*加速度计转换到毫米每平方秒*/
			Acc_mmss.x = accel.accel_x * 2.392615f; //   /65535 * 16*9800; +-8G
			Acc_mmss.y = accel.accel_y * 2.392615f;
			Acc_mmss.z = accel.accel_z * 2.392615f;
			printf("ax:%7.2f;ay:%7.2f;az:%7.2f", Acc_mmss.x, Acc_mmss.y, Acc_mmss.z);
			printf("gx:%7.2f;gy:%7.2f;gz:%7.2f", Gyro_deg.x, Gyro_deg.y, Gyro_deg.z);
			IMU_update(0.005f, &Gyro_deg, &Acc_mmss, &imu_data); //姿态解算

			printf("pitch:%3.2f, yaw:%3.2f, roll:%3.2f\n", imu_data.pit, imu_data.yaw, imu_data.rol);
			//lv_chart_set_next(chart_imu, series, imu_data.pit);
			//lv_chart_set_next(chart_imu, series1, imu_data.rol);
			// vTaskDelayUntil(&xLastWakeTime, (2 / portTICK_RATE_MS));
			vTaskDelay(5);
		}
		else
		{
			bsp_i2c_master_deinit();
			vTaskDelete(NULL);
		}
		vTaskDelay(10);
	}
}
