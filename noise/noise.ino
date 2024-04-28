// ATTINY45
// burn bootloader for 8MHz
// arduino IDE 2.2.1

#define audioPin 0

/* NOISE */

class noise
{
public:
    noise(int noise_pin);
    void generate(int noise_color);
private:
    int _pin;
	int _color;
	unsigned long int reg;
	unsigned long int newr;
	unsigned char lobit;
	unsigned char b31, b29, b25, b24;
};

noise::noise(int noise_pin)
{
	pinMode(noise_pin, OUTPUT);
	_pin = noise_pin;
	reg = 0x55aa55aaL; //The seed for the bitstream. 
	
}
void noise::generate(int noise_color)
{
  b31 = (reg & (1L << 31)) >> 31;
	b29 = (reg & (1L << 29)) >> 29;
	b25 = (reg & (1L << 25)) >> 25;
	b24 = (reg & (1L << 24)) >> 24;
	lobit = b31 ^ b29 ^ b25 ^ b24;
	newr = (reg << 1) | lobit;
	reg = newr;
	digitalWrite (_pin, reg & 1);
	delayMicroseconds (noise_color);
};

noise noise(audioPin);

void setup() {
}

void loop() {
  noise.generate(0);
}