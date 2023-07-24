/**********************************************************************
 *
 * File name    : base64.cpp / base64.c
 * Function     : base64 encoding and decoding of data or file.
 * Created time : 2020-08-04
 *
 *********************************************************************/

#include <stdio.h>
#include <string.h>

//base64 编码转换表，共64个
static const char base64_encode_table[] = {
    'A','B','C','D','E','F','G','H','I','J',
    'K','L','M','N','O','P','Q','R','S','T',
    'U','V','W','X','Y','Z','a','b','c','d',
    'e','f','g','h','i','j','k','l','m','n',
    'o','p','q','r','s','t','u','v','w','x',
    'y','z','0','1','2','3','4','5','6','7',
    '8','9','+','/'
};

//base64 解码表
static const unsigned char base64_decode_table[] = {
    //每行16个
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,                //1 - 16
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,                //17 - 32
    0,0,0,0,0,0,0,0,0,0,0,62,0,0,0,63,              //33 - 48
    52,53,54,55,56,57,58,59,60,61,0,0,0,0,0,0,      //49 - 64
    0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,           //65 - 80
    15,16,17,18,19,20,21,22,23,24,25,0,0,0,0,0,     //81 - 96
    0,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40, //97 - 112
    41,42,43,44,45,46,47,48,49,50,51,0,0,0,0,0      //113 - 128
};

/**
 * @brief base64_encode     base64编码
 * @param indata            需编码的数据
 * @param inlen             需编码的数据大小
 * @param outdata           编码后输出的数据
 * @param outlen            编码后输出的数据大小
 * @return  int             0：成功    -1：无效参数
 */
int base64_encode(const char *indata, int inlen, char *outdata, int *outlen)
{
    if(indata == NULL || inlen <= 0) {
        return -1;
    }
    //方法二：
    int i, j;
    unsigned char num = inlen % 3;
    if(outdata != NULL) {
        //编码，3个字节一组，若数据总长度不是3的倍数，则跳过最后的 num 个字节数据
        for(i=0, j=0; i<inlen - num; i+=3, j+=4) {
            outdata[j] = base64_encode_table[(unsigned char)indata[i] >> 2];
            outdata[j + 1] = base64_encode_table[(((unsigned char)indata[i] & 0x03) << 4) | ((unsigned char)indata[i + 1] >> 4)];
            outdata[j + 2] = base64_encode_table[(((unsigned char)indata[i + 1] & 0x0f) << 2) | ((unsigned char)indata[i + 2] >> 6)];
            outdata[j + 3] = base64_encode_table[(unsigned char)indata[i + 2] & 0x3f];
        }
        //继续处理最后的 num 个字节的数据
        if(num == 1) { //余数为1，需补齐两个字节'='
            outdata[j] = base64_encode_table[(unsigned char)indata[inlen - 1] >> 2];
            outdata[j + 1] = base64_encode_table[((unsigned char)indata[inlen - 1] & 0x03) << 4];
            outdata[j + 2] = '=';
            outdata[j + 3] = '=';
        }
        else if(num == 2) { //余数为2，需补齐一个字节'='
            outdata[j] = base64_encode_table[(unsigned char)indata[inlen - 2] >> 2];
            outdata[j + 1] = base64_encode_table[(((unsigned char)indata[inlen - 2] & 0x03) << 4) | ((unsigned char)indata[inlen - 1] >> 4)];
            outdata[j + 2] = base64_encode_table[((unsigned char)indata[inlen - 1] & 0x0f) << 2];
            outdata[j + 3] = '=';
        }
    }
    if(outlen != NULL) {
        *outlen = (inlen + (num == 0 ? 0 : 3 - num)) * 4 / 3; //编码后的长度
    }

    return 0;
}

/**
 * @brief base64_decode     base64解码
 * @param indata            需解码的数据
 * @param inlen             需解码的数据大小
 * @param outdata           解码后输出的数据
 * @param outlen            解码后输出的数据大小
 * @return  int             0：成功    -1：无效参数
 * 注意：解码的数据的大小必须大于4，且是4的倍数
 */
int base64_decode(const char *indata, int inlen, char *outdata, int *outlen)
{
    if(indata == NULL || inlen <= 0 || (outdata == NULL && outlen == NULL)) {
        return -1;
    }
    if(inlen < 4 ||inlen % 4 != 0) { //需要解码的数据长度不是4的倍数  //inlen < 4 ||
        return -1;
    }

    int i, j;

    //计算解码后的字符串长度
    int len = inlen / 4 * 3;
    if(indata[inlen - 1] == '=') {
        len--;
    }
    if(indata[inlen - 2] == '=') {
        len--;
    }

    if(outdata != NULL) {
        for(i=0, j=0; i<inlen; i+=4, j+=3) {
            outdata[j] = (base64_decode_table[(unsigned char)indata[i]] << 2) | (base64_decode_table[(unsigned char)indata[i + 1]] >> 4);
            outdata[j + 1] = (base64_decode_table[(unsigned char)indata[i + 1]] << 4) | (base64_decode_table[(unsigned char)indata[i + 2]] >> 2);
            outdata[j + 2] = (base64_decode_table[(unsigned char)indata[i + 2]] << 6) | (base64_decode_table[(unsigned char)indata[i + 3]]);
        }
    }
    if(outlen != NULL) {
        *outlen = len;
    }
    return 0;
}
