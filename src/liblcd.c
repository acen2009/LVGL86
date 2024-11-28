#include "Init.h"
#include "lcd.h"

extern unsigned long dpmi_LinMapAlloc(unsigned long phyaddr, unsigned long size);
extern void dpmi_LinMapFree(unsigned long linaddr);
extern int dpmi_SelAlloc(unsigned long linaddr, unsigned long size);
extern int dpmi_SelFree(int selector);

unsigned long g_FB_linaddress = 0;
int           g_FB_selector   = -1;

WORD pRelated_IOAddress;
DWORD* pFBBaseAddress;

void OutIOPort(WORD Index,BYTE Value)     
{
    WORD IOBase = pRelated_IOAddress+RELATED_IO_OFFSET+Index;
    
    outp(IOBase,Value);
}

BYTE InIOPort(WORD Index)     
{
    BYTE temp = 0x0;    
    WORD IOBase = pRelated_IOAddress+RELATED_IO_OFFSET+Index;
    
    temp = (BYTE)inp(IOBase);

    return temp;                                                 
}

void ResetATTR()
{
    InIOPort(COLOR_INPUT_STATUS1_READ);
    InIOPort(MONO_INPUT_STATUS1_READ);
}

void EnableATTR()
{
    ResetATTR();
    OutIOPort(ATTR_DATA_WRITE, 0x20);
    ResetATTR();
}

/*----------------------------------------------------------------------------
;
;       SetCRReg
;	In: Index, RegValue, Mask
;---------------------------------------------------------------------------*/
void SetCRReg(BYTE bRegIndex, BYTE bRegValue, BYTE bMask)
{
    BYTE btemp = 0x0,btemp1 = 0x0;
    
    if(bMask != 0xFF)
    {
        OutIOPort(COLOR_CRTC_INDEX,bRegIndex);
        btemp = (BYTE)InIOPort(COLOR_CRTC_DATA);
        bRegValue &= bMask;
        btemp &=~(bMask);
        btemp |= bRegValue;
        OutIOPort(COLOR_CRTC_DATA,btemp);
    }
    else
    {
        OutIOPort(COLOR_CRTC_INDEX,bRegIndex);
        OutIOPort(COLOR_CRTC_DATA,bRegValue);
    }
}

/*----------------------------------------------------------------------------
;
;       GetCRReg
;	In: Index
;	Out: RegValue
;---------------------------------------------------------------------------*/
BYTE GetCRReg(BYTE bRegIndex)
{
    BYTE btemp = 0x0;
    
    OutIOPort(COLOR_CRTC_INDEX,bRegIndex);
    btemp = (BYTE)InIOPort(COLOR_CRTC_DATA);
        
    return btemp;
}

void SetDACReg(WORD Index,BYTE Value)
{
    OutIOPort(Index,Value);
}


/*----------------------------------------------------------------------------
;
;       SetSRReg
;	In: Index, RegValue, Mask
;---------------------------------------------------------------------------*/
void SetSRReg(BYTE bRegIndex, BYTE bRegValue, BYTE bMask)
{
    BYTE btemp = 0x0,btemp1 = 0x0;
    
    if(bMask != 0xFF)
    {
        OutIOPort(SEQ_INDEX,bRegIndex);
        btemp = (BYTE)InIOPort(SEQ_DATA);
        bRegValue &= bMask;
        btemp &=~(bMask);
        btemp |= bRegValue;
        OutIOPort(SEQ_DATA,btemp);
    }
    else
    {
        OutIOPort(SEQ_INDEX,bRegIndex);
        OutIOPort(SEQ_DATA,bRegValue);
    }
}

/*----------------------------------------------------------------------------
;
;       GetSRReg
;	In: Index
;	Out: RegValue
;---------------------------------------------------------------------------*/
BYTE GetSRReg(BYTE bRegIndex)
{
    BYTE btemp = 0x0;
    
    OutIOPort(SEQ_INDEX,bRegIndex);
    btemp = (BYTE)InIOPort(SEQ_DATA);
        
    return btemp;
}

/*----------------------------------------------------------------------------
;
;       SetARReg
;	In: Index, RegValue
;---------------------------------------------------------------------------*/
void SetARReg(BYTE index,BYTE value)
{
    ResetATTR();

    OutIOPort(ATTR_DATA_WRITE,index);
    OutIOPort(ATTR_DATA_WRITE,value);

    InIOPort(COLOR_INPUT_STATUS1_READ);
    OutIOPort(ATTR_DATA_WRITE,BIT5);
}

/*----------------------------------------------------------------------------
;
;       GetARReg
;	In: Index
;	Out: RegValue
;---------------------------------------------------------------------------*/
BYTE GetARReg(BYTE index)
{
    BYTE bTmp;
    InIOPort(COLOR_INPUT_STATUS1_READ);
    OutIOPort(ATTR_DATA_WRITE,index);
    bTmp = (BYTE)InIOPort(ATTR_DATA_READ);
    InIOPort(COLOR_INPUT_STATUS1_READ);
    OutIOPort(ATTR_DATA_WRITE,BIT5);
    return bTmp;
}

/*----------------------------------------------------------------------------
;
;       SetGRReg
;	In: Index, Value, Mask
;---------------------------------------------------------------------------*/
void SetGRReg(BYTE bRegIndex, BYTE bRegValue, BYTE bMask)
{
    BYTE btemp = 0x0,btemp1 = 0x0;
    
    if(bMask != 0xFF)
    {
        OutIOPort(GRAPH_INDEX,bRegIndex);
        btemp = (BYTE)InIOPort(GRAPH_DATA);
        bRegValue &= bMask;
        btemp &=~(bMask);
        btemp |= bRegValue;
        OutIOPort(GRAPH_DATA,btemp);
    }
    else
    {
        OutIOPort(GRAPH_INDEX,bRegIndex);
        OutIOPort(GRAPH_DATA,bRegValue);
    }

    return;
}

