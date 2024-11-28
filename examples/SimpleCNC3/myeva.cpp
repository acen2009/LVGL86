#include "myeva.h"

#include "utility/tinyxml2.h"

using namespace tinyxml2;

#define UNSUPPORT        (0)
#define SLAVE_DIQ        (1)
#define SLAVE_HID        (2)
#define SLAVE_LCD        (3)
#define SLAVE_OPF        (4)
#define SLAVE_MOTOR      (5)
#define SLAVE_MONITOR    (6)

extern char* userInternalResourcePath;
char eniPath[256];
char evaPath[256];
typedef struct {

    char       *ObjectName;
    uint32_t    VendorId;
    uint32_t    ProductCode;
    uint16_t    AliasId;
    uint16_t    SlaveId;
    bool        haveMCU;
    double      Is;
    double      Ip;
    uint32_t    MaxSerial;
    uint32_t    MaxLcm;
    uint32_t    MaxMpg;
    uint32_t    MaxKeypad;
    uint32_t    MaxBuzzerPin;
    uint32_t    MaxLcd;
    uint32_t    MaxMicrophone;
    uint32_t    MaxSpeaker;
    uint32_t    MaxCounterPin;
    uint32_t    MaxMachine;
    uint32_t    MaxServo;
    uint32_t    MaxEncoder;
    uint32_t    MaxDO;
    uint32_t    MaxDI;
    int         Type;
    int        *MappingSerial;
    int        *MappingLcm;
    int        *MappingMpg;
    int        *MappingKeypad;
    int        *MappingBuzzerPin;
    int        *MappingLcd;
    int        *MappingMicrophone;
    int        *MappingSpeaker;
    int        *MappingCounterPin;
    int        *MappingMachine;
    int        *MappingServo;
    int        *MappingEncoder;
    int        *MappingDO;
    int        *MappingDI;
    void       *PrivateData;

} _MyEcatSlaveSetting;

typedef struct myecat_setting {

    char *ObjectName;
    char *ENIName;
    bool Redundant;
    bool IgnoreOverride;
    int  DcMode;
    uint64_t CycleTime;
    uint32_t MaxDigitalPins;
    uint32_t MaxLcds;
    uint32_t MaxSerials;
    uint32_t MaxLcms;
    uint32_t MaxMpgs;
    uint32_t MaxKeypads;
    uint32_t MaxBuzzerPins;
    uint32_t MaxMicrophones;
    uint32_t MaxSpeakers;
    uint32_t MaxCounterPins;
    uint32_t MaxMachines;
    uint32_t MaxEncoders;
    uint32_t MaxServos;
    uint16_t SlaveCount;
    _MyEcatSlaveSetting *Slave;

} _MyEcatSetting;

static inline int getBoolAttribute(XMLElement *root, const char *token, void *pValue)
{
    return (root->QueryBoolAttribute(token, (bool*)pValue) > 0) ? 0 : 1;
}

static inline int getIntAttribute(XMLElement *root, const char *token, void *pValue)
{
    return (root->QueryIntAttribute(token, (int*)pValue) > 0) ? 0 : 1;
}


static inline int getHexDecValue(XMLElement *root, void *pValue, size_t size)
{
    int64_t scan;
    int rc;

    if (size > sizeof(scan))
        return -1;

    rc = sscanf(root->GetText(), "%lld", (int64_t *)&scan);
    if (rc <= 0) {
        rc = sscanf(root->GetText(), "#x%llX", (int64_t *)&scan);
        if (rc <= 0)
            return -1;
    }

    if (sizeof(scan) < sizeof(uint32_t) && (scan & (0xFFFFFFFFULL << (8 * size))))
        return -1;

    memcpy(pValue, &scan, size);
    
    return 1;
}

static inline int getHexDecValueElement(XMLElement *root, const char *token, void *pValue, size_t size)
{
    XMLElement *element;

    element = root->FirstChildElement(token);
    if (!element)
        return 0;

    return getHexDecValue(element, pValue, size);
}

static inline int getStringElement(XMLElement *root, const char *token, char **pString)
{
    XMLElement *element;
    int len;

    element = root->FirstChildElement(token);
    if (!element || element->GetText() == NULL || (len = strlen(element->GetText())) == 0)
        return 0;

    pString[0] = (char*)malloc(len + 1);
    if (pString[0] == NULL)
        return -1;

    if (element)
        strncpy(pString[0], element->GetText(), len);
    pString[0][len] = '\0';
    
    return 1;
}

static inline void __release_slave(_MyEcatSlaveSetting *slave)
{
    if (slave->Type == SLAVE_DIQ) {
        if (slave->MappingDO) free(slave->MappingDO);
        if (slave->MappingDI) free(slave->MappingDI);
    } else if (slave->Type == SLAVE_HID) {
        if (slave->MappingSerial) free(slave->MappingSerial);
        if (slave->MappingLcm) free(slave->MappingLcm);
        if (slave->MappingMpg) free(slave->MappingMpg);
        if (slave->MappingKeypad) free(slave->MappingKeypad);
        if (slave->MappingBuzzerPin) free(slave->MappingBuzzerPin);
    }  else if (slave->Type == SLAVE_MONITOR) {
        if (slave->MappingMicrophone) free(slave->MappingMicrophone);
        if (slave->MappingSpeaker) free(slave->MappingSpeaker);
        if (slave->MappingCounterPin) free(slave->MappingCounterPin);
    } else if (slave->Type == SLAVE_LCD) {
        if (slave->MappingLcd) free(slave->MappingLcd);
    } else if (slave->Type == SLAVE_MOTOR) {
        if (slave->MappingMachine) free(slave->MappingMachine);
        if (slave->MappingServo) free(slave->MappingServo);
        if (slave->MappingEncoder) free(slave->MappingEncoder);
    }
    
    if (slave->ObjectName)
        free(slave->ObjectName);
    
    slave->ObjectName = NULL;
    slave->MappingDO = NULL;
    slave->MappingDI = NULL;
}

static inline void __release_master(_MyEcatSetting *master)
{
    if (master->ObjectName)
        free(master->ObjectName);
    if (master->ENIName)
        free(master->ENIName);
    master->ENIName = NULL;
}

