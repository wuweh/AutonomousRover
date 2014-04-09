/*
 * main implementation: use this 'C' sample to create your own application
 *
 */




#include <stdio.h>
#include "derivative.h" /* include peripheral declarations */

void delay( int value )
{
	int i;
	for ( i = 0; i < value; i++ );
}

int main(void)
{
	GPIOD_PDDR |= 0x0002;
	GPIOD_PDOR |= 0x0002;
	for( ; ; );
	return 0;
}