/*----------------------------------------------------------------------------
;
;       SetMSReg
;	In: RegValue
;---------------------------------------------------------------------------*/
void SetMSReg(BYTE bRegValue)
{
    OutIOPort(MISC_WRITE,bRegValue);
}

/*----------------------------------------------------------------------------
;
;      SetEnableVGAReg
;	In: RegValue
;---------------------------------------------------------------------------*/
void SetEnableVGAReg(BYTE bRegValue)
{
    OutIOPort(VGA_ENABLE,bRegValue);
}


/*----------------------------------------------------------------------------
;
;       SerialLoadTable
;	Serial load register table
;---------------------------------------------------------------------------*/
void SerialLoadTable(REG_PACKAGE *pREG_PACKAGE_Table)
{
    REG_PACKAGE *pTemp_Reg = pREG_PACKAGE_Table;
    
    do{        
         switch(pTemp_Reg->RegGroup)
        {
            case SR:
                SetSRReg(pTemp_Reg->RegIndex, pTemp_Reg->RegValue, pTemp_Reg->RegMask);
                break;
            case CR:
                SetCRReg(pTemp_Reg->RegIndex, pTemp_Reg->RegValue, pTemp_Reg->RegMask);
                break;
            case GR:
                SetGRReg(pTemp_Reg->RegIndex, pTemp_Reg->RegValue, pTemp_Reg->RegMask);                
                break;
            case MR:
                SetMSReg(pTemp_Reg->RegValue);
                break;
            case AR:
                SetARReg(pTemp_Reg->RegIndex, pTemp_Reg->RegValue);                
                break;
            default:
                break;
        }
        pTemp_Reg++;
    }while(pTemp_Reg->RegGroup!=NR);
}

/*----------------------------------------------------------------------------
;
;       VideoPost --- First function for Init VGA
;	1. Enable 3C3
;	2. Switch IO port
;	3. Check DRAM clock
;	4. Set clock & init default register
;	5. Load DRAM timing
;---------------------------------------------------------------------------*/
void VideoPost(void)
{
    BYTE bStrapPin;
    BYTE bTemp, bTemp1;
    WORD i = 0;
    
    // Enable VGA, Switch I/O port to 3D4/3D5, UnLock extend CRTC register
    SetEnableVGAReg(BIT0);
    SetMSReg(0x23);
        
    SetCRReg(0x80, 0xA8, 0xFF);
    
   	// Disable standard VGA I/O
   	bTemp = GetCRReg(0xA1);
   	SetCRReg(0xA1, bTemp|0x2, 0xFF);
		
    //Load VGA default setting
    SerialLoadTable(ExtendRegs);   
}

/*----------------------------------------------------------------------------
;
;       SequencerOn / SequencerOff
;	Enable/ Disable VGA to fetch data from Memory
;---------------------------------------------------------------------------*/
void SequencerOn()
{
    SetSRReg(0x01, 0, BIT5);
}

void SequencerOff()
{
    SetSRReg(0x01, BIT5, BIT5);
}

void TurnOffScaler()
{
    SetSRReg(0x58, 0, BIT0);	
}

/*----------------------------------------------------------------------------
;
;       WaitPowerSequenceDone
;	Avoid to power on/off LCD panel too fast
;---------------------------------------------------------------------------*/
void WaitPowerSequenceDone()
{
    BYTE SR32;
     
    SR32 = (GetSRReg(0x32) & BIT2);

    // loop while power sequence status not changed
    while(SR32 != BIT2)
    {
    	SR32 = (GetSRReg(0x32) & BIT2);
    }
}

/*----------------------------------------------------------------------------
;
;       PowerSequenceOn / PowerSequenceOff
;
;---------------------------------------------------------------------------*/
void PowerSequenceOn()
{
    // check if power sequence already on
    if (!(GetSRReg(0x11) & BIT0))
    {
        // Power Sequence Control Pad Output Enable
        SetSRReg(0x32, BIT1, BIT1);
        
        // panel on
        SetSRReg(0x11, BIT0, BIT0);
        
        // delay for power sequence done
        WaitPowerSequenceDone();
    }
}


void PowerSequenceOff()
{
    // check if power sequence already off
    if (GetSRReg(0x11) & BIT0)
    {
        // panel off
        SetSRReg(0x11, 0, BIT0);
        
        // wait power sequence done
        WaitPowerSequenceDone();
        
        // Power Sequence Control Pad Output Disable
        SetSRReg(0x32, 0, BIT1);
    }
}

/*----------------------------------------------------------------------------
;
;       UnLockCR0ToCR7 / LockCR0ToCR7
;
;---------------------------------------------------------------------------*/
void UnLockCR0ToCR7()
{
    // CR11[7] = 0
    SetCRReg(0x11, 0x00, BIT7);
}

void LockCR0ToCR7()
{
    // CR11[7] = 1
    SetCRReg(0x11, 0x80, BIT7);
}