static inline int __parse_diq_mapping(XMLElement *root, _MyEcatSlaveSetting *setting)
{
    XMLElement *element;
    int i;

    /* Digital Output. */
    setting->MaxDO = 0;
    element = root->FirstChildElement("DigitalOutput");
    while (element) {
        setting->MaxDO++;
        element = element->NextSiblingElement("DigitalOutput");
    }
    setting->MappingDO = (int *)malloc(sizeof(int) * setting->MaxDO);
    if (setting->MappingDO == NULL)
        return -1;
    i = 0;
    element = root->FirstChildElement("DigitalOutput");
    while (element) {
        if (getHexDecValue(element, &setting->MappingDO[i++], sizeof(*setting->MappingDO)) <= 0)
            return -1;
        element = element->NextSiblingElement("DigitalOutput");
    }

    /* Digital Input. */
    setting->MaxDI = 0;
    element = root->FirstChildElement("DigitalInput");
    while (element) {
        setting->MaxDI++;
        element = element->NextSiblingElement("DigitalInput");
    }
    setting->MappingDI = (int *)malloc(sizeof(int) * setting->MaxDI);
    if (setting->MappingDI == NULL)
        return -1;
    i = 0;
    element = root->FirstChildElement("DigitalInput");
    while (element) {
        if (getHexDecValue(element, &setting->MappingDI[i++], sizeof(*setting->MappingDI)) <= 0)
            return -1;
        element = element->NextSiblingElement("DigitalInput");
    }
    return 0;
}

static inline int __parse_lcd_mapping(XMLElement *root, _MyEcatSlaveSetting *setting)
{
    XMLElement *element;
    int i;

    /* Digital Output. */
    setting->MaxLcd = 0;
    element = root->FirstChildElement("Lcd24Number");
    while (element) {
        setting->MaxLcd++;
        element = element->NextSiblingElement("Lcd24Number");
    }
    setting->MappingLcd = (int *)malloc(sizeof(int) * setting->MaxLcd);
    if (setting->MappingLcd == NULL)
        return -1;
    i = 0;
    element = root->FirstChildElement("Lcd24Number");
    while (element) {
        if (getHexDecValue(element, &setting->MappingLcd[i++], sizeof(*setting->MappingLcd)) <= 0)
            return -1;
        element = element->NextSiblingElement("Lcd24Number");
    }

    return 0;
}

static inline int __parse_hid_mapping(XMLElement *root, _MyEcatSlaveSetting *setting)
{
    XMLElement *element;
    int i;

    // Serial
    setting->MaxSerial = 0;
    element = root->FirstChildElement("SerialPortNumber");
    while (element) {
        setting->MaxSerial++;
        element = element->NextSiblingElement("SerialPortNumber");
    }
    setting->MappingSerial = (int *)malloc(sizeof(int) * setting->MaxSerial);
    if (setting->MappingSerial == NULL)
        return -1;
    i = 0;
    element = root->FirstChildElement("SerialPortNumber");
    while (element) {
        if (getHexDecValue(element, &setting->MappingSerial[i++], sizeof(*setting->MappingSerial)) <= 0)
            return -1;
        element = element->NextSiblingElement("SerialPortNumber");
    }
    
    // Lcm
    setting->MaxLcm = 0;
    element = root->FirstChildElement("LcmNumber");
    while (element) {
        setting->MaxLcm++;
        element = element->NextSiblingElement("LcmNumber");
    }
    setting->MappingLcm = (int *)malloc(sizeof(int) * setting->MaxLcm);
    if (setting->MappingLcm == NULL)
        return -1;
    i = 0;
    element = root->FirstChildElement("LcmNumber");
    while (element) {
        if (getHexDecValue(element, &setting->MappingLcm[i++], sizeof(*setting->MappingLcm)) <= 0)
            return -1;
        element = element->NextSiblingElement("LcmNumber");
    }
    
    // Mpg
    setting->MaxMpg = 0;
    element = root->FirstChildElement("MpgNumber");
    while (element) {
        setting->MaxMpg++;
        element = element->NextSiblingElement("MpgNumber");
    }
    setting->MappingMpg = (int *)malloc(sizeof(int) * setting->MaxMpg);
    if (setting->MappingMpg == NULL)
        return -1;
    i = 0;
    element = root->FirstChildElement("MpgNumber");
    while (element) {
        if (getHexDecValue(element, &setting->MappingMpg[i++], sizeof(*setting->MappingMpg)) <= 0)
            return -1;
        element = element->NextSiblingElement("MpgNumber");
    }
    
    // Keypad
    setting->MaxKeypad = 0;
    element = root->FirstChildElement("KeypadNumber");
    while (element) {
        setting->MaxKeypad++;
        element = element->NextSiblingElement("KeypadNumber");
    }
    setting->MappingKeypad = (int *)malloc(sizeof(int) * setting->MaxKeypad);
    if (setting->MappingKeypad == NULL)
        return -1;
    i = 0;
    element = root->FirstChildElement("KeypadNumber");
    while (element) {
        if (getHexDecValue(element, &setting->MappingKeypad[i++], sizeof(*setting->MappingKeypad)) <= 0)
            return -1;
        element = element->NextSiblingElement("KeypadNumber");
    }
    
    // Buzzer
    setting->MaxBuzzerPin = 0;
    element = root->FirstChildElement("BuzzerPinNumber");
    while (element) {
        setting->MaxBuzzerPin++;
        element = element->NextSiblingElement("BuzzerPinNumber");
    }
    setting->MappingBuzzerPin = (int *)malloc(sizeof(int) * setting->MaxBuzzerPin);
    if (setting->MappingBuzzerPin == NULL)
        return -1;
    i = 0;
    element = root->FirstChildElement("BuzzerPinNumber");
    while (element) {
        if (getHexDecValue(element, &setting->MappingBuzzerPin[i++], sizeof(*setting->MappingBuzzerPin)) <= 0)
            return -1;
        element = element->NextSiblingElement("BuzzerPinNumber");
    }

    return 0;
}

static inline int __parse_monitor_mapping(XMLElement *root, _MyEcatSlaveSetting *setting)
{
    XMLElement *element;
    int i;

    // Microphone
    setting->MaxMicrophone = 0;
    element = root->FirstChildElement("MicrophoneNumber");
    while (element) {
        setting->MaxMicrophone++;
        element = element->NextSiblingElement("MicrophoneNumber");
    }
    setting->MappingMicrophone = (int *)malloc(sizeof(int) * setting->MaxMicrophone);
    if (setting->MappingMicrophone == NULL)
        return -1;
    i = 0;
    element = root->FirstChildElement("MicrophoneNumber");
    while (element) {
        if (getHexDecValue(element, &setting->MappingMicrophone[i++], sizeof(*setting->MappingMicrophone)) <= 0)
            return -1;
        element = element->NextSiblingElement("MicrophoneNumber");
    }
    
    // Speaker
    setting->MaxSpeaker = 0;
    element = root->FirstChildElement("SpeakerNumber");
    while (element) {
        setting->MaxSpeaker++;
        element = element->NextSiblingElement("SpeakerNumber");
    }
    setting->MappingSpeaker = (int *)malloc(sizeof(int) * setting->MaxSpeaker);
    if (setting->MappingSpeaker == NULL)
        return -1;
    i = 0;
    element = root->FirstChildElement("SpeakerNumber");
    while (element) {
        if (getHexDecValue(element, &setting->MappingSpeaker[i++], sizeof(*setting->MappingSpeaker)) <= 0)
            return -1;
        element = element->NextSiblingElement("SpeakerNumber");
    }
    
    // Counter Pin
    setting->MaxCounterPin = 0;
    element = root->FirstChildElement("CounterPinNumber");
    while (element) {
        setting->MaxCounterPin++;
        element = element->NextSiblingElement("CounterPinNumber");
    }
    setting->MappingCounterPin = (int *)malloc(sizeof(int) * setting->MaxCounterPin);
    if (setting->MappingCounterPin == NULL)
        return -1;
    i = 0;
    element = root->FirstChildElement("CounterPinNumber");
    while (element) {
        if (getHexDecValue(element, &setting->MappingCounterPin[i++], sizeof(*setting->MappingCounterPin)) <= 0)
            return -1;
        element = element->NextSiblingElement("CounterPinNumber");
    }
    
    return 0;
}

