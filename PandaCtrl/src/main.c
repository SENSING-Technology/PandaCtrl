#include <libusb-1.0/libusb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#define UVC_EXTENSION_UINITS_BUF_SIZE 33
#define FUNC_ID_FIRMWARE_VER        0x00
#define FUNC_ID_16BIT_REG_RD		0x01
#define FUNC_ID_16BIT_REG_WT		0x02
#define FUNC_ID_8BIT_REG_RD			0x03
#define FUNC_ID_8BIT_REG_WT			0x04
#define FUNC_ID_CHANGECONFIG		0x05
#define FUNC_ID_DEVICEIDENTIFY		0x06
#define FUNC_ID_CAMERAPROGRAM       0x07
#define FUNC_ID_LINKSTATUS			0x08
#define FUNC_ID_32BIT_REG_RD        0x09
#define FUNC_ID_32BIT_REG_WT        0x0A
#define FUNC_ID_SENSORSETTING       0x0B
#define FUNC_ID_GETCURRENT          0x0C
#define FUNC_ID_FSYNC_SWITCH		0x10
#define FUNC_ID_INNER_FSYNC_STATUS	0x11
#define FUNC_ID_SN_WRITE     		0x12
#define FUNC_ID_SN_READ         	0x13
#define FUNC_ID_I2CTRANSMIT			0x20
#define FUNC_ID_I2CRECEIVE			0x21
#define FUNC_ID_UVC_CONFIGDESC      0xF0
#define FUNC_ID_I2C_SWITCH			0xF1
#define FUNC_ID_TRIGGER_FPS         0xF2
#define FUNC_ID_FRAME_INFO			0xF3
#define FUNC_ID_LINK_TEST           0xF4    // 链路LOCK测试
#define FUNC_ID_POCPOWER_SWITCH		0xF5
//定义 libusb的参数
#define BREQUEST_SET_CUR   0X01
#define BREQUESTTYPE_SET   0x21
#define BREQUEST_GET_CUR   0X81
#define BREQUESTTYPE_GET   0xA1
#define JUSTSET            0x00
#define NEEDREAD           0x01
#define MODE_8BITREG_BYTEWRITE   0x01  //8bit寄存器地址 8bit 值
#define MODE_8BITREG_WORDWRITE   0x02  //8bit寄存器地址 16bit 值
#define MODE_16BITREG_BYTEWRITE  0x03  //16bit寄存器地址 8bit 值
#define MODE_16BITREG_WORDWRITE  0x04  //16bit寄存器地址 16bit 值
// 全局变量
uint8_t img_format = 0;
uint32_t wval = 0;
uint32_t hval = 0;

// 图像格式与对应的值
#define YUV422_YUYV   1
#define YUV422_UYVY   2
#define RAWDATA_12BIT 3
#define RAWDATA_10BIT 5
#define RAWDATA_8BIT  4
#define UVC_RAW10     51
#define UVC_RAW12     52
#define format_uvc_yuv422_8bit_mode2 62
#define Defalt_9296_IIC_addr  0x90

uint8_t writemod = MODE_16BITREG_BYTEWRITE;
uint8_t IIC_addr = Defalt_9296_IIC_addr;
uint16_t IIC_REG_ADDAR = 0x0000;
uint8_t  IIC_Write_Val = 0x00;