/*----------------------------------------------------------------------------
;
;       LoadDisplay1VESAModeInitRegs
;
;       1. Program MISC register
;	2. Unlock
;	3. Init VESA mode registers
;	4. Init AR registers
;---------------------------------------------------------------------------*/
void LoadDisplay1VESAModeInitRegs()
{
    OutIOPort(MISC_WRITE, 0x2F);

    UnLockCR0ToCR7();
    
    SerialLoadTable(Display1VESAModeInitRegs);

    ResetATTR();

    SetARReg(0x10, 0x01);
    SetARReg(0x11, 0x00);
    SetARReg(0x12, 0x00);
    SetARReg(0x13, 0x00);
    SetARReg(0x14, 0x00);

    EnableATTR();
}
/*----------------------------------------------------------------------------
;
;       WriteRegToHW
;
;       in: 
;           pRegOp = pointer to REG_OP table
;           value = value to be written
;
;---------------------------------------------------------------------------*/
void WriteRegToHW(REG_OP *pRegOp, WORD value)
{
    BYTE btemp, btemp1;
    BYTE bCount;
    BYTE bMasktemp;

    while((pRegOp->RegGroup)!= NR)
    {
        btemp = 0x0; btemp1 = 0x0;
        bCount = 0x0; bMasktemp = 0x0;
    
        bMasktemp = (pRegOp->RegMask);

        // Shift all register into LSB.
        while(!(bMasktemp & BIT0))
        {
            bMasktemp = bMasktemp >> 1;
            bCount ++;
        }
    
        // Calculate the value that we need to set into register.
        btemp = value >> (pRegOp->RegShiftBit);
        btemp &= (bMasktemp);

        // Shift the data bit into right position.
        if(!(pRegOp->RegMask & BIT0))
        {
            btemp = btemp << bCount;
        }
            
        if(pRegOp->RegGroup == CR)
        {
            //CR
            OutIOPort(COLOR_CRTC_INDEX,(pRegOp->RegIndex));
            btemp1 = (BYTE)InIOPort(COLOR_CRTC_DATA);
            btemp1 &= ~(pRegOp->RegMask);
            btemp1 |= btemp;
            OutIOPort(COLOR_CRTC_DATA,btemp1);
        }
        else
        {
            //SR
            OutIOPort(SEQ_INDEX,(pRegOp->RegIndex));
            btemp1 = (BYTE)InIOPort(SEQ_DATA);
            btemp1 &= ~(pRegOp->RegMask);
            btemp1 |= btemp;
            OutIOPort(SEQ_DATA,btemp1);            
        }

        pRegOp++;
        
    }
 
}

/*-------------------------------------------------------------------
;
;       SetHTotal()
;
;       in:
;           DisplayPath = display path
;           Value = H total in pixel
;
;------------------------------------------------------------------*/
void SetHTotal(BYTE DisplayPath, WORD Value)
{
    // Subtract the 40 pixels for setting HW
    Value -= 40;

    if(DisplayPath == DISP1)
    {
        WriteRegToHW(HTotal1, Value);
    }
}

/*-------------------------------------------------------------------
;
;       SetHDisplayEnd()
;
;       in:
;           DisplayPath = display path
;           Value = H display end in pixel
;
;------------------------------------------------------------------*/
void SetHDisplayEnd(BYTE DisplayPath, WORD Value)
{
    // Subtract the 8 pixels for setting HW
    Value -= 8;

    if(DisplayPath == DISP1)
    {
        WriteRegToHW(HDispEnd1, Value);
    }
}

/*-------------------------------------------------------------------
;
;       SetHBlankingStart()
;
;       in:
;           DisplayPath = display path
;           Value = H blanking start in pixel
;
;------------------------------------------------------------------*/
void SetHBlankingStart(BYTE DisplayPath, WORD Value)
{
    // Subtract the 8 pixels for setting HW
    Value -= 8;

    if(DisplayPath == DISP1)
    {
        WriteRegToHW(HBnkSt1, Value);
    }
}

/*-------------------------------------------------------------------
;
;       SetHBlankingEnd()
;
;       in:
;           DisplayPath = display path
;           Value = H blanking end in pixel
;
;------------------------------------------------------------------*/
void SetHBlankingEnd(BYTE DisplayPath, WORD Value)
{
    // Subtract the 8 pixels for setting HW
    Value -= 8;

    if(DisplayPath == DISP1)
    {
        WriteRegToHW(HBnkEnd1, Value);
    }   
}

/*-------------------------------------------------------------------
;
;       SetHSyncStart()
;
;       in:
;           DisplayPath = display path
;           Value = H Sync start in pixel
;
;------------------------------------------------------------------*/
void SetHSyncStart(BYTE DisplayPath, WORD Value)
{
    // Subtract the 0 pixels for setting HW
    Value -= 0;

    if(DisplayPath == DISP1)
    {
        WriteRegToHW(HSyncSt1, Value);
    } 
}

/*-------------------------------------------------------------------
;
;       SetHSyncEnd()
;
;       in:
;           DisplayPath = display path
;           Value = H Sync end in pixel
;
;------------------------------------------------------------------*/
void SetHSyncEnd(BYTE DisplayPath, WORD Value)
{
    // Subtract the 0 pixels for setting HW
    Value -= 0;

    if(DisplayPath == DISP1)
    {
        WriteRegToHW(HSyncEnd1, Value);
    }
}

/*-------------------------------------------------------------------
;
;       SetVTotal()
;
;       in:
;           DisplayPath = display path
;           Value = V total
;
;------------------------------------------------------------------*/
void SetVTotal(BYTE DisplayPath, WORD Value)
{
    // Subtract the 2 line for setting HW
    Value -= 2;

    if(DisplayPath == DISP1)
    {
        WriteRegToHW(VTotal1, Value);
    } 
}

/*-------------------------------------------------------------------
;
;       SetVDisplayEnd()
;
;       in:
;           DisplayPath = display path
;           Value = V display end
;
;------------------------------------------------------------------*/
void SetVDisplayEnd(BYTE DisplayPath, WORD Value)
{
    // Subtract the 1 line for setting HW
    Value -= 1;

    if(DisplayPath == DISP1)
    {
        WriteRegToHW(VDispEnd1, Value);
    }
}

/*-------------------------------------------------------------------
;
;       SetVBlankingStart()
;
;       in:
;           DisplayPath = display path
;           Value = V blanking start
;
;------------------------------------------------------------------*/
void SetVBlankingStart(BYTE DisplayPath, WORD Value)
{
    // Subtract the 1 line for setting HW
    Value -= 1;

    if(DisplayPath == DISP1)
    {
        WriteRegToHW(VBnkSt1, Value);
    }
}

/*	dpmi_LinMapAlloc:
		allocate linear mapping memory via dpmi function.
	parameter:
		phyaddr - physical address
		size    - memory size
	return:
		0 - fail
		other - success
*/
// unsigned long dpmi_LinMapAlloc(unsigned long phyaddr, unsigned long size){
	// __dpmi_meminfo meminfo;
	
	// if(phyaddr < 0x100000L)
		// return phyaddr; // only needs to map non-DOS memory (DPMI host has already mapped DOS memory below 1Mb)
	// meminfo.address = phyaddr;
	// meminfo.size    = size;
	// if(__dpmi_physical_address_mapping(&meminfo) != 0)
		// __dpmi_lock_linear_region(&meminfo);
	// return meminfo.address;
