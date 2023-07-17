#ifndef DATA_H
#define DATA_H

#include <stdint.h>

// pack - переводит буфер в строку шестнадцатиричных символов
// unpack - наоборот
// для упаковки на архитектурах littleendian  использовать #define LITTLEENDIANPACK
// для bigendian закоментировать или удалить #define LITTLEENDIANPACK

// packch и unpackch - для работы со строками char[] - отличие в игнорировании
// littleendian bigendian

inline int packch(char* buf, void *data, int dataSize)
{
	for(int i = 0; i < dataSize; ++i)
	{

		const unsigned char c = ((unsigned char*)data)[i];
		const unsigned char h = (c >> 4);
		buf[0] = (h < 10) ? (h + 48) : (h + 55);
		const unsigned char l = (c & 15);
		buf[1] = (l < 10) ? (l + 48) : (l + 55);
		buf += 2;
	}

	return dataSize * 2;
}
inline int unpackch(char* buf, void *data, int dataSize)
{
	for(int i = 0; i < dataSize; ++i)
	{
		((char*)data)[i] =
				((buf[0] - (buf[0] < 58 ? 48 : 55)) << 4) |
				(buf[1] - (buf[1] < 58 ? 48 : 55));
		buf += 2;
	}

	return dataSize * 2;
}

#define LITTLEENDIANPACK
inline int pack(char* buf, void *data, int dataSize)
{
#ifdef LITTLEENDIANPACK
	for(int i = 0; i < dataSize; ++i)
#else
	for(int i = dataSize - 1; i >= 0; --i)
#endif
	{

		const unsigned char c = ((unsigned char*)data)[i];
		const unsigned char h = (c >> 4);
		buf[0] = (h < 10) ? (h + 48) : (h + 55);
		const unsigned char l = (c & 15);
		buf[1] = (l < 10) ? (l + 48) : (l + 55);
		buf += 2;
	}

	return dataSize * 2;
}
inline int unpack(char* buf, void *data, int dataSize)
{
#ifdef LITTLEENDIANPACK
	for(int i = 0; i < dataSize; ++i)
#else
	for(int i = dataSize - 1; i >= 0; --i)
#endif
	{
		((char*)data)[i] =
				((buf[0] - (buf[0] < 58 ? 48 : 55)) << 4) |
				(buf[1] - (buf[1] < 58 ? 48 : 55));
		buf += 2;
	}

	return dataSize * 2;
}
#undef LITTLEENDIANPACK

struct Data
{
	// поля структуры должны иметь хорошо документированный размер,
	// зависящий от платформы, так как этот код планируется использовать и
	// в буях (микроконтроллер) и на ПК поддержки (x86 или 64). НЕОХОДИМО быть
	// уверенным что sizeof(поле структуры) имеет одинаковый размер на всех
	// платформах работающих с кодом, так как структура сериализуется и
	// отправляется по сети побайтно.
	// int x; // пример плохого - int платформозависим


	// Сейсчас размер буфера для упаковки = 150 и это число hardcoded много где,
	// поэтому поля структуры менять с осторожностью.
	char    datetime[20]; // дата-время (строка вида "2016-01-01 22:45:54\0")
	int8_t  buoy_id; // идентификатор буя
	double  lat;     // широта
	double  lon;     // долгота
	int16_t state;   // состояние буя
	double  bat_v;   // напряжение батареи
	double  sensor1; // данные датчика 1 (температура)
	double  sensor2; // данные датчика 2 (солёность)
	double  sensor3; // данные датчика 3 (глубина)
	int16_t bitrate; // битрейт (от модема)
	int16_t RSSI;    // уровень сигнала (от модема)
	//!если добавляем поля структуры, незабывать добавлять PACKALL

	// в PACKALL добавлять поля структуры (для рас/упаковки)
#define PACKALL \
	PACKCH(datetime);\
	PACK(buoy_id);\
	PACK(lat);    \
	PACK(lon);    \
	PACK(state);  \
	PACK(bat_v);  \
	PACK(sensor1);\
	PACK(sensor2);\
	PACK(sensor3);\
	PACK(bitrate);\
	PACK(RSSI);


public:

	// Внимание! необходимо самостоятельно и строго следить за размером
	// сериализованных данных и буфера buf. Никаких проверок на выход за
	// границу!
	int toString(char *buf)
	{
		int pos = 0;
#define PACKCH(m) pos += packch(buf + pos, &m, sizeof(m))
#define PACK(m) pos += pack(buf + pos, &m, sizeof(m))
		PACKALL
#undef  PACK
#undef  PACKCH

		return pos; // вернуть количество упакованных символов
	}
	void fromString(char *buf)
	{
		int pos = 0;
#define PACKCH(m) pos += unpackch(buf + pos, &m, sizeof(m))
#define PACK(m) pos += unpack(buf + pos, &m, sizeof(m))
		PACKALL
#undef  PACK
#undef  PACKCH
	}

#undef PACKALL
};

#endif // DATA_H
