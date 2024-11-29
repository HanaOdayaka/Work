#ifndef DATA_STORAGE_H
#define DATA_STORAGE_H

#include <stdint.h>
#include <vector>
#include <list>
#include <thread>

#include "processing.h"

enum class CaptureState {
	WAITING = 0,
	CAPTURING,
	PROCESSING,
	PROCESSED,
	REPROCESSED
};


//класс отвечающий за хранение графиков и создание новых
class DataStorageApi {

public:
	DataStorageApi();

	//лист необработанных графиков
	std::list<std::vector<float>>    rawData;
	std::vector<float> tempData = std::vector<float>(8192);



	//лист обработанных графиков
	std::list<std::vector<float>>    processedData;

	//переменная счетчик для усреднения времени
	int counter = max_counter;
	//переменная отвечающая за кол-во измерений для усреднения времени
	int max_counter = 49;

	//проверка машины состояний на готовность нового графика
	bool HasNewData() {
		return state == CaptureState::PROCESSED;
	}


	//проверка машины состояний на готовность обработки фильтром старого графика
	bool HasUpdatedData() {
		return state == CaptureState::REPROCESSED;
	}

	//сброс машины состояний до состояния WAITING, если график обработан или получен
	void ResetReseption() {
		if(state == CaptureState::PROCESSED ||
		   state == CaptureState::REPROCESSED)
		{
			state = CaptureState::WAITING;
		}
	}

	//функция реализующая машину состояний для измерения графика 
	void ProcessCapture();
	void sending();
	void getting();

	std::vector<float> GetLastRawRecord() {
		if(rawData.size() != 0)
			return rawData.back();
		else
			return {0};

	}

	//функция возвращающая последний график
	std::vector<float> GetLastProcessedRecord() {

		return processedData.back();

	}

	//функция обработки послднего графика при установке новых параметров фильтра
	void ReprocessData();

	//функция включения/выключения машины состояний
	void SetOnscreenSampleButtonState(bool state)
	{
		onScreenSampleButtonState = state;
	}

private:
	ProcessingData processingThreadData;

	//переменная прошлого состояния включенности машиы состояний
	bool prevKeyState;

	//пересенная хранящая состояни машины состояний
	CaptureState   state;

    // Транслируем нажатие клавиши "лазер" на экране настроек в эту переменную
	bool onScreenSampleButtonState;
};

extern DataStorageApi DataStorage;

#endif
