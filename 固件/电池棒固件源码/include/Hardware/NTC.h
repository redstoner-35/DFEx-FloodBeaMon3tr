
/*-------------------------------------------------------------
 This is an automatically generated file by NTC resistor LUT 
 generator. DO NOT EDIT UNLESS YOU FULLY UNDERSTAND WHAT THIS
 FILE ACTUALLY DOES!
 NTC PARAMETER:50.00KΩ @ 25℃ B3950
 Table temperature range:-40℃ to 100℃
 Total ROM space for table:400 Bytes
 Target MCU Architecture:8051 Based MCU
-------------------------------------------------------------*/
#ifndef _NTC_
#define _NTC_
/*-------------------------------------------------------------
 Internal Include Files
-------------------------------------------------------------*/
#include <stdbool.h>

/*-------------------------------------------------------------
 Global Function prototype definition
-------------------------------------------------------------*/
char CalcNTCTemp(bool *IsNTCOK,unsigned long NTCRes);



#endif /* _NTC_ */
/*----------------------  End of File  ----------------------*/
