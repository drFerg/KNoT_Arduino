typedef struct temp_double_payload{
	double temp;
	uint8_t type;
}TempDPayload;

typedef struct rgb_payload{
	uint8_t red;
	uint8_t green;
	uint8_t blue;
}RGBPayload;


typedef struct serial_connect{
	uint8_t addr;
	uint8_t rate;
}SerialConnect;

typedef struct serial_query{
	uint8_t type;
}SerialQuery;