static inline int __parse_motor_mapping(XMLElement *root, _MyEcatSlaveSetting *setting)
{
    XMLElement *element;
    int i;

    setting->MaxMachine = 0;
    element = root->FirstChildElement("MachineNumber");
    while (element) {
        setting->MaxMachine++;
        element = element->NextSiblingElement("MachineNumber");
    }
    setting->MappingMachine = (int *)malloc(sizeof(int) * setting->MaxMachine);
    if (setting->MappingMachine == NULL)
        return -1;
    i = 0;
    element = root->FirstChildElement("MachineNumber");
    while (element) {
        if (getHexDecValue(element, &setting->MappingMachine[i++], sizeof(*setting->MappingMachine)) <= 0)
            return -1;
        element = element->NextSiblingElement("MachineNumber");
    }
    
    setting->MaxServo = 0;
    element = root->FirstChildElement("ServoNumber");
    while (element) {
        setting->MaxServo++;
        element = element->NextSiblingElement("ServoNumber");
    }
    setting->MappingServo = (int *)malloc(sizeof(int) * setting->MaxServo);
    if (setting->MappingServo == NULL)
        return -1;
    i = 0;
    element = root->FirstChildElement("ServoNumber");
    while (element) {
        if (getHexDecValue(element, &setting->MappingServo[i++], sizeof(*setting->MappingServo)) <= 0)
            return -1;
        element = element->NextSiblingElement("ServoNumber");
    }
    
    setting->MaxEncoder = 0;
    element = root->FirstChildElement("EncoderNumber");
    while (element) {
        setting->MaxEncoder++;
        element = element->NextSiblingElement("EncoderNumber");
    }
    setting->MappingEncoder = (int *)malloc(sizeof(int) * setting->MaxEncoder);
    if (setting->MappingEncoder == NULL)
        return -1;
    i = 0;
    element = root->FirstChildElement("EncoderNumber");
    while (element) {
        if (getHexDecValue(element, &setting->MappingEncoder[i++], sizeof(*setting->MappingEncoder)) <= 0)
            return -1;
        element = element->NextSiblingElement("EncoderNumber");
    }
    
    return 0;
}

static inline int __parse_slave(XMLElement *root, _MyEcatSlaveSetting *setting)
{
    XMLElement *element;
    if (getStringElement(root, "ObjectName", &setting->ObjectName) <= 0)
        return -1;
    if (getHexDecValueElement(root, "VendorId", &setting->VendorId, sizeof(setting->VendorId)) <= 0)
        return -1;
    if (getHexDecValueElement(root, "ProductCode", &setting->ProductCode, sizeof(setting->ProductCode)) <= 0)
        return -1;
    if (getHexDecValueElement(root, "AliasAddress", &setting->AliasId, sizeof(setting->AliasId)) <= 0)
        return -1;
    if (getHexDecValueElement(root, "HaveMCU", &setting->haveMCU, sizeof(setting->haveMCU)) <= 0)
        return -1;
    
    getHexDecValueElement(root, "Type", &setting->Type, sizeof(setting->Type));
    
    element = root->FirstChildElement("Mapping");
    if (element == NULL) {
        return -1; // we don't determine it , because it may be one unsopport device
    }
    
    if (setting->Type == SLAVE_HID) {
        if (element && __parse_hid_mapping(element, setting) < 0)
            return -1;
    } else if (setting->Type == SLAVE_LCD) {
        if (element && __parse_lcd_mapping(element, setting) < 0)
            return -1;
    } else if (setting->Type == SLAVE_MONITOR) {
        if (element && __parse_monitor_mapping(element, setting) < 0)
            return -1;
    } else if (setting->Type == SLAVE_MOTOR) {
        if (element && __parse_motor_mapping(element, setting) < 0)
            return -1;
    } else {
        setting->Type == SLAVE_DIQ;
        if (element && __parse_diq_mapping(element, setting) < 0)
            return -1;
    }
    
    return 0;
}

static inline int __parse_master(XMLElement *root, _MyEcatSetting *setting)
{
    setting->Redundant = false;
    setting->IgnoreOverride = true;
    setting->DcMode = 0;
    if (getBoolAttribute(root, "Redundant", &setting->Redundant) < 0)
        return -1;
    if (getBoolAttribute(root, "IgnoreOverride", &setting->IgnoreOverride) < 0)
        return -1;
    if (getIntAttribute(root, "DcMode", &setting->DcMode) < 0)
        return -1;
    if (getHexDecValueElement(root, "CycleTime", &setting->CycleTime, sizeof(setting->CycleTime)) <= 0)
        return -1;
    if (getHexDecValueElement(root, "MaxDigitalPins", &setting->MaxDigitalPins, sizeof(setting->MaxDigitalPins)) <= 0)
        return -1;
    if (getHexDecValueElement(root, "MaxLcds", &setting->MaxLcds, sizeof(setting->MaxLcds)) <= 0)
        return -1;
    if (getHexDecValueElement(root, "MaxSerials", &setting->MaxSerials, sizeof(setting->MaxSerials)) <= 0)
        return -1;
    if (getHexDecValueElement(root, "MaxLcms", &setting->MaxLcms, sizeof(setting->MaxLcms)) <= 0)
        return -1;
    if (getHexDecValueElement(root, "MaxMpgs", &setting->MaxMpgs, sizeof(setting->MaxMpgs)) <= 0)
        return -1;
    if (getHexDecValueElement(root, "MaxKeypads", &setting->MaxKeypads, sizeof(setting->MaxKeypads)) <= 0)
        return -1;
    if (getHexDecValueElement(root, "MaxBuzzerPins", &setting->MaxBuzzerPins, sizeof(setting->MaxBuzzerPins)) <= 0)
        return -1;
    if (getHexDecValueElement(root, "MaxMicrophones", &setting->MaxMicrophones, sizeof(setting->MaxMicrophones)) <= 0)
        return -1;
    if (getHexDecValueElement(root, "MaxSpeakers", &setting->MaxSpeakers, sizeof(setting->MaxSpeakers)) <= 0)
        return -1;
    if (getHexDecValueElement(root, "MaxCounterPins", &setting->MaxCounterPins, sizeof(setting->MaxCounterPins)) <= 0)
        return -1;
    if (getHexDecValueElement(root, "MaxMachines", &setting->MaxMachines, sizeof(setting->MaxMachines)) <= 0)
        return -1;
    if (getHexDecValueElement(root, "MaxServos", &setting->MaxServos, sizeof(setting->MaxServos)) <= 0)
        return -1;
    if (getHexDecValueElement(root, "MaxEncoders", &setting->MaxEncoders, sizeof(setting->MaxEncoders)) <= 0)
        return -1;
    if (getStringElement(root, "ObjectName", &setting->ObjectName) < 0)
        return -1;
    if (getStringElement(root, "ENIName", &setting->ENIName) < 0)
        return -1;
    return 0;
}

