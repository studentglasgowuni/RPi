#include "window.h"
#include "adcreader.h"
#include <stdlib.h>//for audio
//#include <cmath>  // for sine stuff


Window::Window() : gain(5),count(0)
{
	
	knob = new QwtKnob;
	// set up the gain knob 
	knob->setValue(gain);

	// use the Qt signals/slots framework to update the gain -
	// every time the knob is moved, the setGain function will be called
	connect( knob, SIGNAL(valueChanged(double)), SLOT(setGain(double)) );

	// set up the thermometer
        thermo=new QwtThermo;
	thermo->setFillBrush( QBrush(Qt::red) );
	thermo->setRange(0, 20);
	thermo->show();

        

	// set up the initial plot data
	for( int index=0; index<plotDataSize; ++index )
	{
		xData[index] = index;
		yData[index] = gain*1;
	}

	curve = new QwtPlotCurve;
	plot = new QwtPlot;
	// make a plot curve from the data and attach it to the plot
	curve->setSamples(xData, yData, plotDataSize);
	curve->attach(plot);

	plot->replot();
	plot->show();


	// set up the layout - knob above thermometer
	vLayout = new QVBoxLayout;
	vLayout->addWidget(knob);
	vLayout->addWidget(thermo);
	

	// plot to the left of knob and thermometer
	hLayout = new QHBoxLayout;
	hLayout->addLayout(vLayout);
	hLayout->addWidget(plot);

	setLayout(hLayout);

	// This is a demo for a thread which can be
	// used to read from the ADC asynchronously.
	// At the moment it doesn't do anything else than
	// running in an endless loop and which prints out "tick"
	// every second.
	adcreader = new ADCreader();
	adcreader->start();
}

Window::~Window() {
	// tells the thread to no longer run its endless loop
	adcreader->quit();
	// wait until the run method has terminated
	adcreader->wait();
//	delete adcreader;
}

void Window::timerEvent( QTimerEvent * )
{
	double inval;

	while(adcreader->hasSample())
	{
				
		inval=adcreader->getSample();
	
		// add the new input to the plot
		memmove( yData, yData+1, (plotDataSize-1) * sizeof(double) );
		yData[plotDataSize-1] = inval;
		curve->setSamples(xData, yData, plotDataSize);
		plot->replot();
		if(inval>=(-22000) && inval<=(-15000))
		{
			thermo->setValue( -(inval)/1000);//mapping thermo 0 to 20, no stress=0 and max stress=20
			system("omxplayer /home/pi/bleep_01.wav");
			
		}
		else if(inval>(-15000) && inval<=(10000))
		thermo->setValue(10);
		else 
		thermo->setValue(0);
	}
	// set the thermometer value
	
}


// this function can be used to change the gain of the A/D internal amplifier
void Window::setGain(double gain)
{
	// for example purposes just change the amplitude of the generated input
	this->gain = gain;
}
