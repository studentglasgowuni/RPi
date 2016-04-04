#ifndef ADCREADER
#define ADCREADER

#include <QThread>
 
class ADCreader : public QThread
{
public:
	ADCreader();
	void quit();
	void run();
	int get_samples();
	bool read_enable();
private:
	bool running;
	int ret;
	int fd;
	int no_tty;
	int inp;
	int outp;
	int buff[100];


};

#endif