static inline _MyEcatSetting *requestEVA()
{
    _MyEcatSetting *setting;
    XMLDocument xml;
    XMLError error;
    XMLElement *root, *element;
    uint16_t SlaveCount = 0;
    int i = 0;
    
    if (userInternalResourcePath != NULL)
        sprintf(evaPath, "%smyeccfg\\", userInternalResourcePath);
    else
        sprintf(evaPath, "B:\\resource\\myeccfg\\");
    strcat(evaPath, "cfg.eva");
    
    error = xml.LoadFile(evaPath);
    if(error != XML_SUCCESS) {
        printf("ERROR: XMLError = %d\n", error);
        return NULL;
    }
    
    root = xml.RootElement();
    if (strcmp(root->Name(), "EtherCATVirtualArduino")) {
        printf("ERROR: No EtherCATVirtualArduino.\n");
        return NULL;
    }

    element = root->FirstChildElement("Slave");
    while (element) {
        SlaveCount++;
        element = element->NextSiblingElement();
    }
    if (SlaveCount == 0) {
        printf("ERROR: No Slave.\n");
        return NULL;
    }

    element = root->FirstChildElement("Master");
    if (element == NULL) {
        printf("ERROR: No Master.\n");
        return NULL;
    }

    setting = (_MyEcatSetting *)malloc(sizeof(*setting) + SlaveCount * sizeof(_MyEcatSlaveSetting));
    if (setting == NULL) {
        printf("ERROR: Allocate _MyEcatSetting structure failed.\n");
        return NULL;
    }
    memset(setting, 0, sizeof(*setting) + SlaveCount * sizeof(_MyEcatSlaveSetting));
    setting->Slave = (_MyEcatSlaveSetting *)(setting + 1);
    setting->SlaveCount = SlaveCount;

    if (__parse_master(element, setting) < 0) {
        printf("ERROR: Parse Master failed.\n");
        return NULL;
    }

    element = root->FirstChildElement("Slave");
    while (element) {
        if (__parse_slave(element, &setting->Slave[i++]) < 0) {
            /*
            while (i-- >= 0) {
                __release_slave(&setting->Slave[i]);
            }
            */
        }
        element = element->NextSiblingElement();
    }

    return setting;
}

static inline void releaseEVA(_MyEcatSetting *setting)
{
    for (int i = 0; i < setting->SlaveCount; i++)
        __release_slave(&setting->Slave[i]);
    __release_master(setting);
    free(setting);
}

#include "queue_lite.h"
static queue_t * queue = NULL;
static bool enableCheckCBError = true;
void ErrorCallback(uint32_t error)
{
    if (queue && !queue_full(queue))
        queue_push(queue, (uint8_t)error);
}


void debug(bool d) {
    if (d) {
        enableCheckCBError = true;
    }
    else {
        enableCheckCBError = false;
    }
}

#define ATTACH_ID(alias, idx)   ((alias > 0) ? alias : idx)
#define ATTACH_MODE(alias)      ((alias > 0) ? ECAT_ALIAS_ADDRESS : ECAT_SLAVE_NO)

EthercatDevice_QECR11MP3S Slave0;
#define NUM_SLAVE (1)
EthercatMaster EcatMaster;
MyEVA EVA;

MyVirtualCNC VirtualCNC1;
MyVirtualCNC VirtualCNC2;
MyVirtualCNC VirtualCNC3;
MyVirtualCNC VirtualCNC4;
MyVirtualServo VirtualServo1;
MyVirtualServo VirtualServo2;
MyVirtualServo VirtualServo3;
MyVirtualServo VirtualServo4;
MyVirtualServo VirtualServo5;
MyVirtualServo VirtualServo6;
MyVirtualServo VirtualServo7;
MyVirtualServo VirtualServo8;
MyVirtualEncoder VirtualEncoder1;
MyVirtualEncoder VirtualEncoder2;
MyVirtualEncoder VirtualEncoder3;
void _task_handler();
static _HelperTask* _taskRTC = NULL;

typedef struct myecat_pinmap_digital {
    _MyEcatSlaveSetting* Setting;
    uint32_t Pin;
    unsigned Out : 1;
} _MyEcatPinMapDigital;

typedef struct myecat_portmap_serial {
    void* Object;
    int Port;
} _MyEcatPortMapSerial;
_MyEcatPinMapDigital* DIO = NULL;
_MyEcatPortMapSerial* BUZZER = NULL;
_MyEcatPortMapSerial* COUNTER = NULL;
_MyEcatSetting* Setting = NULL;

typedef struct deviceName {
    unsigned long pid;
    const char* name;
} DEVICEIFO;

DEVICEIFO devices[] = {
    {0x0086D0D4, "QEC-R11D0FS"},
    {0x0086D305, "QEC-R11D0FH"},
    {0x0086D30A, "QEC-R00D0FH"},
    {0x0086D0D2, "QEC-R11DF0D"},
    {0x0086D306, "QEC-R11DF0H"},
    {0x0086D30B, "QEC-R00DF0H"},
    {0x0086D303, "QEC-R00D0FS"},
    {0x0086D300, "QEC-R00DF0D"},
    {0x0086D0D5, "QEC-R11D88S"},
    {0x0086D301, "QEC-R00D88D"},
    {0x0086D307, "QEC-R11D88D"},
    {0x0086D308, "QEC-R11D88H"},
    {0x0086D309, "QEC-R00D88S"},
    {0x0086D304, "QEC-R00DC4D"},
    {0x0086D302, "QEC-R00D4CD"},
    {0x0086D404, "QEC-R11HU1S"},
    {0x0086D405, "QEC-R00HU1S"},
    {0x0086D403, "QEC-R11HU5S"},
    {0x0086D400, "QEC-R00HU5S"},
    {0x0086D401, "QEC-R00HU9S"},
    {0x0086D402, "QEC-R11HU9S"},
    {0x0086D100, "QEC-R00UN01"},
    {0x0086D103, "QEC-R11UN01"},
    {0x0086D0D6, "QEC-R11MP3S"},
    {0x0086D0D9, "QEC-R00MP3S"},
    {0x0086D500, "QEC-R11MC8S"},
    {0x0086D501, "QEC-R11MC1S"},
    {0x0086D502, "QEC-R11MC4S"},
    {0x00860102, "QEC-T120"},
    {0x00860103, "QEC-T600"},
    {0x0086D320, "QEC-R11DT0L"},
    {0x0086D700, "QEC-R11DT0H"},
    {0x0086D324, "QEC-R11D0TL"},
    {0x0086D800, "QEC-R11D0TH"},
    {0x0086D323, "QEC-R00DT0L"},
    {0x0086D701, "QEC-R00DT0H"},
    {0x0086D327, "QEC-R00D0TL"},
    {0x0086D801, "QEC-R00D0TH"}
};