int  Reg16BitByteRead(uint8_t devAddr, uint32_t regAddr, uint16_t * regValue);
int  Reg16BitByteWrite(uint8_t devAddr, uint16_t regAddr, uint8_t regValue);
int  Reg16BitWordWrite(uint8_t devAddr, uint16_t regAddr, uint16_t regValue);
int  Reg16BitWordRead(uint8_t devAddr, uint32_t regAddr, uint16_t * regValue);
int Reg8BitByteWrite(uint8_t devAddr, uint8_t regAddr, uint8_t regValue);
void IICWrite(uint8_t writeMode , uint16_t rea_addr ,uint16_t val);
void parse_line(char *strLine, unsigned char *currentI2CAddr);
void trim(char* str);
int cx3_send_extension_request(
    libusb_device_handle *dev,
    uint8_t *data,
    uint16_t length,uint8_t controlStatus
);
libusb_device_handle *dev_handle = NULL;
int file_exists(const char *path) {
    return access(path, F_OK) == 0;  // F_OK检查文件是否存在
}
int main(int argc, char *argv[]) {
// 检查参数数量
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <config.ini>\n", argv[0]);
        return -1;
    }
 
    /* 文件存在检查 */
    if (!file_exists(argv[1])) {
        fprintf(stderr, "Error: Config file '%s' does not exist\n", argv[1]);
        return -1;
    }
    libusb_context *context = NULL;
  
    int r;
    r = libusb_init(&context);
    if (r < 0) {
    fprintf(stderr, "libusb_init failed: %s\n", libusb_error_name(r));
    return -1;
    }
    // 打开设备
    dev_handle = libusb_open_device_with_vid_pid(context, 0x04b4, 0x00c3);
    if (dev_handle == NULL) {
        fprintf(stderr, "Error opening device\n");
        libusb_exit(context);
        return -1;
    }
    if (r < 0) {
        fprintf(stderr, "Unable to reset device: %s\n", libusb_error_name(r));
    }
    // 先检查是否有内核驱动占用
    if (libusb_kernel_driver_active(dev_handle, 1) == 1) {
        printf("Kernel driver active, detaching...\n");
        r = libusb_detach_kernel_driver(dev_handle, 1);
        if (r < 0) {
        fprintf(stderr, "Failed to detach kernel driver: %s\n", libusb_error_name(r));
        libusb_close(dev_handle);
        libusb_exit(context);
        return -1;
    }
}
    // 确保没有内核驱动占用
    if (libusb_kernel_driver_active(dev_handle, 0) == 1) {
        libusb_detach_kernel_driver(dev_handle, 0);
    }
    // 设置设备配置
    usleep(100000);  // 延时100毫秒
    r = libusb_set_configuration(dev_handle, 1);
    if (r < 0) {
    fprintf(stderr, "Unable to set configuration: %s\n", libusb_error_name(r));
    return;
    }
    struct libusb_config_descriptor *config_desc;
    r = libusb_get_config_descriptor(dev_handle, 0, &config_desc);
    if (r == 0) {
    // 确保配置索引正确
    } else {
       // printf("Error getting config descriptor: %s\n", libusb_error_name(r));
    }
    r = libusb_claim_interface(dev_handle, 1);
    if (r < 0) {
        fprintf(stderr, "Unable to claim interface: %s\n", libusb_error_name(r));
        return -2;
    }
    // 声明并选择接口
    printf("Device opened successfully\n");
    uint16_t readVal = 0;

    sleep(1);
//Reg16BitByteWrite(0x90, 0x0320, 0x2C);
//----------

IIC_addr = 0x90;
    // 打开配置文件
    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
        perror("Unable to open file");
        return 1;
    }

    // 逐行读取文件并解析
    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        parse_line(line, &IIC_addr);
        usleep(10000); 
    }
    // 关闭文件
    fclose(file);
//-------------
sleep(1);
// Reg16BitByteRead(0x90, 0x0320, &readVal);
// printf("get 2 info readVal = %d" ,readVal);
    // 关闭设备
    
libusb_release_interface(dev_handle, 1);
usleep(100000);  // 延迟100毫秒
if (libusb_claim_interface(dev_handle, 1) < 0) {
    fprintf(stderr, "Unable to claim interface\n");
    libusb_attach_kernel_driver(dev_handle, 0);
    return -3;
}
libusb_attach_kernel_driver(dev_handle, 0);
 libusb_reset_device(dev_handle);
usleep(100000);  // 延迟100毫秒
libusb_close(dev_handle);
libusb_exit(context);

    return 0;
}