// }

/* dpmi_LinMapFree
		free linear mapping memory via dpmi function.
	parameter:
		linaddr - dpmi allocated linear memory
*/
// void dpmi_LinMapFree(unsigned long linaddr) {
        // __dpmi_meminfo meminfo;

        // if (linaddr >= 0x100000L)
        // {
            // meminfo.address = linaddr;
            // __dpmi_free_physical_address_mapping(&meminfo);
        // }
// }

/* dpmi_SelAlloc
		allocate selector for memory segment
	parameter:
		linaddr - linear memory address 
		size    - memory size
	return:
		-1 - fail
		others - success
*/
// int dpmi_SelAlloc(unsigned long linaddr, unsigned long size) {
        // int selector;
    
        // if ((selector = __dpmi_allocate_ldt_descriptors(1)) < 0) return -1;
    
        // if (__dpmi_set_segment_base_address(selector, linaddr) == 0) //&&
        // if (__dpmi_set_segment_limit(selector, size - 1) == 0)
            // return selector;

        // __dpmi_free_ldt_descriptor(selector);
        // return -1;
// }

/* dpmi_SelAlloc
		free selector for memory segment
	parameter:
		selector - selector allocated via dpmi function
	return:
		FALSE - fail
		TRUE  - success
*/
// unsigned char dpmi_SelFree(int selector) {
        // if (selector < 0) return 1;

        // if (__dpmi_free_ldt_descriptor(selector) != 0) 
			// return 0;
        // return 1;
// }

/*-------------------------------------------------------------------
;
;       SetVBlankingEnd()
;
;       in:
;           DisplayPath = display path
;           Value = V blanking end
;
;------------------------------------------------------------------*/
void SetVBlankingEnd(BYTE DisplayPath, WORD Value)
{
    // Subtract the 1 line for setting HW
    Value -= 1;

    if(DisplayPath == DISP1)
    {
        WriteRegToHW(VBnkEnd1, Value);
    }   
}

/*-------------------------------------------------------------------
;
;       SetVSyncStart()
;
;       in:
;           DisplayPath = display path
;           Value = V sync start
;
;------------------------------------------------------------------*/
void SetVSyncStart(BYTE DisplayPath, WORD Value)
{
    // Subtract the 1 line for setting HW
    Value -= 1;

    if(DisplayPath == DISP1)
    {
        WriteRegToHW(VSyncSt1, Value);
    }  
}

/*-------------------------------------------------------------------
;
;       SetVSyncEnd()
;
;       in:
;           DisplayPath = display path
;           Value = V sync end
;
;------------------------------------------------------------------*/
void SetVSyncEnd(BYTE DisplayPath, WORD Value)
{
    // Subtract the 1 line for setting HW
    Value -= 1;

    if(DisplayPath == DISP1)
    {
        WriteRegToHW(VSyncEnd1, Value);
    }  
}

DWORD Difference(DWORD Value1, DWORD Value2)
{
    if (Value1 > Value2)
        return (Value1 - Value2);
    else
        return (Value2 - Value1);
}

/*-------------------------------------------------------------------
;
;       ClockToPLLF4002A()
;
;       in:
;           Clock = pixel clock in khz
;       return:
;           PLL_Info = PLL value
;
;------------------------------------------------------------------*/
PLL_Info ClockToPLLF4002A(DWORD Clock)
{
    DWORD MSCount, NSCount, RSCount, FCKVCO, FCKOUT;    //defined in PLL module
    DWORD dwDiff = Clock * 5 / 1000; // The diff is in 0.5%
    PLL_Info PLLInfo = {0,0,0};
    BYTE bFound = 0;

    for (MSCount = 1; MSCount <= 7; MSCount++)
    {
        for (NSCount = 1; NSCount <= 255; NSCount++)
        {
            FCKVCO = PLLReferenceClock * NSCount / MSCount;
            
            if ((MaxFCKVCO4002A >= FCKVCO) && 
                (FCKVCO >= MinFCKVCO4002A) &&
                (bFound == 0))
            {
                for (RSCount = 0; RSCount <= 7; RSCount++)
                {
                    FCKOUT = FCKVCO >> RSCount;
                    if ((Difference(FCKOUT, Clock) <= dwDiff)&&
                        (bFound == 0))
                    {
                        bFound = 1;
                        PLLInfo.MS = (BYTE)MSCount;
                        PLLInfo.NS = (BYTE)NSCount;
                        PLLInfo.RS = (BYTE)RSCount;
                    }
                }
            }
        }

        if (bFound)
            break;
    }

    // Shift for setting.
    PLLInfo.MS <<=3;
    return PLLInfo;
}


/*-------------------------------------------------------------------
;
;       ClockToPLLF9003A()
;
;       in:
;           Clock = pixel clock in khz
;       return:
;           PLL_Info = PLL value
;
;------------------------------------------------------------------*/
PLL_Info ClockToPLLF9003A(DWORD Clock)
{
    DWORD MSCount, NSCount, RSCount, FCKVCO, FCKOUT;    //defined in PLL module
    DWORD NearestClock = 0xFFFFFFFF;
    PLL_Info PLLInfo;
    
    for (MSCount = 3; MSCount < 6; MSCount++)
    {
        for (NSCount = 1; NSCount < 256; NSCount++)
        {
            FCKVCO = PLLReferenceClock * NSCount / MSCount;
            
            if ( (MaxFCKVCO9003A >= FCKVCO) && (FCKVCO >= MinFCKVCO9003A) )
            {
                for (RSCount = 1; RSCount < 6; RSCount++)
                {
                    FCKOUT = FCKVCO >> RSCount;
                    if ( Difference(FCKOUT, Clock) < Difference(NearestClock, Clock) )
                    {
                        NearestClock = FCKOUT;
                        if (MSCount == 3)
                            PLLInfo.MS = 0x00;
                        if (MSCount == 4)
                            PLLInfo.MS = BIT3;
                        if (MSCount == 5)
                            PLLInfo.MS = BIT4+BIT3;
                        PLLInfo.NS = (BYTE)NSCount;
                        PLLInfo.RS = (BYTE)RSCount-1;
                    }
                }
            }
        }

    }

    return PLLInfo;
}