char UDName[] = "Unknown Device";
const char* getDeviceName(int aliasAddress) {
    int _size = sizeof(devices) / sizeof(DEVICEIFO);
    for (int i = 0; i < NUM_SLAVE; i++) {
        if (aliasAddress == Setting->Slave[i].AliasId) {
            for (int j = 0; j < _size; j++)
                if (devices[j].pid == Setting->Slave[i].ProductCode) return devices[j].name;
        }
    }

    return UDName;
}

void printErrorMessage(int rc) {
    char buf[256];
    EcatMaster.getErrorMessage(rc, buf, sizeof(buf));
    Serial.print("Error: ");
    Serial.println(buf);
}

void getErrorMessage(int error) {
    char buf[256];
    EcatMaster.getErrorMessage(error, buf, sizeof(buf)); 
    Serial.print("Error: ");
    Serial.println(buf);
}

static double* realCurrentIs = NULL;
static double* realCurrentIp = NULL;
static double* showCurrentIs = NULL;
static double* showCurrentIp = NULL;
static void recalculateCurrent(_MyEcatSetting * setting, int idx, double Ip, double Is) {
    static bool prepareIpRealData = true;
    static bool prepareIsRealData = true;

    if (realCurrentIp == NULL || realCurrentIs == NULL) return;

    realCurrentIs[idx] = Is;
    realCurrentIp[idx] = Ip;

    for (int i = 0; i < NUM_SLAVE; i++) {
        if (realCurrentIs[i] == -1)
            break;

        if (i == (NUM_SLAVE - 1))
            prepareIsRealData = false;
    }

    if (!prepareIsRealData) {
        for (int i = 0; i < NUM_SLAVE; i++) {
            showCurrentIs[i] = realCurrentIs[i]; // update from real to show array
            if ((i + 1) < NUM_SLAVE) {
                if (strstr(getDeviceName(setting->Slave[i].AliasId), "QEC-R11") != NULL && strstr(getDeviceName(setting->Slave[i + 1].AliasId), "QEC-R11") != NULL) // I am red-head LAN and next also is red-head LAN
                    showCurrentIs[i] -= realCurrentIs[i + 1];

                if (showCurrentIs[i] < 0.0) showCurrentIs[i] = 0.0;
            }
            setting->Slave[i].Is = showCurrentIs[i];
            realCurrentIs[i] = -1;
        }
        prepareIsRealData = true;
    }

    if (prepareIpRealData) {
        for (int i = 0; i < NUM_SLAVE; i++) {
            if (realCurrentIp[i] == -1)
                break;

            if (i == (NUM_SLAVE - 1))
                prepareIpRealData = false;
        }
    }

    if (!prepareIpRealData) {
        for (int i = 0; i < NUM_SLAVE; i++) {
            showCurrentIp[i] = realCurrentIp[i]; // update from real to show array
            if ((i + 1) < NUM_SLAVE) {
                if (strstr(getDeviceName(setting->Slave[i].AliasId), "QEC-R11") != NULL && strstr(getDeviceName(setting->Slave[i + 1].AliasId), "QEC-R11") != NULL) // I am red-head LAN and next also is red-head LAN
                    showCurrentIp[i] -= realCurrentIp[i + 1];

                if (showCurrentIp[i] < 0.0) showCurrentIp[i] = 0.0;
            }
            setting->Slave[i].Ip = showCurrentIp[i];
            realCurrentIp[i] = -1;
        }
        prepareIpRealData = true;
    }
}

static bool masterIsRedHead(void) {
    static FILE* _myfp = NULL;
    static bool once = false;

    if (once) {
        if (_myfp != NULL)
            return true; // don't open file again
        else
            return false;
    }

    FILE* fp = fopen("A:/redhead.v86", "r");
    once = true;
    if (fp == NULL) return false; // no this file, it is Black Head
    _myfp = fp;
    fclose(fp);

    return true;
}

double myIS[NUM_SLAVE] = {0.0};
double myIP[NUM_SLAVE] = {0.0};
static double _master_Ip = -1.0, _master_Is = -1.0;
static void refreshIsIp(_MyEcatSetting * setting) {
    double vs, sp, is, ip, temp;

    if (NUM_SLAVE == 0) {
        _master_Ip = peripheralPowerCurrent(); 
        _master_Is = systemPowerCurrent(); 
        return;
    }

    for (int i = 0; i < NUM_SLAVE; i++) {
        if (!setting->Slave[i].haveMCU) {
            is = 0.0; ip = 0.0;
        } else {
            is = myIS[i];
            ip = myIP[i];
        }
        if (i == 0) {
            if (masterIsRedHead() && strstr(getDeviceName(setting->Slave[0].AliasId), "QEC-R11") != NULL) {
                _master_Ip = peripheralPowerCurrent() - ip; 
                _master_Is = systemPowerCurrent() - is; 
                if (_master_Ip < 0) _master_Ip = 0.0; 
                if (_master_Is < 0) _master_Is = 0.0; 
            } else {
                _master_Ip = peripheralPowerCurrent();
                _master_Is = systemPowerCurrent();
            }
        }

        recalculateCurrent(setting, i, ip, is);
    }
}