//16 8 read
int  Reg16BitByteRead(uint8_t devAddr, uint32_t regAddr, uint16_t * regValue)
{
    unsigned char data[UVC_EXTENSION_UINITS_BUF_SIZE];
    // send request
    memset(data, 0, UVC_EXTENSION_UINITS_BUF_SIZE);
    data[0] = FUNC_ID_16BIT_REG_RD;		// FUNC : 16Bit reg read
    data[1] = (devAddr & 0xFE) | 0x01;  // I2C ADDR
    data[2] = (regAddr & 0xFF00) >> 8;	// 16BIT REG ADDR HI
    data[3] = (regAddr & 0x00FF) >> 0;	// 16BIT REG ADDR LO
    data[4] = 1;                        // READ COUNT
    data[5] = 0;                        // order
    // 发送请求（触发UVCHandleExtensionUnitRqts）
     cx3_send_extension_request(dev_handle, data, sizeof(data),NEEDREAD);  
     * regValue =  data[6];                 
    return 0;
}
/* 
 * 16位寄存器写 8bit 值
 * 寄存器 16bit
 * 值 8bit
 */
int Reg16BitByteWrite(uint8_t devAddr, uint16_t regAddr, uint8_t regValue)
{
    unsigned char data[UVC_EXTENSION_UINITS_BUF_SIZE];
    // send request
    memset(data, 0, UVC_EXTENSION_UINITS_BUF_SIZE);
    data[0] = FUNC_ID_16BIT_REG_WT;		// FUNC : 16Bit reg write
    data[1] = devAddr & 0xFE;           // I2C ADDR
    data[2] = (regAddr & 0xFF00) >> 8;	// 16BIT REG ADDR HI
    data[3] = (regAddr & 0x00FF) >> 0;	// 16BIT REG ADDR LO
    data[4] = 1;                        // WRITE COUNT
    data[5] = 0;                        // order
    data[6] = regValue;                 // WRITE value
   
   cx3_send_extension_request(dev_handle, data, sizeof(data),NEEDREAD);      
        if (data[10] == 0) {
        }
        else {
            printf("  faild write i2c=0x%x regaddr = 0x%x  val = 0x%x\n",devAddr,regAddr,regValue);
        }   
    return 0;
}

/*
 * 16位寄存器写
 * 寄存器 16bit
 * 值 16bit
 */
int Reg16BitWordWrite(uint8_t devAddr, uint16_t regAddr, uint16_t regValue)
{
    unsigned char data[UVC_EXTENSION_UINITS_BUF_SIZE];

    // send request
    memset(data, 0, UVC_EXTENSION_UINITS_BUF_SIZE);
    data[0] = FUNC_ID_16BIT_REG_WT;                 // FUNC : 16Bit reg write
    data[1] = devAddr & 0xFE;                       // I2C ADDR
    data[2] = (regAddr & 0xFF00) >> 8;              // 16BIT REG ADDR HI
    data[3] = (regAddr & 0x00FF) >> 0;              // 16BIT REG ADDR LO
    data[4] = 2;                                    // WRITE COUNT
    data[5] = 0;                                    // order
    data[6] = (uint8_t)(regValue >> 8) & 0xFF;      // WRITE value hi-byte
    data[7] = (uint8_t)(regValue >> 0) & 0xFF;      // WRITE value lo-byte
    cx3_send_extension_request(dev_handle, data, sizeof(data),NEEDREAD);      
        if (data[10] == 0) {
        }
        else {
            printf("  faild write i2c=0x%x regaddr = 0x%x  val = 0x%x\n",devAddr,regAddr,regValue);
        }   
    return 0;
}

int  Reg16BitWordRead(uint8_t devAddr, uint32_t regAddr, uint16_t * regValue)
{
    unsigned char data[UVC_EXTENSION_UINITS_BUF_SIZE];
    // send request
    memset(data, 0, UVC_EXTENSION_UINITS_BUF_SIZE);
    data[0] = FUNC_ID_16BIT_REG_RD;		// FUNC : 16Bit reg read
    data[1] = (devAddr & 0xFE) | 0x01;  // I2C ADDR
    data[2] = (regAddr & 0xFF00) >> 8;	// 16BIT REG ADDR HI
    data[3] = (regAddr & 0x00FF) >> 0;	// 16BIT REG ADDR LO
    data[4] = 2;                        // READ COUNT
    data[5] = 0;                        // order
   
    cx3_send_extension_request(dev_handle, data, sizeof(data),NEEDREAD);
        if (data[10] == 0) {
            *regValue = ((uint32_t)data[6] << 8) | data[7];
            return 0;
        }
        else {
            regValue = 0;
            return -1;
        }
    

    return 0;
}
/*
 * 8位寄存器写
 * 寄存器 8bit
 * 值 8bit
 */