/*-------------------------------------------------------------------
;
;       SetDPLL() - set display PLL
;
;       in:
;           DisplayPath = display path
;           PLLInfo: PLL value
;
;------------------------------------------------------------------*
void SetDPLL(BYTE DisplayPath, PLL_Info PLLInfo)
{
    BYTE RetValue; 
/*
    if (DisplayPath == DISP1)
    {
        SetCRReg(0xC1, PLLInfo.MS, BIT4+BIT3);
        SetCRReg(0xC0, PLLInfo.NS, 0xFF);
        SetCRReg(0xCF, PLLInfo.RS, BIT2+BIT1+BIT0);
    }
    else
    {
        SetCRReg(0xBF, PLLInfo.MS, BIT4+BIT3);
        SetCRReg(0xBE, PLLInfo.NS, 0xFF);
        SetCRReg(0xCE, PLLInfo.RS, BIT2+BIT1+BIT0);
    }

    // Fire the PLL setting
    RetValue = GetCRReg(0xBB);
    SetCRReg(0xBB, RetValue, 0xFF);
}*/
void SetDPLL(BYTE DisplayPath, PLL_Info PLLInfo, DWORD Clock)
{
    BYTE RetValue;

    SetCRReg(0xC1, 0xFF, BIT4+BIT3);
    SetCRReg(0xC0, 1200/(Clock/1000), 0xFF);
    // SetCRReg(0xC0, 27, 0xFF);  //27
    SetCRReg(0xCF, 0, BIT2+BIT1+BIT0);

    RetValue = GetCRReg(0xBB);
    SetCRReg(0xBB, RetValue, 0xFF);
}


/*-------------------------------------------------------------------
;
;       SetPixelClock()
;
;       in:
;           DisplayPath = display path
;           Clock = pixel clock in khz
;
;------------------------------------------------------------------*/
void SetPixelClock(BYTE DisplayPath, DWORD Clock)
{
    PLL_Info PLLInfo;
    
    //PLLInfo = ClockToPLLF4002A(Clock);
    //PLLInfo = ClockToPLLF9003A(Clock);
    SetDPLL(DisplayPath, PLLInfo, Clock);
}
/*-------------------------------------------------------------------
;
;       SetPolarity()
;
;       in:
;           DisplayPath = display path
;           Value: bit1 = 1 for positive H sync
;                       = 0 for negative H sync
;                  bit2 = 1 for positive V sync
;                       = 0 for negative V sync
;------------------------------------------------------------------*/
void SetPolarity(BYTE Value)
{
    Value ^= BIT2+BIT1; // invert Bit[2:1]    
    
    OutIOPort(MISC_WRITE, ((Value<<5) & 0xC0) | (InIOPort(MISC_READ) & 0x3F));

    SetSRReg(0x20, Value>>1, BIT1+BIT0);

    SetSRReg(0x20, Value<<2, BIT4+BIT3);
}

/*-------------------------------------------------------------------
;
;       SetFIFO()
;
;       in:
;           DisplayPath = display path
;       out:
;           none
;------------------------------------------------------------------*/
void SetFIFO(BYTE DisplayPath)
{
    if (DisplayPath == DISP1)
    {
        SetCRReg(0xA7, 0x5F, 0xFF);
        SetCRReg(0xA6, 0x3F, 0xFF);
    }
}

/*-------------------------------------------------------------------
;
;       SetPitch()
;
;       in:
;           DisplayPath = display path
;           Value = Pitch in byte
;
;------------------------------------------------------------------*/
void SetPitch(BYTE DisplayPath, WORD Value)
{
    // adjust pitch in 8 byte alignment
    Value += 7;
    Value >>= 3;

    if(DisplayPath == DISP1)
    {
        WriteRegToHW(Pitch1, Value);
    }
}

/*----------------------------------------------------------------------------
;
;       SetTimingRegs
;
;       in: 
;           DisplayPath = display path
;           pMODE_INFO = pointer to MODE_INFO
;           pRRATE_TABLE = pointer to RRATE_TABLE
;
;---------------------------------------------------------------------------*/
void SetTimingRegs(BYTE DisplayPath, MODE_INFO *pModeInfo, RRATE_TABLE *pRRateTable)
{
    WORD Attribute = pRRateTable->Attribute;
   
    SetHTotal(DisplayPath, pRRateTable->H_Total);

    SetHBlankingEnd(DisplayPath, (Attribute & HB) ? pRRateTable->H_Total-8 : pRRateTable->H_Total);

    SetHDisplayEnd(DisplayPath, pModeInfo->H_Size);

    SetHBlankingStart(DisplayPath, (Attribute & HB) ? pModeInfo->H_Size+8 : pModeInfo->H_Size);

    SetHSyncStart(DisplayPath, pRRateTable->H_Sync_Start);

    SetHSyncEnd(DisplayPath, pRRateTable->H_Sync_End);

    SetVTotal(DisplayPath, pRRateTable->V_Total);

    SetVBlankingEnd(DisplayPath, (Attribute & VB) ? pRRateTable->V_Total-8 : pRRateTable->V_Total);

    SetVDisplayEnd(DisplayPath, pModeInfo->V_Size);

    SetVBlankingStart(DisplayPath, (Attribute & VB) ? pModeInfo->V_Size+8 : pModeInfo->V_Size);

    SetVSyncStart(DisplayPath, pRRateTable->V_Sync_Start);

    SetVSyncEnd(DisplayPath, pRRateTable->V_Sync_End);
    
    SetPixelClock(DisplayPath, pRRateTable->Clock);

    SetPolarity((BYTE)pRRateTable->Attribute);    
}