MyEVA::MyEVA() {}
int MyEVA::begin()
{
    const char *ObjectName[NUM_SLAVE] = {"Slave0"};
    const void *ObjectPointer[NUM_SLAVE] = {&Slave0};
   _MyEcatSlaveSetting *mapping[NUM_SLAVE] = {NULL};
    _MyEcatSetting * setting;
    _MyEcatPinMapDigital *Dio;
    _MyEcatPortMapSerial *SerialMapping;
    _MyEcatPortMapSerial *MpgMapping;
    _MyEcatPortMapSerial *LcmMapping;
    _MyEcatPortMapSerial *KeypadMapping;
    _MyEcatPortMapSerial *BuzzerPinMapping;
    _MyEcatPortMapSerial *LcdMapping;
    _MyEcatPortMapSerial *MicrophoneMapping;
    _MyEcatPortMapSerial *SpeakerMapping;
    _MyEcatPortMapSerial *CounterPinMapping;
    _MyEcatPortMapSerial *MachineMapping;
    _MyEcatPortMapSerial *ServoMapping;
    _MyEcatPortMapSerial *EncoderMapping;
    EthercatMasterSettings myEcSetting;
    int rc, i, j, k;

    realCurrentIs = (double*)malloc(sizeof(double) * NUM_SLAVE);
    realCurrentIp = (double*)malloc(sizeof(double) * NUM_SLAVE);
    for (int i = 0; i < NUM_SLAVE; i++) {
        realCurrentIs[i] = -1;
        realCurrentIp[i] = -1;
    }
    showCurrentIs = (double*)malloc(sizeof(double) * NUM_SLAVE);
    showCurrentIp = (double*)malloc(sizeof(double) * NUM_SLAVE);

    setting = requestEVA();
    if (setting == NULL) {
        return EVAERR_NO_CONFIG_FILE;
    }

    for (i = 0; i < NUM_SLAVE; i++) {
        for (j = 0; j < setting->SlaveCount; j++) {
            if (strcmp(setting->Slave[j].ObjectName, ObjectName[i]) == 0) {
                setting->Slave[j].PrivateData = (void*)ObjectPointer[i];
                mapping[i] = &setting->Slave[j];
                break;
            }
        }
    }

    /* Allocate memory of digital pin mapping. */
    Dio = (_MyEcatPinMapDigital*)malloc(setting->MaxDigitalPins * sizeof(*Dio));
    if (Dio == NULL) {
        releaseEVA(setting);
        return EVAERR_NO_ENOUGH_MEMORY;
    }
    memset(Dio, 0, setting->MaxDigitalPins * sizeof(*Dio)); 

    SerialMapping = (_MyEcatPortMapSerial*)malloc((setting->MaxSerials + setting->MaxMpgs + setting->MaxLcms + setting->MaxKeypads + setting->MaxBuzzerPins + setting->MaxLcds + setting->MaxCounterPins + setting->MaxSpeakers + setting->MaxMicrophones + setting->MaxMachines + setting->MaxServos + setting->MaxEncoders) * sizeof(*SerialMapping));
    if (SerialMapping == NULL) {
        releaseEVA(setting);
        return EVAERR_NO_ENOUGH_MEMORY;
    }
    memset(SerialMapping, 0, (setting->MaxSerials + setting->MaxMpgs + setting->MaxLcms + setting->MaxKeypads + setting->MaxBuzzerPins + setting->MaxLcds + setting->MaxCounterPins + setting->MaxSpeakers + setting->MaxMicrophones + setting->MaxMachines + setting->MaxServos + setting->MaxEncoders) * sizeof(*SerialMapping));

    MpgMapping = (_MyEcatPortMapSerial*)(SerialMapping + setting->MaxSerials);
    LcmMapping = (_MyEcatPortMapSerial*)(MpgMapping + setting->MaxMpgs);
    KeypadMapping = (_MyEcatPortMapSerial*)(LcmMapping + setting->MaxLcms);
    BuzzerPinMapping = (_MyEcatPortMapSerial*)(KeypadMapping + setting->MaxKeypads);
    LcdMapping = (_MyEcatPortMapSerial*)(BuzzerPinMapping + setting->MaxBuzzerPins);
    CounterPinMapping = (_MyEcatPortMapSerial*)(LcdMapping + setting->MaxLcds);
    SpeakerMapping = (_MyEcatPortMapSerial*)(CounterPinMapping + setting->MaxCounterPins);
    MicrophoneMapping = (_MyEcatPortMapSerial*)(SpeakerMapping + setting->MaxSpeakers);
    MachineMapping = (_MyEcatPortMapSerial*)(MicrophoneMapping + setting->MaxMicrophones);
    ServoMapping = (_MyEcatPortMapSerial*)(MachineMapping + setting->MaxMachines);
    EncoderMapping = (_MyEcatPortMapSerial*)(ServoMapping + setting->MaxServos);

    for (i = 0; i < setting->MaxSerials; i++) {
        for (j = 0; j < setting->SlaveCount; j++) {
            if (setting->Slave[j].Type == SLAVE_HID) {
                for (k = 0; k < setting->Slave[j].MaxSerial; k++) {
                    if (i == setting->Slave[j].MappingSerial[k]) {
                        SerialMapping[i].Object = setting->Slave[j].PrivateData;
                        SerialMapping[i].Port = k;
                    }
                }
            }
        }
    }

    for (i = 0; i < setting->MaxMpgs; i++) {
        for (j = 0; j < setting->SlaveCount; j++) {
            if (setting->Slave[j].Type == SLAVE_HID) {
                for (k = 0; k < setting->Slave[j].MaxMpg; k++) {
                    if (i == setting->Slave[j].MappingMpg[k]) {
                        MpgMapping[i].Object = setting->Slave[j].PrivateData;
                        MpgMapping[i].Port = 0;
                    }
                }
            }
        }
    }

    for (i = 0; i < setting->MaxLcms; i++) {
        for (j = 0; j < setting->SlaveCount; j++) {
            if (setting->Slave[j].Type == SLAVE_HID) {
                for (k = 0; k < setting->Slave[j].MaxLcm; k++) {
                    if (i == setting->Slave[j].MappingLcm[k]) {
                        LcmMapping[i].Object = setting->Slave[j].PrivateData;
                        LcmMapping[i].Port = 0;
                    }
                }
            }
        }
    }

    for (i = 0; i < setting->MaxKeypads; i++) {
        for (j = 0; j < setting->SlaveCount; j++) {
            if (setting->Slave[j].Type == SLAVE_HID) {
                for (k = 0; k < setting->Slave[j].MaxKeypad; k++) {
                    if (i == setting->Slave[j].MappingKeypad[k]) {
                        KeypadMapping[i].Object = setting->Slave[j].PrivateData;
                        KeypadMapping[i].Port = 0;
                    }
                }
            }
        }
    }

    for (i = 0; i < setting->MaxBuzzerPins; i++) {
        for (j = 0; j < setting->SlaveCount; j++) {
            if (setting->Slave[j].Type == SLAVE_HID) {
                for (k = 0; k < setting->Slave[j].MaxBuzzerPin; k++) {
                    if (i == setting->Slave[j].MappingBuzzerPin[k]) {
                        BuzzerPinMapping[i].Object = setting->Slave[j].PrivateData;
                        BuzzerPinMapping[i].Port = 0;
                    }
                }
            }
        }
    }
    for (i = 0; i < setting->MaxLcds; i++) {
        for (j = 0; j < setting->SlaveCount; j++) {
            if (setting->Slave[j].Type == SLAVE_LCD) {
                for (k = 0; k < setting->Slave[j].MaxLcd; k++) {
                    if (i == setting->Slave[j].MappingLcd[k]) {
                        LcdMapping[i].Object = setting->Slave[j].PrivateData;
                        LcdMapping[i].Port = 0;
                    }
                }
            }
        }
    }
    for (i = 0; i < setting->MaxCounterPins; i++) {
        for (j = 0; j < setting->SlaveCount; j++) {
            if (setting->Slave[j].Type == SLAVE_MONITOR) {
                for (k = 0; k < setting->Slave[j].MaxCounterPin; k++) {
                    if (i == setting->Slave[j].MappingCounterPin[k]) {
                        CounterPinMapping[i].Object = setting->Slave[j].PrivateData;
                        CounterPinMapping[i].Port = k; // here, Port mean Pin, Counter is pin not port
                    }
                }
            }
        }
    }
    for (i = 0; i < setting->MaxSpeakers; i++) {
        for (j = 0; j < setting->SlaveCount; j++) {
            if (setting->Slave[j].Type == SLAVE_MONITOR) {
                for (k = 0; k < setting->Slave[j].MaxSpeaker; k++) {
                    if (i == setting->Slave[j].MappingSpeaker[k]) {
                        SpeakerMapping[i].Object = setting->Slave[j].PrivateData;
                        SpeakerMapping[i].Port = 0;
                    }
                }
            }
        }
    }
    for (i = 0; i < setting->MaxMicrophones; i++) {
        for (j = 0; j < setting->SlaveCount; j++) {
            if (setting->Slave[j].Type == SLAVE_MONITOR) {
                for (k = 0; k < setting->Slave[j].MaxMicrophone; k++) {
                    if (i == setting->Slave[j].MappingMicrophone[k]) {
                        MicrophoneMapping[i].Object = setting->Slave[j].PrivateData;
                        MicrophoneMapping[i].Port = 0;
                    }
                }
            }
        }
    }
    for (i = 0; i < setting->MaxMachines; i++) {
        for (j = 0; j < setting->SlaveCount; j++) {
            if (setting->Slave[j].Type == SLAVE_MOTOR) {
                for (k = 0; k < setting->Slave[j].MaxMachine; k++) {
                    if (i == setting->Slave[j].MappingMachine[k]) {
                        MachineMapping[i].Object = setting->Slave[j].PrivateData;
                        MachineMapping[i].Port = 0;
                    }
                }
            }
        }
    }
    for (i = 0; i < setting->MaxServos; i++) {
        for (j = 0; j < setting->SlaveCount; j++) {
            if (setting->Slave[j].Type == SLAVE_MOTOR) {
                for (k = 0; k < setting->Slave[j].MaxServo; k++) {
                    if (i == setting->Slave[j].MappingServo[k]) {
                        ServoMapping[i].Object = setting->Slave[j].PrivateData;
                        ServoMapping[i].Port = k;
                    }
                }
            }
        }
    }
    for (i = 0; i < setting->MaxEncoders; i++) {
        for (j = 0; j < setting->SlaveCount; j++) {
            if (setting->Slave[j].Type == SLAVE_MOTOR) {
                for (k = 0; k < setting->Slave[j].MaxEncoder; k++) {
                    if (i == setting->Slave[j].MappingEncoder[k]) {
                        EncoderMapping[i].Object = setting->Slave[j].PrivateData;
                        EncoderMapping[i].Port = k;
                    }
                }
            }
        }
    }

    for (i = 0; i < setting->SlaveCount; i++) {
        if (setting->Slave[i].Type == SLAVE_DIQ) {
            for (j = 0; (uint32_t)j < setting->Slave[i].MaxDO; j++) {
                k = setting->Slave[i].MappingDO[j]; 
                if (k >= 0 && (uint32_t)k < setting->MaxDigitalPins) {
                        Dio[k].Setting = &setting->Slave[i]; 
                        Dio[k].Pin = j; 
                        Dio[k].Out = 1; 
                }
            }

            for (j = 0; (uint32_t)j < setting->Slave[i].MaxDI; j++) {
                k = setting->Slave[i].MappingDI[j]; 
                if (k >= 0 && (uint32_t)k < setting->MaxDigitalPins) {
                        Dio[k].Setting = &setting->Slave[i]; 
                        Dio[k].Pin = j; 
                        Dio[k].Out = 0; 
                }
            }
        }
    }

    EcatMaster.readSettings(&myEcSetting);
    myEcSetting.IgnoreBiosOverride = setting->IgnoreOverride;
    EcatMaster.saveSettings(&myEcSetting);

    if (setting->ENIName != NULL) {
        if (userInternalResourcePath != NULL)
            sprintf(eniPath, "%smyeccfg\\", userInternalResourcePath); 
        else
            sprintf(eniPath, "B:\\resource\\myeccfg\\"); 
        strcat(eniPath, "eni.xml"); 
        rc = EcatMaster.begin(setting->Redundant ? ECAT_ETH_REDUNDANCY : ECAT_ETH_0, eniPath);
    } else {
        rc = EcatMaster.begin(setting->Redundant ? ECAT_ETH_REDUNDANCY : ECAT_ETH_0, NULL);
    }
    if (rc < 0) {
        Serial.print("Begin - ");
        printErrorMessage(rc);
        free(Dio);
        releaseEVA(setting);
        return rc;
    }

    EcatMaster.attachErrorCallback(ErrorCallback);
    queue = queue_create(4095);
    if (queue == NULL)
        Serial.println("ERROR: queue_create() failed.");

    VirtualCNC1.init(MachineMapping[0].Object);
    VirtualCNC2.init(MachineMapping[1].Object);
    VirtualCNC3.init(MachineMapping[2].Object);
    VirtualCNC4.init(MachineMapping[3].Object);
    VirtualEncoder1.init(EncoderMapping[0].Object, EncoderMapping[0].Port);
    VirtualEncoder2.init(EncoderMapping[1].Object, EncoderMapping[1].Port);
    VirtualEncoder3.init(EncoderMapping[2].Object, EncoderMapping[2].Port);
    if (mapping[0]) {
        if (Slave0.attach(ATTACH_ID(mapping[0]->AliasId, 0), EcatMaster, ATTACH_MODE(mapping[0]->AliasId)) != 0) {
            Serial.print("Seq ID:"); Serial.print(0); Serial.print(" Alias address:"); Serial.print(mapping[0]->AliasId); Serial.println("  attach error!");
        }
    Slave0.machineServoOn();
    }
    VirtualServo1.init(ServoMapping[0].Object, ServoMapping[0].Port);
    VirtualServo2.init(ServoMapping[1].Object, ServoMapping[1].Port);
    VirtualServo3.init(ServoMapping[2].Object, ServoMapping[2].Port);
    VirtualServo4.init(ServoMapping[3].Object, ServoMapping[3].Port);
    VirtualServo5.init(ServoMapping[4].Object, ServoMapping[4].Port);
    VirtualServo6.init(ServoMapping[5].Object, ServoMapping[5].Port);
    VirtualServo7.init(ServoMapping[6].Object, ServoMapping[6].Port);
    VirtualServo8.init(ServoMapping[7].Object, ServoMapping[7].Port);

    for (j = 0; j < setting->SlaveCount; j++) {
        if (setting->Slave[j].Type == SLAVE_MONITOR) { // Enable Counter
            ((EthercatDevice_DmpMIC_Generic*)setting->Slave[j].PrivateData)->counterSetMode(3, 1);
        }
    }

    rc = EcatMaster.start(setting->CycleTime, ECAT_FREERUN_AUTO); 
    if (rc < 0) {
        Serial.print("Start - ");
        printErrorMessage(rc);
    }

    Setting = setting;
    DIO = Dio;
    BUZZER = BuzzerPinMapping;
    COUNTER = CounterPinMapping;

    _taskRTC = myHelper.startLoop(100 * 1024);
    if (_taskRTC == NULL) {
        printf("Error: myHelper init fail");
        return 1;
    }
    _taskRTC->registerFunc(_task_handler);
    _taskRTC->start();

    return 0;
}

