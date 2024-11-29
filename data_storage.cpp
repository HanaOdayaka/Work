#include "data_storage.h"
#include "i2c.h"
#include "settings.h"
#include "sampler.h"
#include "lualgo.h"
#include "slow_timer.h"
#include "keys.h"
#include "processing.h"
#include "laser_control.h"
#include "display_io.h"
#include "regs.h"
#include <iostream>
#include <chrono>
#include "sleep.h"
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <vector>
#include <iostream>
#include <sstream>
#include <iterator>
#include <thread>
#include <numeric>
#include <stdio.h>
#include <iomanip>
#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/transformed.hpp>

bool success = false;
std::string str ="\n";
const char* cstr = str.c_str();
DataStorageApi DataStorage;

DataStorageApi::DataStorageApi()
			:rawData{},
			 tempData{},
			 processedData{},
			 state{CaptureState::WAITING},
			 processingThreadData{},
			 prevKeyState{false},
			 onScreenSampleButtonState{false}
{
#if 0
	std::vector<float> samples(8192);

	for(int i = 0; i < 8192; i ++)
	{
		samples[i] = i;
		samples[i] %= 1000;

	}

	rawData.push_back(samples);	
	processedData.push_back(std::move(samples));
#endif
}

void DataStorageApi::sending()
{
	/*
	auto start1 = std::chrono::high_resolution_clock::now();
	
	std::string str = boost::algorithm::join( processingThreadData.processedData | boost::adaptors::transformed( static_cast<std::string(*)(int)>(std::to_string) ), "," );
	auto end11 = std::chrono::high_resolution_clock::now();
	double time_taken11 = std::chrono::duration_cast<std::chrono::nanoseconds>(end11 - start1).count();
	time_taken11 *= 1e-9;
	std::cout<<"Converting to str in ms: "<<time_taken11*1000<<std::setprecision(9)<<std::endl;
	str.append("n");
	//std::cout<<str<<std::endl;
	//output << 'n' << std::endl;				
	//std::string str = output.str();
	//std::cout<<str<<std::endl;
	//const char* cstr = str.c_str();
	auto cstr = &*str.begin();
	
    send(Settings.client_socket,cstr,strlen(cstr),0);
	auto end1 = std::chrono::high_resolution_clock::now();
	double time_taken1 = std::chrono::duration_cast<std::chrono::nanoseconds>(end1 - start1).count();
	time_taken1 *= 1e-9;
	std::cout<<"Sending in ms: "<<time_taken1*1000<<std::setprecision(9)<<std::endl;
	*/
	//char* result = reinterpret_cast<char*>(&processingThreadData.processedData);
	//std::cout<<processingThreadData.rawData[0]<<std::endl;
	//std::cout<<Settings.meanCount<<std::endl;
	//std::cout<<processingThreadData.rawData.size()<<std::endl;
	//processingThreadData.rawData.push_back(0);


	
	for(int i = 0; i < Settings.meanCount; i++)
	{
		for(int j =0 ; j < 32; j++)
		{
			std::vector<uint32_t> sub_vector(processingThreadData.rawData.begin() + i*8192 + j*256 , processingThreadData.rawData.begin() + i*8192 + (j+1)*256);
			if (j==31)
				sub_vector.push_back(4294967295);
			send(Settings.client_socket, sub_vector.data(), sizeof(sub_vector[0])*sub_vector.size(), 0);
		}
	}
	
	std::cout<<"End"<<std::endl;
	//send(Settings.client_socket,cstr,strlen(cstr),0);
}

void DataStorageApi::getting()
{
	auto start2 = std::chrono::high_resolution_clock::now();
	if(Sampler.GetSamples(processingThreadData.rawData))
	{
		if (Settings.captureMode2T)
		{
			std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
			start = std::chrono::high_resolution_clock::now();
			LaserControl.LaserOff();
			end = std::chrono::high_resolution_clock::now();
			Settings.time_ledoff =  std::chrono::duration<double>(end - start).count();
		}
		if(processingThreadData.rawData.size() == 0)
		{
			state = CaptureState::WAITING;
		}
		else
		{
			processingThreadData.ready = false;
			//if (Settings.state_laser_com == false)
			ProcessingApi::GetMean(processingThreadData);
			state = CaptureState::PROCESSING;	
		}
	}
	auto end2 = std::chrono::high_resolution_clock::now();
	double time_taken2 = std::chrono::duration_cast<std::chrono::nanoseconds>(end2 - start2).count();
	time_taken2 *= 1e-9;
	std::cout<<"Getting values in ms: "<<time_taken2*1000<<std::setprecision(9)<<std::endl;

}