/*----------------------------------------------------------------------------
;
;       LoadTimingRegs
;
;       in: 
;           WORD ModeNum (VESA Mode Number)
;---------------------------------------------------------------------------*/
extern void timer_Delay(unsigned long ms);
void LoadTiming(WORD Width, WORD Height)
{
    BYTE i,PanelID,bR_Rate_value = 0x0;
    PANEL_INFO *pPanelInfo = NULL;
    PANEL_TABLE *pPanelTable = NULL;
    MODE_INFO pModeInfo;
    RRATE_TABLE pRRateTable;
    
    for (i=0; i < LCD_TABLE_LEN; i++)
    {
        pPanelInfo  = &(LCDTable[i]);  	
        if ((pPanelInfo->H_Size == Width) && (pPanelInfo->V_Size == Height))
            break;     
    }
    
    timer_Delay(100); // garbage code, but we need this for v86boot.exe, if no it, will often get whole white screen when power on.

    pPanelTable = &(pPanelInfo->pPanelTable[0]);  
    pRRateTable = pPanelTable->Timing;
    pModeInfo.H_Size = pPanelInfo->H_Size;
    pModeInfo.V_Size = pPanelInfo->V_Size;
    SetTimingRegs(1,&pModeInfo,&pRRateTable);
}

/*----------------------------------------------------------------------------
;
;       Set Power Sequence Timer
;	ConvertMStoCount(WORD)
;	CBIOSSetTD0 to set TD0
;	CBIOSSetTD1 to set TD1
;	CBIOSSetTD2 to set TD2
;	CBIOSSetTD3 to set TD3
;	CBIOSSetTD5 to set TD5
;	CBIOSSetTD6 to set TD6
;	CBIOSSetTD7 to set TD7
;---------------------------------------------------------------------------*/
WORD ConvertMStoCount(WORD wTDX)
{
    WORD  wTmp1;
    DWORD dwTmp2;
    wTmp1= wTDX>>6;
    wTDX<<=10;
    dwTmp2= (wTmp1<<16) + wTDX;
    return (WORD)(dwTmp2/293);
}

void CBIOSSetTD0(WORD wTD0)
{
    WORD wSetReg;
    BYTE ucHighByte,ucLowByte;
    wSetReg=ConvertMStoCount(wTD0);
    ucHighByte=wSetReg>>8;
    ucLowByte =(BYTE)wSetReg;
    SetSRReg(0x12,ucLowByte ,0xFF);
    SetSRReg(0x1A,ucHighByte,0x0F);
}

void CBIOSSetTD1(WORD wTD1)
{
    WORD wSetReg;
    BYTE ucHighByte,ucLowByte;
    wSetReg=ConvertMStoCount(wTD1);
    ucHighByte=wSetReg>>4;
    ucLowByte =(BYTE)wSetReg;
    SetSRReg(0x13,ucLowByte ,0xFF);
    SetSRReg(0x1A,ucHighByte,0xF0);
}

void CBIOSSetTD2(WORD wTD2)
{
    WORD wSetReg;
    BYTE ucHighByte,ucLowByte;
    wSetReg=ConvertMStoCount(wTD2);
    ucHighByte=wSetReg>>8;
    ucLowByte =(BYTE)wSetReg;
    SetSRReg(0x14,ucLowByte ,0xFF);
    SetSRReg(0x1B,ucHighByte,0x0F);
}

void CBIOSSetTD3(WORD wTD3)
{
    WORD wSetReg;
    BYTE ucHighByte,ucLowByte;
    wSetReg=ConvertMStoCount(wTD3);
    ucHighByte=wSetReg>>4;
    ucLowByte =(BYTE)wSetReg;
    SetSRReg(0x15,ucLowByte ,0xFF);
    SetSRReg(0x1B,ucHighByte,0xF0);
}

void CBIOSSetTD5(WORD wTD5)
{
    WORD wSetReg;
    BYTE ucHighByte,ucLowByte;
    wSetReg=ConvertMStoCount(wTD5);
    ucHighByte=wSetReg>>4;
    ucLowByte =(BYTE)wSetReg;
    SetSRReg(0x17,ucLowByte ,0xFF);
    SetSRReg(0x1C,ucHighByte,0xF0);
}

void CBIOSSetTD6(WORD wTD6)
{
    WORD wSetReg;
    BYTE ucHighByte,ucLowByte;
    wSetReg=ConvertMStoCount(wTD6);
    ucHighByte=wSetReg>>8;
    ucLowByte =(BYTE)wSetReg;
    SetSRReg(0x18,ucLowByte ,0xFF);
    SetSRReg(0x1D,ucHighByte,0x0F);
}

void CBIOSSetTD7(WORD wTD7)
{
    WORD wSetReg;
    BYTE ucHighByte,ucLowByte;
    wSetReg=ConvertMStoCount(wTD7);
    ucHighByte=wSetReg>>8;
    ucLowByte =(BYTE)wSetReg;
    SetSRReg(0x19,ucLowByte ,0xFF);
    SetSRReg(0x1D,ucHighByte,0xF0);
}