void MyEVA::end()
{
    EcatMaster.stop();
    EcatMaster.end();
    free(DIO);
    free(BUZZER);
    free(COUNTER);
    releaseEVA(Setting);
    Setting = NULL;
    DIO = NULL;
}

int MyEVA::digitalWrite(uint32_t pin, uint8_t val)
{
    if (Setting == NULL || pin >= Setting->MaxDigitalPins ||
        DIO[pin].Out == 0 ||
        DIO[pin].Setting == NULL ||
        DIO[pin].Setting->PrivateData == NULL)
        return -1;

    return ((EthercatDevice_DmpDIQ_Generic*)DIO[pin].Setting->PrivateData)->digitalWrite(DIO[pin].Pin, val);
}

int MyEVA::digitalRead(uint32_t pin)
{
    if (Setting == NULL || pin >= Setting->MaxDigitalPins ||
        DIO[pin].Out == 1 ||
        DIO[pin].Setting == NULL ||
        DIO[pin].Setting->PrivateData == NULL)
        return LOW;

    return ((EthercatDevice_DmpDIQ_Generic*)DIO[pin].Setting->PrivateData)->digitalRead(DIO[pin].Pin);
}

int MyEVA::tone(uint8_t pin, uint32_t freq, uint32_t duration)
{
    if (Setting == NULL || pin >= Setting->MaxBuzzerPins ||
        BUZZER[pin].Object == NULL)
        return -1;

    return ((EthercatDevice_DmpHID_Generic*)BUZZER[pin].Object)->buzzer(freq, duration);
}