void DataStorageApi::ProcessCapture()
{
	if (Settings.server_ip[0] != '0' && success == false)
	{
		printf("H", Settings.server_ip.c_str());
	

		success = true;
		printf("H", Settings.server_ip.c_str());
		//connect(Settings.client_socket, (struct sockaddr *)&server_address, sizeof(server_address));
	}

	uint32_t xy = rd32(REG_TOUCH_SCREEN_XY);
	uint16_t x = (xy & 0xFFFF0000) >> 16;
	uint16_t y = xy & 0xFFFF;

	bool key_state = ((I2CBus.GetKeyMask() & MEASUREMENT_KEY_MASK) != 0) ;
	//printf("start ProcessCapture\n");
	//  Дополнительная проверка клавиши "лазер" меню "настройки"
	// на случай, если в MainMenu что-то пойдёт не так
	//if(!I2CBus.GetKeyState(Key::KEY1))
	//	onScreenSampleButtonState = false;

	// Старт захвата по любой из 2х клавиш
	key_state |= onScreenSampleButtonState;

	if (Settings.state_laser_com)
	{
		key_state = Settings.state_laser_com;
		Settings.t4mod = true;
		Settings.captureMode2T = false;

	}

	LaserState laser_state = LaserControl.GetLaserState();
	
	if(LaserControl.IsLaserOn() && !Settings.t4mod && !Settings.captureMode2T)
	{
		LaserControl.LaserOff();
	}



	//если лазер включен и при т4 мод он должен быть выключен, то выключаем лазер
	if(LaserControl.IsLaserOn() && !Settings.captureMode2T && !Settings.t4mod)
	{
		LaserControl.LaserOff();
	}

	if (state ==  CaptureState::WAITING)
		{

			if(!key_state)
			{
				LaserControl.LaserOff();
				
			}
			else if((laser_state == LaserState::ON || laser_state == LaserState::READY) ||
					(laser_state == LaserState::NO_START && !prevKeyState))
			{
				if (Settings.captureMode2T)
				{
					Settings.g_start = std::chrono::high_resolution_clock::now();
					counter = 0;
				}
				Sampler.ConfigSample(Settings.meanCount, Settings.threshold, Settings.edgeDirection, Settings.filtMode); //-200 7700
				state = CaptureState::CAPTURING;

				StartTimer(Timer::CAPTURE, 2000);

				//режим работы 2т или 4т
				if (Settings.captureMode2T)
				{
					std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
					start = std::chrono::high_resolution_clock::now();
					if(!LaserControl.LaserOn())
					{
						StopTimer(Timer::CAPTURE);
						state = CaptureState::WAITING;
					}
					msleep(1);
					end = std::chrono::high_resolution_clock::now();
					Settings.time_ledon =  std::chrono::duration<double>(end - start).count();
				}
				else
				{	//если лазер был выключен при 4т мод, включаем, если был включен пропускаем этап включения 
					if(!LaserControl.IsLaserOn())
					{
						if(!LaserControl.LaserOn())
						{
							StopTimer(Timer::CAPTURE);
							state = CaptureState::WAITING;
						}
					}
				}
			}
		}

	if (state == CaptureState::CAPTURING)
		{
			std::thread th0(&DataStorageApi::getting, this);
			th0.join();
			if(GetTimerState(Timer::CAPTURE) == TimerState::EXPIRED)
			{
				StopTimer(Timer::CAPTURE);
				state = CaptureState::WAITING;
				LaserControl.LaserOff();
			}
		}

	if (state == CaptureState::PROCESSING)
		if(processingThreadData.ready)
		{

			if (Settings.state_laser_com && success == true)
			{
				std::thread th(&DataStorageApi::sending, this);
  				th.join();
			}

			if(LuAlgo.SetRawSignal(processingThreadData.processedData.data(), Settings.meanCount))
			{
				std::vector<float> processed_samples(PACKET_SAMPLE_COUNT);

				if(LuAlgo.GetSignal(processed_samples.data()))
				{
					//увеличение счтчика усреднения времени
					if (!Settings.captureMode2T)
					{
						if (counter == max_counter)
						{
							Settings.g_end = std::chrono::high_resolution_clock::now();
							//printf("Среднее время измерения в 4Т: %f seconds\n",std::chrono::duration<double>(Settings.g_end - Settings.g_start).count()/(max_counter+1));
							//printf("Средняя частота измерения  4Т: %f hz\n",1/(std::chrono::duration<double>(Settings.g_end - Settings.g_start).count()/(max_counter+1)));
							
							counter = 0;
							Settings.g_start = std::chrono::high_resolution_clock::now();
						
							
						}
						else
						{
							++counter;
						}
					}
					else
					{
						Settings.g_end = std::chrono::high_resolution_clock::now();
						//printf("Среднее время измерения без включения и выключения лазера: %f seconds\n",std::chrono::duration<double>(Settings.g_end - Settings.g_start).count()- Settings.time_ledoff - Settings.time_ledon);
						//printf("Средняя частота измерения без включения и выключения лазера: %f hz\n",1/(std::chrono::duration<double>(Settings.g_end - Settings.g_start).count()- Settings.time_ledoff - Settings.time_ledon));
					}

					if(rawData.size() >= Settings.GetHistoryCount())
						rawData.pop_front();

					if(processedData.size() >= Settings.GetHistoryCount())
						processedData.pop_front();

					std::vector<float> temp{};

					std::swap(temp, processingThreadData.processedData);

					rawData.push_back(std::move(temp));
					processedData.push_back(std::move(processed_samples));

					state = CaptureState::PROCESSED;
				}
				else
				{
					state = CaptureState::WAITING;
				}
			}
			else
			{
				state = CaptureState::WAITING;
			}

		}

	prevKeyState = key_state;
}

void DataStorageApi::ReprocessData()
{
	if(rawData.size() == 0 ||
	   state != CaptureState::WAITING)
		return;

	if(LuAlgo.SetRawSignal(rawData.back().data(), Settings.meanCount))
	{
		std::vector<float> processed_samples(PACKET_SAMPLE_COUNT);

		if(LuAlgo.GetSignal(processed_samples.data()))
		{
			processedData.pop_back();
			processedData.push_back(std::move(processed_samples));
			state = CaptureState::REPROCESSED;
		}
	}
}