/*----------------------------------------------------------------------------
;
;       InitLCD
;	Default LCDTableIndex = 0, 1024x600
;           LCDTableIndex = 1, 800x480
;           LCDTableIndex = 2, 640x480
;	If want to add other LCD panel table, please add LCDTable
;       1. Init Power Sequence timing
;---------------------------------------------------------------------------*/	
void InitLCD(int index)
{
    BYTE LCDTableIndex = 0;
    
    // Set power sequence timer
    CBIOSSetTD0(LCDTable[index].pPanelTable->TD0);
    CBIOSSetTD1(LCDTable[index].pPanelTable->TD1);
    CBIOSSetTD2(LCDTable[index].pPanelTable->TD2);
    CBIOSSetTD3(LCDTable[index].pPanelTable->TD3);
    CBIOSSetTD5(LCDTable[index].pPanelTable->TD5);
    CBIOSSetTD6(LCDTable[index].pPanelTable->TD6);
    CBIOSSetTD7(LCDTable[index].pPanelTable->TD7);
    
    SetSRReg(0x32,0,BIT0);  //Disable Power Sequence Bypass
    
    if(LCDTable[index].pPanelTable->Timing.Attribute & ROT) //wAttribute[6] = 1/0, Rotation/Sequential
    {
        SetSRReg(0x1E,BIT0,BIT0); //Set Rotation
    }
    else
    {
        SetSRReg(0x1E,0,BIT0);   //Set Sequential
    }

    if(LCDTable[index].pPanelTable->Timing.Attribute & _2_CH) //Set 24 /12 bit LVDS or 18bit TTL
    {
        SetSRReg(0x1E,BIT3,BIT3); //Set 24 bit LVDS or 18bit TTL
    }
    else
    {
        SetSRReg(0x1E,0,BIT3);   //Set 12 bit LVDS
    }

/*
    if(LCDTable[0].pPanelTable->Timing.Attribute & MSB) //wAttribute[6] = 1/0, Rotation/Sequential
    {
        SetSRReg(0x1E,0,BIT2);       //Set SPWG mode ( Data is the same as TMDS channel)
    }
    else
    {
        SetSRReg(0x1E,BIT2,BIT2);   //Set OPENLDI mode
    }
*/
SetSRReg(0x1E,0,BIT2);
}

/*-------------------------------------------------------------------
;
;       SetColorDepth()
;
;       in:
;           DisplayPath = display path
;           Value = color depth (8/16/32)
;
;------------------------------------------------------------------*/
void SetColorDepth(BYTE DisplayPath, BYTE Value)
{
    BYTE bSetBit = 0x0;

    switch(Value)
    {
        case 8:
            bSetBit = (BYTE)BIT0;
            break;
        case 16:
            bSetBit = (BYTE)BIT2;
            break;
        case 32:
            bSetBit = (BYTE)BIT3;
            break;
        default:
            return;
    }

    if(DisplayPath == DISP1)
    {
        SetCRReg(0xA3, 0x0, BIT0+BIT1+BIT2+BIT3); //Clear ColorDepth first
        SetCRReg(0xA3, bSetBit, bSetBit);
    }
}

void EnableDigitalPad()
{
    //Turn ON DVP1 pad    
    SetCRReg(0xA3, BIT7, BIT7);
    // Turn ON DVP2 pad
    SetCRReg(0xA3, BIT6, BIT6);
}

/*-------------------------------------------------------------------
;
;       SetVESAMode()
;       Default set vesa mode 800x600x8 Mode Num 0x103
;       ColorDepth = 8, 
;       Pitch = (800x(ColorDepth >> 3) + 7) >> 3, 8 byte alignment 
;
;------------------------------------------------------------------*/
void SetVESAMode()
{
    BYTE ColorDepth = DRC;
    WORD Width = DRW; 
    WORD Height = DRH;
    WORD Pitch = (Width * (ColorDepth >> 3));
        
    SequencerOff();
#if ENABLELCD    
    TurnOffScaler();            
    PowerSequenceOff();
#endif    
    LoadDisplay1VESAModeInitRegs();
    LoadTiming(Width, Height);
    SetPitch(DISP1, Pitch);
    SetColorDepth(DISP1, ColorDepth);
#if ENABLELCD    
    PowerSequenceOn();
    EnableDigitalPad();
#endif    
    SequencerOn();
}


void InitDAC()
{
    WORD i = 0;
    
    // Enable DAC Mask
    SetDACReg(PEL_MASK, 0xFF);
    SetDACReg(DAC_INDEX_WRITE, i);
      
    for (i = 0; i < 256; i++)
    {
        SetDACReg(DAC_DATA, DAC[i].R);
        SetDACReg(DAC_DATA, DAC[i].G);
        SetDACReg(DAC_DATA, DAC[i].B);
    } 
}

void DrawPattern(LCDINFO *pLCDInfo)
{
    DWORD i,j;
    DWORD Width,Hight,Pitch,CD;
    
    //BYTE* pFBBase;
    
    //pFBBase = (BYTE*)pFBBaseAddress;
    Width = DRW;
    Hight = DRH;
    CD  = DRC >> 3;
    Pitch = Width << CD;
    
	_farsetsel(g_FB_selector);
	for(j = 0; j < Hight; j++)
	{
		for(i = 0; i < Pitch; i++)
		{
			_farnspokeb(i + j*Pitch, 0x0A);
		}
	}
	/*
    // Clean On-Screen
    for (j = 0; j < Hight; j++)
        for(i = 0; i < Pitch; i++)
            *(pFBBase + j*Pitch + i) = 0x00;
            
    for (j = 0; j < Hight; j++)
    {
        for (i = 0; i < Pitch; i++)
        {
            *(pFBBase + j*Pitch + i) = 0x0A;  
        }
    }
	*/
}

void EnableCRTPath()
{
    //Turn ON DAC & H, V sync    
    SetCRReg(0xDF, 0x00, BIT2);
    // Turn OM Analog pad
    SetCRReg(0xA8, 0x00, BIT7);
}



