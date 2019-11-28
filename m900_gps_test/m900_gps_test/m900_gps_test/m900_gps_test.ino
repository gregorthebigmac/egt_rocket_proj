#include <SPI.h>
#include <RH_RF95.h>
#include <TinyGPS.h>

// for feather32u4
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 7

// if freq changes, change this value to new freq
#define RF95_FREQ 915.0

RH_RF95 rf95(RFM95_CS, RFM95_INT);

TinyGPS gps;
float flat = 0;
float flon = 0;
float falt = 0;

void f_to_char(float f_input, char c_input[], char output[]);

void setup() {
	pinMode(RFM95_RST, OUTPUT);
	digitalWrite(RFM95_RST, HIGH);
	delay(100);
	// manual reset
	digitalWrite(RFM95_RST, LOW);
	delay(10);
	digitalWrite(RFM95_RST, HIGH);
	delay(10);
	Serial1.begin(9600);
	while (!Serial1);
	while (!rf95.init()) {
		while (1);
	}

	// defaults after init are 434.0 MHz, modulation GFSK_Rb250Fd250, +13dbM
	if (!rf95.setFrequency(RF95_FREQ))
		while (1);
	rf95.setTxPower(23, false);
}

int16_t packetnum = 0;	// packet counter, we increment per xmission

void loop() {
	bool new_data = false;
	unsigned long gps_chars;
	unsigned short sentences, failed;
	// for one second we parse GPS data and report some key values
	for (unsigned long start = millis(); millis() - start < 1000;) {
		while (Serial1.available()) {
			char c = Serial1.read();
			if (gps.encode(c))
				new_data = true;
		}
	}
	if (new_data) {
		int sat, prec, year;
		byte month, day, hour, minute, second, hundredths;
		unsigned long age, fix_age;
		char chlat[20] = "LAT: ";
		char chlon[20] = "LON: ";
		char chsat[20] = "SAT: ";
		char chprec[20] = "PREC: ";
		gps.f_get_position(&flat, &flon, &age);
		gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &fix_age);
		falt = gps.f_altitude();
		sat = gps.satellites() == TinyGPS::GPS_INVALID_SATELLITES ? 0 : gps.satellites();
		itoa(sat, chsat + 5, 15);
		prec = gps.hdop() == TinyGPS::GPS_INVALID_HDOP ? 0 : gps.hdop();
		itoa(prec, chprec + 6, 14);
		gps.stats(&gps_chars, &sentences, &failed);
		delay(10);
		// sending latitude to receiver
		char c_lat[16];
		f_to_char(flat, "LAT: ", c_lat);
		rf95.send((uint8_t *)c_lat, 17);
		delay(10);
		// sending longitude to receiver
		char c_lon[16];
		f_to_char(flon, "LON: ", c_lon);
		rf95.send((uint8_t *)c_lon, 17);
		delay(10);
		// sending altitude to receiver
		char c_alt[10];
		f_to_char(falt, "ALT: ", c_alt);
		rf95.send((uint8_t *)c_alt, 11);
		delay(10);
		// sending number of satellites locked to receiver
		char c_sat[7];
		sat_and_prec(sat, "SAT: ", c_sat);
		rf95.send((uint8_t *)c_sat, 8);
		delay(10);
		// sending precision calculation to receiver
		char c_prec[10];
		sat_and_prec(prec, "PREC: ", c_prec);
		rf95.send((uint8_t *)c_prec, 11);
		delay(10);
		rf95.waitPacketSent();
	}
}

// convert float to char array via string
void f_to_char(float f_input, char c_input[], char output[]) {
	String s_final = String(c_input);
	// we don't need more than 2 decimal places for altitude precision.
	if (c_input[0] == 'A') {
		String temp = String(f_input, 2);
		s_final.concat(temp);
	}
	// we DO need the full 6 digit precision for LAT/LON
	else {
		String temp = String(f_input, 7);
		s_final.concat(temp);
	}
	int final_length = s_final.length();
	s_final.toCharArray(output, final_length);
}

// convert integers to character array via string
void sat_and_prec(int i_input, char c_input[], char output[]) {
	String s_final = String(c_input);
	String temp = String(i_input);
	s_final.concat(temp);
	s_final.concat("0");
	int final_size = s_final.length();
	s_final.toCharArray(output, final_size);
}
