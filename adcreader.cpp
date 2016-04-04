#include "adcreader.h"
#include <QDebug>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#include "gz_clk.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static void pabort(const char *s)
{
	perror(s);
	abort();
}

static const char *device = "/dev/spidev0.0";
static uint8_t mode = SPI_CPHA | SPI_CPOL;
static uint8_t bits = 8;
static uint32_t speed = 50000;
static uint16_t delay = 10;
static int drdy_GPIO = 22;
int inp=0;
int outp=0;
int buff[100];

static void writeReset(int fd)
{
	int ret;
	uint8_t tx1[5] = {0xff,0xff,0xff,0xff,0xff};
	uint8_t rx1[5] = {0};
	struct spi_ioc_transfer tr = {
		(unsigned long)tx1,
		(unsigned long)rx1,
		ARRAY_SIZE(tx1),
		speed,
		delay,
		bits,
		NULL,
		NULL,
	};
	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		pabort("can't send spi message");
}

static void writeReg(int fd, uint8_t v)
{
	int ret;
	uint8_t tx1[1];
	tx1[0] = v;
	uint8_t rx1[1] = {0};
	struct spi_ioc_transfer tr = {
		(unsigned long)tx1,
		(unsigned long)rx1,
		ARRAY_SIZE(tx1),
		speed,
		delay,
		bits,
		NULL,NULL,

	};

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		pabort("can't send spi message");

}

static uint8_t readReg(int fd)
{
	int ret;
	uint8_t tx1[1];
	tx1[0] = 0;
	uint8_t rx1[1] = {0};
	struct spi_ioc_transfer tr = {
		(unsigned long)tx1,
		(unsigned long)rx1,
		ARRAY_SIZE(tx1),
		speed,
		delay,
		8,
		NULL,NULL
	};


	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
	  pabort("can't send spi message");
	  
	return rx1[0];
}

static int readData(int fd)
{
	int ret;
	uint8_t tx1[2] = {0,0};
	uint8_t rx1[2] = {0,0};
	struct spi_ioc_transfer tr = {
		(unsigned long)tx1,
		(unsigned long)rx1,
		ARRAY_SIZE(tx1),
		speed,
		delay,
		8,NULL,NULL,
	};

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
	  pabort("can't send spi message");
	  
	return (rx1[0]<<8)|(rx1[1]);
}

ADCreader::ADCreader(){

	//int no_tty = !isatty( fileno(stdout) );

	fd = open(device, O_RDWR);
	if (fd < 0)
		pabort("can't open device");

	/*
	 * spi mode
	 */
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
		pabort("can't set spi mode");

	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1)
		pabort("can't get spi mode");

	/*
	 * bits per word
	 */
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't set bits per word");

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't get bits per word");

	/*
	 * max speed hz
	 */
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't set max speed hz");

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't get max speed hz");

	fprintf(stderr, "spi mode: %d\n", mode);
	fprintf(stderr, "bits per word: %d\n", bits);
	fprintf(stderr, "max speed: %d Hz (%d KHz)\n", speed, speed/1000);

	// enable master clock for the AD
	// divisor results in roughly 4.9MHz
	// this also inits the general purpose IO
	gz_clock_ena(GZ_CLK_5MHz,5);

	bcm2835_gpio_fsel(drdy_GPIO, BCM2835_GPIO_FSEL_INPT);

	// resets the AD7705 so that it expects a write to the communication register
	writeReset(fd);

	// tell the AD7705 that the next write will be to the clock register
	writeReg(fd,0x20);
	// write 00001100 : CLOCKDIV=1,CLK=1,expects 4.9152MHz input clock
	writeReg(fd,0x0C);

	// tell the AD7705 that the next write will be the setup register
	writeReg(fd,0x10);
	// intiates a self calibration and then after that starts converting
	writeReg(fd,0x00);
	running=0;

}
void ADCreader::run()
{
	
	// we read data in an endless loop and display it
	running = true; 
	while (running) {
	int d=0;
	  do {
	    // read /DRDY of the AD converter
	    d = bcm2835_gpio_lev(drdy_GPIO);
	    // loop while /DRDY is high
	  } while ( d );
	  
	  // tell the AD7705 to read the data register (16 bits)
	  writeReg(fd,0x38);
	  // read the data register by performing two 8 bit reads
	  int value = readData(fd)-0x8000;
	  buff[inp]=value;
          //fprintf(stderr,"data = %d       \n",buff[inp]);
	  inp=(inp+1)%100;	
	  

		// if stdout is redirected to a file or pipe, output the data
		/*if( no_tty )
		{
			printf("%d\n", value);
			fflush(stdout);
		}*/
	  //sleep(0.25);
	}
}

bool ADCreader::read_enable(){
	 if(inp==outp){
		return false;
	}else{
		 return true;
	}	
}


int ADCreader::get_samples()
{
      int ret=buff[outp];
      //fprintf(stderr,"OUTPUT DATA = %d       \n",buff[outp]);	
      outp=(outp+1)%100;	
      return ret;
}


void ADCreader::quit()
{
	running = false;
	exit(0);
}
