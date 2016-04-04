#ifndef ADCREADER
#define ADCREADER

#include <QThread>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "gz_clk.h"
 
class ADCreader : public QThread
{
public:
	ADCreader();
	void quit();
	void run();
	int getSample();
	bool hasSample();

	bool running;
	int ret;
	int fd;
	int sysfs_fd;
	int no_tty;
	int inp;
	int outp;
	int buff[100];


};

#endif