int Reg8BitByteWrite(uint8_t devAddr, uint8_t regAddr, uint8_t regValue)
{
    uint8_t data[UVC_EXTENSION_UINITS_BUF_SIZE];
    // send request
    memset(data, 0, UVC_EXTENSION_UINITS_BUF_SIZE);
    data[0] = FUNC_ID_8BIT_REG_WT;		// FUNC : 8Bit reg write
    data[1] = devAddr & 0xFE;			// I2C ADDR
    data[2] = (regAddr & 0xFF);			// 8BIT REG ADDR
    data[3] = 0;						// RESERVED
    data[4] = 1;                        // WRITE COUNT
    data[5] = 0;                        // order
    data[6] = regValue;                 // WRITE value
    cx3_send_extension_request(dev_handle, data, sizeof(data),NEEDREAD);
    if (data[10] == 0) {
        return 0;
    }
    else {
        return -1;
    }
    return 0;
}
int changeResolution(uint8_t format, uint16_t width, uint16_t height){
    uint8_t data[UVC_EXTENSION_UINITS_BUF_SIZE];
    memset(data, 0, UVC_EXTENSION_UINITS_BUF_SIZE);
    data[0] = FUNC_ID_UVC_CONFIGDESC;				// FUNC : 修改uvc分辨率和图像格式
    data[1] = (uint8_t)((width >> 8) & 0x00FF);	// width_HI
    data[2] = (uint8_t)((width >> 0) & 0x00FF);	// width_LO
    data[3] = (uint8_t)((height >> 8) & 0x00FF);	// height_HI
    data[4] = (uint8_t)((height >> 0) & 0x00FF);	// height_LO
    data[5] = (uint8_t)format;                      // format
    data[6] = 0;									// RESERVED
    data[7] = 0;									// RESERVED
    cx3_send_extension_request(dev_handle, data, sizeof(data),JUSTSET);   
    return 0;
}
int cx3_send_extension_request(
    libusb_device_handle *dev,
    uint8_t *data,
    uint16_t length,
    uint8_t controlStatus
) {
    uint8_t bRequest = 0x85;               // SET_CUR
    uint8_t bmRequestType = 0xa1   ;         // Class, Interface, Out
    uint16_t wValue = 0x0100;                 // Control Selector=1 (bmControls[0])
    uint16_t wIndex = 0x0300;                 // Extension Unit ID=3
   int ret = 0;
    bRequest = BREQUEST_SET_CUR;               // SET_CUR
    bmRequestType = BREQUESTTYPE_SET   ;         // Class, Interface, IN
        // Send the control transfer
    ret = libusb_control_transfer(
        dev,
        bmRequestType,
        bRequest,
        wValue,
        wIndex,
        data,
        length,
        1000 // Timeout
    );
    if(ret < 0){
         printf(" libusb_control_transfer fsild code = %d\n",ret);
         return -1;
    }
//read info from device   
if(NEEDREAD == controlStatus){
    bRequest = BREQUEST_GET_CUR;
    bmRequestType =  BREQUESTTYPE_GET ;         // Class, Interface, IN
    ret =  libusb_control_transfer(
        dev,
        bmRequestType,
        bRequest,
        wValue,
        wIndex,
        data,
        length,
        1000 // Timeout
    );
    if(ret < 0){
         printf(" libusb_control_transfer fsild code = %d\n",ret);
         return -1;
    }
    }  
    return ret;
}

void IICWrite(uint8_t writeMode , uint16_t rea_addr ,uint16_t val){
if(writeMode == MODE_16BITREG_BYTEWRITE){
    
    Reg16BitByteWrite(IIC_addr, (uint16_t) rea_addr, (uint8_t) val);
}else if(writeMode == MODE_16BITREG_WORDWRITE){
    Reg16BitWordWrite(IIC_addr, (uint16_t) rea_addr, (uint16_t) val);
}
else if(writeMode == MODE_8BITREG_BYTEWRITE){
    
    Reg8BitByteWrite(IIC_addr, (uint8_t) rea_addr, (uint8_t) val);
}
else if(writeMode == MODE_8BITREG_WORDWRITE){    
}
else{
    printf("writeMode error\n");
}
}