int MyEVA::counterRead(uint32_t pin)
{
    if (Setting == NULL || pin >= Setting->MaxCounterPins ||
        COUNTER[pin].Object == NULL)
        return -1;

    return ((EthercatDevice_DmpMIC_Generic*)COUNTER[pin].Object)->counterRead(COUNTER[pin].Port);
}

int MyEVA::setCounterMode(int aliasId, uint8_t mode, uint16_t period_ms)
{
    int index = -1;

    if (Setting == NULL)
        return -1;

    for (int i=0; i<NUM_SLAVE; i++) {
        if (Setting->Slave[i].AliasId == aliasId) {
            index = i;
            break;
        }
    }

    if (index == -1) return -1;

    return ((EthercatDevice_DmpMIC_Generic*)Setting->Slave[index].PrivateData)->counterSetMode(mode, period_ms);
}

double MyEVA::getUsVoltage(int aliasId)
{
    int index = -1;

    if (aliasId == -1) return systemPowerVoltage();

    if (Setting == NULL)
        return -1;

    index = EcatMaster.getSlaveNo(aliasId);

    if (index < 0 || !Setting->Slave[index].haveMCU) return 0.0;

    while (!((_EthercatDevice_DmpCommonDriver*)Setting->Slave[index].PrivateData)->tryToGetSystemPowerVoltage()) { helperYield(); }

    return ((_EthercatDevice_DmpCommonDriver*)Setting->Slave[index].PrivateData)->getSystemPowerVoltage();
}

double MyEVA::getUpVoltage(int aliasId)
{
    int index = -1;

    if (aliasId == -1) return peripheralPowerVoltage();

    if (Setting == NULL)
        return -1;

    index = EcatMaster.getSlaveNo(aliasId);

    if (index < 0 || !Setting->Slave[index].haveMCU) return 0.0;

    while (!((_EthercatDevice_DmpCommonDriver*)Setting->Slave[index].PrivateData)->tryToGetPeripheralPowerVoltage()) { helperYield(); }

    return ((_EthercatDevice_DmpCommonDriver*)Setting->Slave[index].PrivateData)->getPeripheralPowerVoltage();
}

double MyEVA::getIsCurrent(int aliasId)
{
    int index = -1;

    if (aliasId == -1) {
        refreshIsIp(Setting);
        return _master_Is;
    }

    if (Setting == NULL)
        return -1;

    index = EcatMaster.getSlaveNo(aliasId);

    if (index < 0 || !Setting->Slave[index].haveMCU) return 0.0;

    while (!((_EthercatDevice_DmpCommonDriver*)Setting->Slave[index].PrivateData)->tryToGetSystemPowerCurrent()) { helperYield(); }
    myIS[index] = ((_EthercatDevice_DmpCommonDriver*)Setting->Slave[index].PrivateData)->getSystemPowerCurrent();

    refreshIsIp(Setting);

    return Setting->Slave[index].Is; 
}

double MyEVA::getIpCurrent(int aliasId)
{
    int index = -1;

    if (aliasId == -1) {
        refreshIsIp(Setting);
        return _master_Ip;
    }

    if (Setting == NULL)
        return -1;

    index = EcatMaster.getSlaveNo(aliasId);

    if (index < 0 || !Setting->Slave[index].haveMCU) return 0.0;

    while (!((_EthercatDevice_DmpCommonDriver*)Setting->Slave[index].PrivateData)->tryToGetPeripheralPowerCurrent()) { helperYield(); }
    myIP[index] = ((_EthercatDevice_DmpCommonDriver*)Setting->Slave[index].PrivateData)->getPeripheralPowerCurrent();

    refreshIsIp(Setting);

    return Setting->Slave[index].Ip; 
}

double MyEVA::getTemperature(int aliasId)
{
    int index = -1;

    if (aliasId == -1) return cpuTemperature();

    if (Setting == NULL)
        return -1;

    index = EcatMaster.getSlaveNo(aliasId);

    if (index < 0 || !Setting->Slave[index].haveMCU) return 0.0;

    while (!((_EthercatDevice_DmpCommonDriver*)Setting->Slave[index].PrivateData)->tryToGetSystemTemperature()) { helperYield(); }

    return ((_EthercatDevice_DmpCommonDriver*)Setting->Slave[index].PrivateData)->getSystemTemperature();
}
void _task_handler()
{
    uint8_t error;

    ((EthercatDevice_DmpStepper_Generic*)&Slave0)->update();

    if (enableCheckCBError) {
        error = 0;
        io_DisableINT();
        if (!queue_empty(queue))
            error = queue_pop(queue);
        io_RestoreINT();
        if (error)
            getErrorMessage(error);
    }
}
