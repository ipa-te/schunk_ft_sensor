/*
 /*!
 *****************************************************************
 * \file
 *
 * \note
 * Copyright (c) 2017 \n
 * Fraunhofer Institute for Manufacturing Engineering
 * and Automation (IPA) \n\n
 *
 *****************************************************************
 *
 * \note
 * ROS stack name: schunk_ft_sensor
 * \note
 * ROS package name: schunk_ft_sensor
 *
 * \author
 * Author: Turan Elchuev email: turan.elchuev@ipa.fraunhofer.de
 *
 * \date Date of creation: October, 2017
 *
 * \brief
 *
 * *****************************************************************
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer. \n
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution. \n
 * - Neither the name of the Fraunhofer Institute for Manufacturing
 * Engineering and Automation (IPA) nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission. \n
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License LGPL as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License LGPL for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License LGPL along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 *
 ******************************************************************/

#ifndef SCHUNK_FT_SENSOR_INCLUDE_SCHUNK_FT_SENSOR_H_
#define SCHUNK_FT_SENSOR_INCLUDE_SCHUNK_FT_SENSOR_H_

#include "ros/ros.h"
#include <socketcan_interface/socketcan.h>
#include <socketcan_interface/threading.h>
#include <geometry_msgs/Wrench.h>
#include <std_msgs/String.h>
#include <std_srvs/Empty.h>

class Version
{
	public:
		bool received = false;
		unsigned char major = 0, minor = 0;
		unsigned short build = 0;

		bool standardCpTCpF()
		{
			return major < 3 || (major == 3 && minor < 7);
		}

		std::string getVersionStr()
		{
			std::ostringstream ss;
			ss << (int)major << "." << (int)minor << "." << (int)build;
			return ss.str();
		}
};

enum message_types: unsigned int
{
	/*
	 * 2-digit hex number
	 * 1st digit - opcode
	 * 2nd digit - required number of bytes in data (dlc)
	 */
	INVALID = 0xFF,
	Read_SG_Data = 0x00,
	SG_Data_Packet_1 = 0x08,
	SG_Data_Packet_2 = 0x16,
	Read_Matrix = 0x21,
	Matrix_Packet_1 = 0x28,
	Matrix_Packet_2 = 0x38,
	Matrix_Packet_3 = 0x48,
	Read_FT_Serial_Number = 0x50,
	Serial_Number = 0x58,
	Active_Calibration = 0x61,
	Read_Counts_Per_Unit = 0x70,
	Counts_Per_Unit = 0x78,
	Read_Unit_Codes = 0x80,
	Unit_Codes = 0x82,
	Reset = 0xC0,
	Read_Firmware_Version = 0xF0,
	Firmware_Version = 0xF4
};

enum error_types: unsigned short
{
	Watchdog_Reset = (1 << 0),
	DAC_ADC_Too_High = (1 << 1),
	DAC_ADC_Too_Low = (1 << 2),
	Artificial_Ground_Out_of_Range = (1 << 3),
	Power_Supply_Too_High = (1 << 4),
	Power_Supply_Too_Low = (1 << 5),
	Bad_Active_Calibration = (1 << 6),
	EEPROM_Failure = (1 << 7),
	Config_Invalid = (1 << 8),
	Sensor_Temp_Too_High = (1 << 11),
	Sensor_Temp_Too_Low = (1 << 12),
	CANbus_Error = (1 << 14),
	ANY = (1 << 15)
};

class SchunkFTSensorInterface
{
	protected:

		bool debug = false;

		ros::NodeHandle nh;
		ros::Publisher sensorTopic, failureTopic;
		ros::ServiceServer resetBiasSS;

		boost::shared_ptr<can::DriverInterface> driver;

		std::string can_device;
		int node_id = 128;

		Version ver;

		unsigned char calibration = 0;

		short	sg[6] = {0},
				bias[6] = {0};
		volatile int sample_sum[6] = {0}, sample_cnt = 0;
		int sample_count = 20;

		float matrix[6][6];

		unsigned int CpF = 1000000, CpT = 1000000;

		double 	silence_limit = 0.1; // sec.

		bool driver_initialized = false;
		bool average = true;

		volatile bool	calibration_message_received = false,
						calibration_successfully_set = false,
						counts_per_unit_received = false,
						matrix_data_obtained[6] = {false},
						bias_obtained = false,
						sg_data_received = true,
						sensor_running = false;

		volatile double sg_data_request_timstamp = 0;

		unsigned short status = 0;

		can::Frame f_data_request;

		can::CommInterface::FrameListener::Ptr frame_listener;
		can::StateInterface::StateListener::Ptr state_listener;

		message_types getType(const can::Frame &f);
		can::Frame makeFrame(message_types type);
		can::Frame makeFrame(message_types type, unsigned char b);
		can::Frame makeFrame(message_types type, boost::array<unsigned char, 8> data);

		void frameCB(const can::Frame &f);
		void stateCB(const can::State & s);
		void requestSGDataThread();
		void extractRawSGData(const can::Frame &f);
		void extractMatrix(const can::Frame &f);
		void extractFirmwareVersion(const can::Frame &f);
		void extractCountsPerUnit(const can::Frame &f);
		void checkCalibration(const can::Frame &f);
		void averageRawSGData(short *data);
		void biasRawSGData(short *data);
		void convertToFT(short *data);

		bool initParams();
		bool initDriver();
		bool setCalibration();
		bool requestFirmwareVersion();
		bool requestCountsPerUnits();
		bool requestMatrix();
		bool initRos();

		bool failure(std::string mes);
		bool checkStatus();

		bool err(std::string mes);
	public:

		SchunkFTSensorInterface()
		{

		}

		~SchunkFTSensorInterface()
		{
			finalize();
		}

		bool initialize();
		bool finalize();
		void resetBias();
		bool resetBiasSrv(std_srvs::Empty::Request &req, std_srvs::Empty::Response &resp);
		void runSensor();
		void stopSensor();

};

#endif /* SCHUNK_FT_SENSOR_INCLUDE_SCHUNK_FT_SENSOR_H_ */