void parse_line(char *strLine, unsigned char *currentI2CAddr) {
    // 去掉注释部分（删除 // 或 # 后面的内容）
    char *commentPos = strchr(strLine, '/');
    if (commentPos != NULL && *(commentPos + 1) == '/') {
        *commentPos = '\0';  // 将注释开始的字符替换为字符串结束符
    } else {
        commentPos = strchr(strLine, '#');
        if (commentPos != NULL) {
            *commentPos = '\0';  // 去掉以 # 开头的注释
        }
    }

    // 跳过空行或去除两端的空格
    char *trimmedLine = strLine;
    while (*trimmedLine == ' ' || *trimmedLine == '\t') {
        trimmedLine++;  // 跳过前面的空格
    }

    if (*trimmedLine == '\0') {
        return;  // 如果行是空的，直接返回
    }

    // 解析 I2CADDR 配置项
    if (strncmp(trimmedLine, "I2CADDR", 7) == 0) {
        char *indexStart = strchr(trimmedLine, '=');
        if (indexStart != NULL) {
            indexStart++;  // 跳过 '='
            // 跳过空格
            while (*indexStart == ' ') {
                indexStart++;
            }
            char *indexEnd = strchr(indexStart, '\0');  // 查找字符串结尾
            if (indexEnd != NULL) {
                char i2cAddr[256];
                strncpy(i2cAddr, indexStart, indexEnd - indexStart);
                i2cAddr[indexEnd - indexStart] = '\0';  // 确保字符串结束
                sscanf(i2cAddr, "0x%hhx", currentI2CAddr);  // 更新I2C地址
                IIC_addr = *currentI2CAddr;
            }
        }
    }
    // 解析 MODE 配置项
    else if (strncmp(trimmedLine, "MODE", 4) == 0) {
        char *indexStart = strchr(trimmedLine, '=');
        if (indexStart != NULL) {
            indexStart++;  // 跳过 '='
            // 跳过空格
            while (*indexStart == ' ') {
                indexStart++;
            }
            char *indexEnd = strchr(indexStart, '\0');  // 查找字符串结尾
            if (indexEnd != NULL) {
                char mode[256];
                strncpy(mode, indexStart, indexEnd - indexStart);
                mode[indexEnd - indexStart] = '\0';  // 确保字符串结束
                // 删除字符串中的回车符和换行符
            for (int i = 0; i < strlen(mode); i++) {
                if (mode[i] == '\r' || mode[i] == '\n') {
                   mode[i] = '\0';  // 用空字符替换
                  }
                }
                trim(mode);
                 if (strcmp(mode, "8BITREG_BYTEWRITE") == 0) {
                    writemod = MODE_8BITREG_BYTEWRITE;
                } else if (strcmp(mode, "8BITREG_WORDWRITE") == 0) {
                   writemod = MODE_8BITREG_WORDWRITE; 
                } else if (strcmp(mode, "16BITREG_BYTEWRITE") == 0) {
                  writemod = MODE_16BITREG_BYTEWRITE;
                } else if (strcmp(mode, "16BITREG_WORDWRITE") == 0) {
                    writemod = MODE_16BITREG_WORDWRITE;
                 } else {
                    writemod = MODE_16BITREG_BYTEWRITE;
                 printf("Unsupported MODE: %s\n", mode);
             }
            }
        }
    }
    // 解析 REG 配置项
    else if (strncmp(trimmedLine, "REG", 3) == 0) {
    char *indexStart = strchr(trimmedLine, '=');
    if (indexStart != NULL) {
        indexStart++;  // 跳过 '='
        
        // 跳过空格
        while (*indexStart == ' ') {
            indexStart++;
        }
        
        char *indexEnd = strchr(indexStart, '\0');  // 查找字符串结尾
        if (indexEnd != NULL) {
            char regData[256];
            strncpy(regData, indexStart, indexEnd - indexStart);
            regData[indexEnd - indexStart] = '\0';  // 确保字符串结束
            // 解析寄存器地址和值，假设它们是通过逗号分隔的
            uint16_t regAddr = 0;
            uint16_t writeVal = 0;
            int numValuesParsed = 0;

            // 使用逗号分割字符串
            char *token = strtok(regData, ",");
            while (token != NULL) {
                if (numValuesParsed == 0) {
                    // 解析寄存器地址，转换为16进制
                    sscanf(token, "%hx", &regAddr);
                } else if (numValuesParsed == 1) {
                    // 解析写入值，可能是8-bit或16-bit
                    sscanf(token, "%hx", &writeVal);
                }
                numValuesParsed++;
                token = strtok(NULL, ",");  // 获取下一个值
            }

            // 根据解析的数据填充全局变量
            IIC_REG_ADDAR = regAddr;
            if (numValuesParsed > 1) {
                // 如果解析了写入值，则赋值给 IIC_Write_Val
                IIC_Write_Val = (uint8_t)(writeVal & 0xFF);  // 确保值为8-bit
            }
            IICWrite(writemod , IIC_REG_ADDAR ,IIC_Write_Val);
        }
    }
}
    // 解析 DELAY 配置项
    else if (strncmp(trimmedLine, "DELAY", 5) == 0) {
        char *indexStart = strchr(trimmedLine, '=');
        if (indexStart != NULL) {
            indexStart++;  // 跳过 '='
            // 跳过空格
            while (*indexStart == ' ') {
                indexStart++;
            }
            int delay;
            if (sscanf(indexStart, "%d", &delay) == 1) {
               // printf("Parsed DELAY: %d ms\n", delay);
            }
        }
    }
     // 解析 RESOLUTION 配置项
    // 解析 RESOLUTION 配置项
// 解析 RESOLUTION 配置项
else if (strncmp(trimmedLine, "RESOLUTION", 10) == 0) {
    char *indexStart = strchr(trimmedLine, '=');
    if (indexStart != NULL) {
        indexStart++;  // 跳过 '='

        // 跳过空格
        while (*indexStart == ' ') {
            indexStart++;
        }

        // 解析图像格式和分辨率
        char resolution[256];
        strncpy(resolution, indexStart, sizeof(resolution) - 1);
        resolution[sizeof(resolution) - 1] = '\0';  // 确保字符串结束

        // 打印解析出的分辨率
        char *token = strtok(resolution, ",");
        
        // 处理图像格式
        if (token != NULL) {
            if (strcmp(token, "YUV422_YUYV") == 0) {
                img_format = YUV422_YUYV;
            } else if (strcmp(token, "YUV422_UYVY") == 0) {
                img_format = YUV422_UYVY;
            } else if (strcmp(token, "RAWDATA_12BIT") == 0) {
                img_format = RAWDATA_12BIT;
            } else if (strcmp(token, "RAWDATA_10BIT") == 0) {
                img_format = RAWDATA_10BIT;
            } else if (strcmp(token, "RAWDATA_8BIT") == 0) {
                img_format = RAWDATA_8BIT;
            } else if (strcmp(token, "UVC_RAW10") == 0) {
                img_format = UVC_RAW10;
            } else if (strcmp(token, "UVC_RAW12") == 0) {
                img_format = UVC_RAW12;
            }
            else if (strcmp(token, "UVC_RAW12") == 0) {
                img_format = UVC_RAW12;
            }
            else if (strcmp(token, "UVC_YUV422_8BIT_MODE2") == 0) {
                img_format = format_uvc_yuv422_8bit_mode2;
            }
        
        }

        // 解析分辨率（宽度和高度）
        token = strtok(NULL, ",");
        if (token != NULL) {
            wval = (uint16_t)atoi(token);  // 解析宽度
        }
 
        token = strtok(NULL, ",");  // 获取高度
        if (token != NULL) {
            
            hval = (uint16_t)atoi(token);  // 将高度转换为数字并赋值
        }
    }
    changeResolution(img_format, wval, hval);
}
}


void trim(char* str) {
    int start = 0;
    int end = strlen(str) - 1;
    
    // 去除前导空格
    while (start <= end && str[start] == ' ') start++;
    
    // 去除尾部空格
    while (end >= start && str[end] == ' ') end--;
    
    // 剪切字符串
    for (int i = start; i <= end; i++) {
        str[i - start] = str[i];
    }
    str[end - start + 1] = '\0'; // 结束符
}