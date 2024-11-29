#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <list>
#include <memory>

#include "draw_api.h"
#include "draw_coproc_api.h"
#include "regs.h"
#include "font.h"
#include "image.h"
#include "display_io.h"
#include "sleep.h"
#include "spi.h"
#include "i2c.h"

#include "chart_screen_a.h"
#include "chart_screen_a2.h"
#include "chart_screen_b.h"
#include "keyboard.h"

#include "image_manager.h"
#include "slow_timer.h"

#include "laser_driver.h"
#include "processing.h"
#include "settings.h"
#include "sampler.h"
#include "time_ms.h"

#include "config_parser.h"
#include "lualgo.h"
#include "data_storage.h"
#include "laser_control.h"
#include "file_io.h"
#include "file_manager.h"
#include <cmath>
#include "snapshot.h"
#include "pc.h"
#include <jsoncpp/json/json.h>
#include <fstream>
#include "image_manager.h"
#include <iostream>
#include <chrono>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <iomanip>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <cstring>
#include <jsoncpp/json/json.h>
#include <fstream>
#include <pstream.h>
#include <string>
#include <iterator>


int client_socket;
sockaddr_in server_address;
float off;
float raw;
float scl;
float temp;
int ip_time = 0;
std::string temp_com;

std::string exect(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

int previous_meanCount = 0;
int main(int argc, char **)
{

#if 0

	std::vector<ConfigRecord> temp_config{ParseConfigRecord(
		std::string(
			"{"
			"\"<идентификатор ВЭУ 1>\": {\"type\": \"list\",\"name\": \"abc\",\"values\": [\"abc1\",\"def1\",\"ghi1\"],\"default\":\"def1\"},"
			"\"<идентификатор ВЭУ 2>\": {\"type\": \"integer\",\"name\": \"def\",\"minimum\": \"10\",\"maximum\": \"20\",\"increment\": \"2\",\"default\":\"12\"},"
			"\"<идентификатор ВЭУ 3>\": {\"type\": \"real\",\"name\": \"ghi\nм/с\",\"minimum\": \"100\",\"maximum\": \"120.5\",\"increment\": \".5\",\"default\":\"101\"}"
			"}"
		)
	)};



#endif

	FileManager.ExecSystemCommand("ethtool -s eth0 speed 100 duplex full autoneg off");

#if 1

	if(!I2CBus.Init())
	{
        printf("Initialization of I2C failed. Error: %s\n", strerror(errno));
        return -1;
	}

	//msleep(3000);

	if(!Sampler.Init())
	{
        printf("Initialization of Sampler failed. Error: %s\n", strerror(errno));
        return -1;
	}

	if(!I2CBus.Init())
	{
        printf("Initialization of I2C failed. Error: %s\n", strerror(errno));
        return -1;
	}

    if(!DR_Init()){
        printf("Initialization of SPI failed. Error: %s\n", strerror(errno));
        return -1;
    }

    I2CBus.SetOutputState(ControlOutput::LASER_POWER, true);
    msleep(100);

    if(!LaserDriver.Init())
    {
    	printf("LaserDriver init failed\n");
    	return -1;
    }
#endif
    Settings.Init();
	
	std::ifstream txtlist; 
	std::string path{"/LUS_software/gui/mat/.json"};
	std::ifstream material_file;
	Json::Value materials;

	txtlist.open("/LUS_software/gui/mat/material_list.txt");
	if (txtlist.is_open()) 
	{
		int i = 0;
		std::string material_txtname;
		while (getline(txtlist, material_txtname)) 
		{
			path.insert(14, material_txtname);
			printf("Func: %s.\n", path.c_str());
			
			material_file.open(path, std::ifstream::binary);
			
			

			Settings.material[i] = material_txtname;
			if (i == 0 && material_file.is_open() && (material_file.peek() != EOF))
			{
				material_file >> materials;
				int n = 0;
				n = std::stoi(materials["N"].asString());
				Settings.meanCount = (uint16_t)n;
				Settings.fLow = std::stof(materials["FLow"].asString())/0.01;
				Settings.fHigh = std::stof(materials["FHigh"].asString())/0.01;
				Settings.sensor_density = std::stof(materials["Density"].asString());
				Settings.sound_speed = std::stof(materials["SoundSpeed"].asString());
				Settings.depth = std::stof(materials["Depth"].asString());
				
				switch(Settings.meanCount)
				{
					case 1:
						Settings.selectedN = 0;
						break;
					case 20:
						Settings.selectedN = 1; 
						break;
					case 50:
						Settings.selectedN = 2;
						break;
					case 100:
						Settings.selectedN = 3;
						break;
					case 200:
						Settings.selectedN = 4;
						break;
					case 500:
						Settings.selectedN = 5;
						break;
					case 1000:
						Settings.selectedN = 6;
						break;
				}

				if (materials["ProcessType"].asString() == "RAW")
					Settings.selectedProc = 0; 
				else if (materials["ProcessType"].asString() == "FILTERED")
					Settings.selectedProc = 1; 
				else if (materials["ProcessType"].asString() == "PROCESSED")
					Settings.selectedProc = 2;

				Settings.currentProcessType = (uint8_t)Settings.selectedProc;
				Settings.UpdateFilterSettings();

				if (materials["Filt"].asString() == "BP")
					Settings.numfilter = 0; 
				else if (materials["Filt"].asString() == "LP")
					Settings.numfilter = 1; 
				else if (materials["Filt"].asString() == "None")
					Settings.numfilter = 2;

				Settings.UpdateParameter(0);
				Settings.UpdateParameter(1);
				Settings.UpdateParameter(2);
				Settings.UpdateParameter(3);
			}

			path = "/LUS_software/gui/mat/.json";
			i++;

			material_file.close(); 
		}
	}
	txtlist.close();

    SyncConfigData(Configuration, ParseConfigRecord(FileManager.GetStoredConfigurationData()));

    Sampler.SetGpioState(Gpio::FAN, true);

    if(argc > 1)
    {
		//Images and font placing into flash

		ImageStorage.PrepareFlash();
		return 0;
    }

	DR_EnterCmdMode();

	DRC_DlStart();
	DRC_FlashFast();
	DR_Display();
	DRC_Swap();
	DRC_WaitFifoEmpty();

	rd32(REG_FLASH_STATUS);
	rd32(REG_FLASH_SIZE);

	//инициализация в памяти экрана шрифта
	ImageStorage.InitFont(Dialog::DEFAULT_FONT, FontTag::ARIAL_24);

	//первое открытие диалога с А сканом
	Dialog::EnterDialog(new ChartScreenA);
	Dialog::SwitchDialog();
	//инициализация стартового состояния кнопок
	uint16_t keys = 0;
	uint16_t prev_keys = 0;

	uint8_t pinc = 0;
	//инициализация стартового состояния сенсора
	uint16_t prev_x = 0x8000;
	uint16_t prev_y = 0x8000;
	bool first_touch = true;
	//инициализация стартового направления свайпа
	SwipeDirection swipe_direction = SwipeDirection::NONE;
	bool swipe_handled = false;

	StartTimer(Timer::BLINK, 10);
	bool blink_state = false;
	bool on_capture = false;
	bool on_processing = false;

	std::vector<uint16_t> captured_samples{};

	std::thread mean_thread();
	ProcessingData processing_data{};
	std::chrono::time_point<std::chrono::high_resolution_clock> starti2c, endi2c;
	std::chrono::time_point<std::chrono::high_resolution_clock> startTemp, endTemp;
	std::chrono::time_point<std::chrono::high_resolution_clock> startip, endip;
	starti2c = std::chrono::high_resolution_clock::now();
	startTemp = std::chrono::high_resolution_clock::now();
	startip = std::chrono::high_resolution_clock::now();

	if (Settings.ip == "IP Address : ")
	{
		redi::ipstream proc("ip route get 1 | awk '{print $NF;exit}'", redi::pstreams::pstdout | redi::pstreams::pstderr );
		std::string retval;

		while (std::getline(proc.out(), retval))
		{
			ip_time++;
			if (retval[0] == '1')
			{	
				Settings.ip.append(retval);
				break;
			}
			if (ip_time == 3)
			{	
				Settings.ip.append("not connected");
				break;
			}
		}
	}

	while(1)
	{
		if(Settings.refresh)
		{
			std::cout<<"FUF"<<std::endl;
			Settings.refresh = false;
			Settings.old_offset = Settings.offset_chart;
		}
		endi2c = std::chrono::high_resolution_clock::now();
		endTemp = std::chrono::high_resolution_clock::now();
		endip = std::chrono::high_resolution_clock::now();
		SlowTimerRoutine();
		//читаем i2c раз в 0.3с для увеличения фпс
		if (std::chrono::duration<double>(endi2c - starti2c).count() > 0.3)
		{
			I2CBus.ProcessExchange();
			starti2c = std::chrono::high_resolution_clock::now();
		}

		if (std::chrono::duration<double>(endip - startip).count() > 5.0)
		{
			//std::system("ip route flush cache");
			redi::ipstream proc("ip route get 1 | awk '{print $NF;exit}'", redi::pstreams::pstdout | redi::pstreams::pstderr );
			std::string retval;

			while (std::getline(proc.out(), retval))
			{
				std::cout<<"GGGGGG"<<retval<<std::endl;
				if (retval[0] == '1')
				{	
					Settings.ip = "IP Address : ";
					Settings.ip.append(retval);
				}
				else
				{
					Settings.ip = "IP Address : ";
					Settings.ip.append("not connected");
				}
				break;
			}
			startip = std::chrono::high_resolution_clock::now();
		}

		if (std::chrono::duration<double>(endTemp - startTemp).count() > 3.0)
		{
			std::ifstream offset_temp0("/sys/bus/iio/devices/iio:device0/in_temp0_offset");
			std::ifstream raw_temp0("/sys/bus/iio/devices/iio:device0/in_temp0_raw");
			std::ifstream scale_temp0("/sys/bus/iio/devices/iio:device0/in_temp0_scale");
			if (offset_temp0.is_open())
			{
				while (getline(offset_temp0, temp_com)) 
				{
					off = std::stof(temp_com);
					std::cout<<off<<std::endl;
				}
			}

			if (raw_temp0.is_open())
			{
				while (getline(raw_temp0, temp_com)) 
				{
					raw = std::stof(temp_com);
					std::cout<<raw<<std::endl;
				}
			}

			if (scale_temp0.is_open())
			{
				while (getline(scale_temp0, temp_com)) 
				{
					sscanf(temp_com.c_str(), "%f", &scl);
					std::cout<<scl<<std::endl;
				}
			}
			
			temp = ((off + raw) * scl)/1000.0;
			
			if (temp > 40 || Settings.hs1Temp > 40)
				I2CBus.SetOutputState(ControlOutput::LASER_FAN, 1);

			if (temp <= 30 && Settings.hs1Temp <=30)
				I2CBus.SetOutputState(ControlOutput::LASER_FAN, 0);

			startTemp = std::chrono::high_resolution_clock::now();
		}

		if(I2CBus.HasBatteryConnection())
		{
			I2CBus.SetOutputState(
					ControlOutput::CHARGE_LED,
					(I2CBus.GetChargeStatus() & (1 << 6)) == 0);
		}
		else
		{
			I2CBus.SetOutputState(
					ControlOutput::CHARGE_LED,
					false);
		}

		keys = I2CBus.GetKeyMask();


		//если состояние кнопок изменилось, то мы запоминаем новое состояние кнопок
		if(pinc != (I2CBus.pinc & 0x0F))
		{
			pinc = I2CBus.pinc & 0x0F;
			//printf("PINC: %x\n", pinc);
		}
#if 0
		if(keys & ~prev_keys)
		{
			printf("Keys: %x\n", I2CBus.GetKeyMask());
		}
#endif

		//

		std::ifstream snap("/LUS_software/gui/com/snap.txt");//, std::ofstream::out | std::ofstream::trunc);  
		std::string com;
		if (snap.is_open())
		{
			while (getline(snap, com)) 
			{
	
				if (com[0] == 'S')
				{
					snapshot();
					std::system("rm -f /LUS_software/gui/com/snap.txt");
				}
	

			}
			snap.close(); 
		}


		///
		std::ifstream iplist("/LUS_software/gui/com/iplist.txt");//, std::ofstream::out | std::ofstream::trunc);  
		std::string ip;
		if (iplist.is_open())
		{
			while (getline(iplist, ip)) 
			{
	
				Settings.server_ip = ip;
				//std::system("rm -f /LUS_software/gui/com/iplist.txt");	

			}
			iplist.close(); 
		}

		///
		std::ifstream txtlist("/LUS_software/gui/com/command.txt");//, std::ofstream::out | std::ofstream::trunc);  
		std::string command;
		if (txtlist.is_open())
		{
			while (getline(txtlist, command)) 
			{
				if (command[0] == 'A')
				{
					Settings.draw_warning = true;	
					if (Settings.pc_warning_visible)
					{
						Dialog::MainDialog()->InvalidateRequest();
						Dialog::EnterDialog(new PCDialog);
						Dialog::SwitchDialog();
					}
					std::system("rm -f /LUS_software/gui/com/command.txt");
					printf("IP0: %s\n",Settings.server_ip.c_str());
					int optval = 6;
					socklen_t optlen = sizeof(optval);
					Settings.client_socket = socket(AF_INET, SOCK_STREAM, 0);
					setsockopt(Settings.client_socket, SOL_SOCKET, SO_PRIORITY, &optval, optlen);

					Settings.server_address.sin_family = AF_INET;
					Settings.server_address.sin_port = htons(5000);
					Settings.server_address.sin_addr.s_addr = inet_addr(Settings.server_ip.c_str());
					connect(Settings.client_socket, (struct sockaddr *)&Settings.server_address, sizeof(Settings.server_address));
					printf("IP: %s\n", Settings.server_ip.c_str());
				}

				if (command[0] == 'Q')
				{
					Settings.draw_warning = false;	
					Dialog::MainDialog()->InvalidateRequest();

					close(Settings.client_socket);	

					if (Settings.pc_warning_visible)
					{
						wr8(REG_PCLK, 1); 
						ImageStorage.FreeTemp();
						Dialog::EnterDialog(new ChartScreenA);
						
						if (Settings.ab==2)
						{
							ImageStorage.FreeTemp();
							Dialog::EnterDialog(new ChartScreenB);
						}        
						Dialog::SwitchDialog();  
						Dialog::MainDialog()->InvalidateRequest();  
					}

					Settings.t4mod = false;
					Settings.t2mod = true;
					Settings.state_laser_com = false;
					std::system("rm -f /LUS_software/gui/com/command.txt");	
					
					Settings.meanCount = (uint16_t)previous_meanCount;
					
					switch(Settings.meanCount)
					{
						case 1:
							Settings.selectedN = 0;
							break;
						case 5:
							Settings.selectedN = 0;
							break;

						case 20:
							Settings.selectedN = 1; 
							break;
						case 50:
							Settings.selectedN = 2;
							break;
						case 100:
							Settings.selectedN = 3;
							break;
						case 200:
							Settings.selectedN = 4;
							break;
						case 500:
							Settings.selectedN = 5;
							break;
						case 1000:
							Settings.selectedN = 6;
							break;
					}
				}


				if (command[0] == 'E')
				{                                    
					//Settings.t4mod = false;
					//Settings.t2mod = true;
					Settings.state_laser_com = false;
					std::system("rm -f /LUS_software/gui/com/command.txt");	
					Settings.meanCount = (uint16_t)previous_meanCount;
					
					switch(Settings.meanCount)
					{
						case 1:
							Settings.selectedN = 0;
							break;
						case 5:
							Settings.selectedN = 0;
							break;

						case 20:
							Settings.selectedN = 1; 
							break;
						case 50:
							Settings.selectedN = 2;
							break;
						case 100:
							Settings.selectedN = 3;
							break;
						case 200:
							Settings.selectedN = 4;
							break;
						case 500:
							Settings.selectedN = 5;
							break;
						case 1000:
							Settings.selectedN = 6;
							break;
					}
				}
			
				if (command[0] == 'S')
				{

					
				
					//wr8(REG_PCLK, 0); //                                       
					int n = 0;
					previous_meanCount = Settings.meanCount;
					n = std::stoi(command.substr(1, command.size()-1));

					Settings.meanCount = (uint16_t)n;
					
					switch(Settings.meanCount)
					{
						case 1:
							Settings.selectedN = 0;
							break;
						case 5:
							Settings.selectedN = 0;
							break;

						case 20:
							Settings.selectedN = 1; 
							break;
						case 50:
							Settings.selectedN = 2;
							break;
						case 100:
							Settings.selectedN = 3;
							break;
						case 200:
							Settings.selectedN = 4;
							break;
						case 500:
							Settings.selectedN = 5;
							break;
						case 1000:
							Settings.selectedN = 6;
							break;
					}
					Settings.state_laser_com = true;
					Settings.t4mod = true;
					Settings.t2mod = false;

					
					std::system("rm -f /LUS_software/gui/com/command.txt");
				}
			
			}
			txtlist.close(); 
		}
		
		//

		//если за прошлый цикл обновились данные графика, то мы обновляем данные в нашем текущем диалоге
		if(DataStorage.HasNewData())
		{
			StopTimer(Timer::UPDATE_CONFIG);

			Dialog::MainDialog()->UpdateData(true);
			DataStorage.ResetReseption();
		}
		//если за прошлый цикл обновились данные фильтра, то мы обновляем данные в нашем текущем даилоге					
		if(DataStorage.HasUpdatedData())
		{
			StopTimer(Timer::UPDATE_CONFIG);

			Dialog::MainDialog()->UpdateData(false);
			DataStorage.ResetReseption();
		}
		//вход в машину состояний
		DataStorage.ProcessCapture();
		//читаем регистр с оординатами тачскрина 
		uint32_t xy = rd32(REG_TOUCH_SCREEN_XY);
		//если в регистре не число, обозначающее, что касания нет, то обрабатываем сенсор
		if(xy != 0x80008000)
		{
			uint16_t x = (xy & 0xFFFF0000) >> 16;
			uint16_t y = xy & 0xFFFF;
			if(!swipe_handled)
			{
				if (x >1024 - 144)//перевод нажатияна таскрин в нажатие физической клавиатуры
				{
					if (y < 120) //line 1(top)
					{
						if (x > 1024 - 144 + 72)
						{
							Dialog::MainDialog()->HandleKeys(prev_keys, pow(2,4));
							prev_keys = pow(2,4);
						}//right
						else
						{
							Dialog::MainDialog()->HandleKeys(prev_keys, pow(2,9));
							prev_keys = pow(2,9);
						}//left
					}
					else if (y >= 120 && y <= 120*2) //line 2
					{
						if (x > 1024 - 144 + 72)
						{
							Dialog::MainDialog()->HandleKeys(prev_keys, pow(2,3));
							prev_keys = pow(2,3);
						}//right
						else
						{
							Dialog::MainDialog()->HandleKeys(prev_keys, pow(2,8));
							prev_keys = pow(2,8);
						}//left
					}
					else if (y >= 120*2 && y <= 120*3) //line 3
					{
						if (x > 1024 - 144 + 72)
						{
							Dialog::MainDialog()->HandleKeys(prev_keys, pow(2,2));
							prev_keys = pow(2,2);
						}//right
						else
						{
							Dialog::MainDialog()->HandleKeys(prev_keys, pow(2,7));
							prev_keys = pow(2,7);
						}//left
					}
					else if (y >= 120*3 && y <= 120*4) // line 4
					{
						if (x > 1024 - 144 + 72)
						{
							Dialog::MainDialog()->HandleKeys(prev_keys, pow(2,1));
							prev_keys = pow(2,1);
						}//right
						else
						{
							Dialog::MainDialog()->HandleKeys(prev_keys, pow(2,6));
							prev_keys = pow(2,6);
						}//left
					}
					else if (y >= 120*4 && y <= 120*5) // line 5(bottom)
					{
						if (x > 1024 - 144 + 72)
						{
							Dialog::MainDialog()->HandleKeys(prev_keys, pow(2,0));
							prev_keys = pow(2,0);
						}//right
						
						else
						{
							Dialog::MainDialog()->HandleKeys(prev_keys, pow(2,5));
							prev_keys = pow(2,5);
						}//left
					}

					first_touch = false;
				}		
				else
				{
					if(keys != prev_keys)
					{
						Dialog::MainDialog()->HandleKeys(prev_keys, keys);
						prev_keys = keys;
					}
				}		
				Dialog::MainDialog()->HandleTouch(x, y);
			}
		}
		else
		{
			Settings.flag_move = 0;
			if(keys != prev_keys)
			{
				Dialog::MainDialog()->HandleKeys(prev_keys, keys);
				prev_keys = keys;
			}
			Dialog::MainDialog()->HandleDetouch();
			prev_x = 0x8000;
			prev_y = 0x8000;
			swipe_direction = SwipeDirection::NONE;
			swipe_handled = false;
		}
		//Dialog::MainDialog()->InvalidateRequest();  
		//процесс мигания. Если таймер на мигание закончился, то состояние мигания меняется на противоположное
		if(GetTimerState(Timer::BLINK) != TimerState::BUSY)
		{
			LaserControl.ProcessLaserExchange();
			
			printf("timer %d", blink_state);
			blink_state = !blink_state;
			Dialog::MainDialog()->InvalidateRequest();  
			Dialog::MainDialog()->HandleBlink();

			StartTimer(Timer::BLINK, 500);
		}

		if(GetTimerState(Timer::UPDATE_CONFIG) == TimerState::EXPIRED)
		{
			StopTimer(Timer::UPDATE_CONFIG);
			DataStorage.ReprocessData();
		}


		Dialog::MainDialog()->HandleTimers();
		Dialog::SwitchDialog();

		StatusHandler *handler = Dialog::MainDialog()->GetStatusHandler();

		if(handler)
		{
			BatteryState bstate = BatteryState::NORMAL;

			if(!I2CBus.HasBatteryConnection())
				bstate = BatteryState::ABSENT;
			else if((I2CBus.GetChargeStatus() & (1 << 6)) == 0)
				bstate = BatteryState::CHARGING;

			handler->SetBatteryState(bstate, I2CBus.GetChargeLevel());

			handler->SetLaserOn(LaserControl.IsLaserOn());
			handler->SetLaserState(LaserControl.GetLaserState());

			uint8_t contact_state = 2;

			if(LuAlgo.IsContactGood(&contact_state))
			{
				ContactState new_state = ContactState::UNKNOWN;

				switch(contact_state)
				{
				case 0:
					new_state = ContactState::BAD;
					break;

				case 1:
					new_state = ContactState::GOOD;
					break;
				}

				handler->SetContactState(new_state);

			}
			else
			{
				handler->SetContactState(ContactState::UNKNOWN);
			}
		}

					
		//Pause repeat timer on the draw time to prevent repetitions on slow dialogs
		//PauseTimer(UiTimer::REPEAT);
		if(DataStorage.HasNewData())
		{
			StopTimer(Timer::UPDATE_CONFIG);

			Dialog::MainDialog()->UpdateData(true);
			DataStorage.ResetReseption();
		}
		else
		{
			if(DataStorage.HasUpdatedData())
			{
				StopTimer(Timer::UPDATE_CONFIG);

				Dialog::MainDialog()->UpdateData(false);
				DataStorage.ResetReseption();
			}
		}
		//printf("ffffffhhhhhh %d", blink_state);
		Dialog::MainDialog()->Draw(blink_state);
		//ResumeTimer(UiTimer::REPEAT);

		CheckStoreConiguration();

		if(!Sampler.CheckMagicState())
			return 0;
		uint16_t x = (xy & 0xFFFF0000) >> 16;
		uint16_t y = xy & 0xFFFF;

		//вызов скриншота при нажатии на батарейку
		if (x < 50 && y<50)
		{
			snapshot();
		}
/*
		auto end1 = std::chrono::high_resolution_clock::now();
		double time_taken1 = std::chrono::duration_cast<std::chrono::nanoseconds>(end1 - start1).count();
		time_taken1 *= 1e-9;
*/		//std::cout<<"Global while loop in ms: "<<time_taken1*1000<<std::setprecision(9)<<std::endl;
	}
	
	return 0;
}
