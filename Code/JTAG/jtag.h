


void jtag_SetClock(uint32 jtag_freq);
void jtag_init(void);
uint64 jtag_transfer(uint64 IrReg, uint32 IrRegLen, uint64 DrData, uint32 DrDataLen);