void ChipInfo(LCDINFO *LCDInfo)
{
    DWORD   dwBus, dwDev, dwFunc, dwPCICMD, dwClassCode, dwCMDReg,EnableIO, dwFBSize, tmp;
    DWORD   dwBaseReg[5];
    DWORD   dwTemp = 0;
    DWORD   dwTempSzie = 0xFFFFFFFF;
    BYTE    bGetit = 0;
    BYTE    ExpNum = 0;
    BYTE    RegIndex = 0;
    BYTE    i = 0;

    memset(&dwBaseReg,0,6*sizeof(DWORD));
    
    for (dwBus = 0; dwBus < 256; dwBus++)
    {
        for (dwDev = 0; dwDev < 32; dwDev++)
        {
            for(dwFunc = 0; dwFunc < 8; dwFunc++)
            {
                RegIndex = 0x8;
                dwPCICMD = 0x80000000 | 
                           (dwBus << 16) | 
                           (dwDev << 11) | 
                           (dwFunc << 8);
                                
                            
                outpd(0xCF8, dwPCICMD+RegIndex);
                dwClassCode = inpd(0xCFC);
                dwClassCode &= 0xFFFFFFFF00;

                RegIndex = 0x4;
                outpd(0xCF8, dwPCICMD+RegIndex);
                dwCMDReg = inpd(0xCFC);
                dwCMDReg &= 0x00000001;

                {

                    RegIndex = 0x0;
                    outpd(0xCF8, dwPCICMD + RegIndex);
                    dwTemp = inpd(0xCFC);   
                    
                    if (dwTemp == DIDVID)
                    {
                        RegIndex = 0x4;
                        outpd(0xCF8, dwPCICMD+RegIndex);
                        EnableIO = inpd(0xCFC)|0x3;
                        outpd(0xCFC, EnableIO);
                        
                        bGetit = 1;
                        LCDInfo->dwBus=dwBus;
                        LCDInfo->dwDev=dwDev;
                        LCDInfo->dwFunc=dwFunc;                    
                        break; 
                    }
                }
            }

            if (bGetit != 0)
                break;
        }

        if (bGetit != 0)
            break;
    }

    if(bGetit == 1)
    {
        RegIndex = 0x10;
        outpd(0xCF8, dwPCICMD + RegIndex);
        dwBaseReg[0] = inpd(0xCFC);    
        pFBBaseAddress = (DWORD*)(dwBaseReg[0] & 0xFFFFFFF8);

		// vic:
		disable();
		outportl(0xCF8, (0x8000004B & 0xFFFFFFFC));
		tmp = inportl(0xCFC);
		enable();
		
		tmp >>= 24;
		tmp >>= 4;
		tmp &= 0x07;
		switch(tmp)
		{
		case 0:
			dwFBSize = 2 * 1024 * 1024;
			break;
		case 1:
			dwFBSize = 4 * 1024 * 1024;
			break;
		case 2:
			dwFBSize = 8 * 1024 * 1024;
			break;
		case 3:
			dwFBSize = 16 * 1024 * 1024;
			break;
		case 4:
			dwFBSize = 32 * 1024 * 1024;
			break;
		case 5:
			dwFBSize = 64 * 1024 * 1024;
			break;
		}
		
		g_FB_linaddress = dpmi_LinMapAlloc((DWORD)pFBBaseAddress, dwFBSize);
		if(g_FB_linaddress == 0)
		{
			printf("dpmi lin map alloc failed.\n");
			exit(1);
		}
		
		g_FB_selector = dpmi_SelAlloc(g_FB_linaddress, dwFBSize);
		if(g_FB_selector == -1)
		{
			dpmi_LinMapFree(g_FB_linaddress);
			printf("dpmi_SelAlloc failed.\n");
			exit(1);
		}
		
        RegIndex = 0x18;
        outpd(0xCF8, dwPCICMD + RegIndex);
        pRelated_IOAddress = inpd(0xCFC)&0xFFFFFFFE;   

		// Special check
		// If we using this on A9133 LCD controller
		// SW should check NB setting for frame buffer access
		// System also need to check total system memory size
		// It would be affect by LCD controller.
        /*
		if(DIDVID == 0x250017F3)
		{
			outpd(0xCF8, M2500LCD);
			dwTemp = inpd(0xCFC);    
			dwTemp = 0x33000000;
			outpd(0xCFC, dwTemp);
		}
        */
    }
    else
    {
        exit(1);
    }
}

void lcd_draw(unsigned short color)
{
	DWORD i,j;
    DWORD Width,Hight,Pitch,CD;
    
    //BYTE* pFBBase;
    
    //pFBBase = (BYTE*)pFBBaseAddress;
    Width = DRW;
    Hight = DRH;
    CD  = DRC >> 3;
    Pitch = Width * CD;
    
	_farsetsel(g_FB_selector);
	for(j = 0; j < Hight; j++)
	{
		for(i = 0; i < Pitch; i+=2)
		{
			_farnspokew(i + j*Pitch, color);
		}
	}
}

void lcd_on(int index)
{
	LCDINFO      LCDInfo;
    memset(&LCDInfo, 0, sizeof(LCDINFO));
	
    ChipInfo(&LCDInfo);
    lcd_draw(0);
    
    VideoPost();
#if ENABLELCD    
    InitLCD(index);     // This function is only for LCD panel
#endif    
    SetVESAMode();
    InitDAC();    
    EnableCRTPath();
}



/*
void lcd_draw_xy(int x, int y, unsigned short color)
{
    DWORD Width,Pitch,CD;
    
    Width = DRW;
    CD  = DRC >> 3;
    Pitch = Width * CD;
    
    _farsetsel(g_FB_selector);
    _farnspokew(x*CD + y*Pitch, color);
}
*/

int get_lcd_fb_selector(void) {
    return g_FB_selector;
}

void lcd_off()
{
	SequencerOff();

    TurnOffScaler();            
    PowerSequenceOff();
	dpmi_SelFree(g_FB_selector);
	dpmi_LinMapFree(g_FB_linaddress);
}

void lcd_backlight(int value) {
    
    ExtendRegs[19].RegValue = value;
    VideoPost();
    SetVESAMode();
}

/*
#define RED   0x04
#define GREEN 0x02
#define BLUE  0x01

int main(int argc, char *argv[])
{
	unsigned char colors[] = {RED, GREEN, BLUE};
	lcd_on();
	int i = 0;
	while(1)
	{
		lcd_draw(colors[i]);
		if(getch() == 27)
			break;
		i++;
		i%=3;
	}

	lcd_off();
    return 1;
}
*/
