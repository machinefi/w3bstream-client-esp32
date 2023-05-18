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
#if 0
    //方法一：
    int i, j;
    char ch;
    int add_len = (inlen % 3 == 0 ? 0 : 3 - inlen % 3); //原字符串需补齐的字符个数
    int in_len = inlen + add_len; //源字符串补齐字符后的长度，为3的倍数
    if(outdata != NULL) {
        //编码，长度为调整之后的长度，3字节一组
        for(i=0, j=0; i<in_len; i+=3, j+=4) {
            //将indata第一个字符向右移动2bit（丢弃2bit）
            ch = base64_encode_table[(unsigned char)indata[i] >> 2]; //对应base64转换表的字符
            outdata[j] = ch; //赋值

            //处理最后一组（最后3个字节）的数据
            if(i == in_len - 3 && add_len != 0) {
                if(add_len == 1) {
                    outdata[j + 1] = base64_encode_table[(((unsigned char)indata[i] & 0x03) << 4) | ((unsigned char)indata[i + 1] >> 4)];
                    outdata[j + 2] = base64_encode_table[((unsigned char)indata[i + 1] & 0x0f) << 2];
                    outdata[j + 3] = '=';
                }
                else if(add_len == 2) {
                    outdata[j + 1] = base64_encode_table[((unsigned char)indata[i] & 0x03) << 4];
                    outdata[j + 2] = '=';
                    outdata[j + 3] = '=';
                }
            }
            //处理正常的3字节数据
            else {
                outdata[j + 1] = base64_encode_table[(((unsigned char)indata[i] & 0x03) << 4) | ((unsigned char)indata[i + 1] >> 4)];
                outdata[j + 2] = base64_encode_table[(((unsigned char)indata[i + 1] & 0x0f) << 2) | ((unsigned char)indata[i + 2] >> 6)];
                outdata[j + 3] = base64_encode_table[(unsigned char)indata[i + 2] & 0x3f];
            }
        }
    }
    if(outlen != NULL) {
        *outlen = in_len * 4 / 3; //编码后的长度
    }
#endif
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

#if 0
/**
 * @brief base64_encode_file    base64编码文件
 * @param src                   需编码的文件路径
 * @param dst                   编码后输出的文件路径
 * @return  int                 0：成功    -1：无效参数     -2：文件操作失败
 */
int base64_encode_file(const char *src, const char *dst)
{
    if(src == NULL || dst == NULL) {
        return -1;
    }

    FILE *src_fp, *dst_fp;
    char rdata[128*3+3]; //存放读取到的文件数据，+3表示预留3个字节空间存放余下来的数据
    size_t rmemb; //读文件数据返回值，读取到的块数
    size_t nmemb = sizeof(rdata) - 3; //每次读取文件数据的块数，最好是3的倍数
    char encode_data[(nmemb+(nmemb%3==0?0:3-nmemb%3))*4/3+1]; //存放编码后的数据
    int encode_datalen; //编码后的数据大小
    unsigned char num = 0, lastnum = 0;

    src_fp = fopen(src, "rb");
    if(NULL == src_fp) {
        perror("open src file failed");
        return -2;
    }
    dst_fp = fopen(dst, "wb");
    if(NULL == dst_fp) {
        fclose(src_fp);
        perror("open dst file failed");
        return -2;
    }
    while(1) {
        //memset(rdata, 0, sizeof(rdata));
        //memset(encode_data, 0, sizeof(encode_data));
        encode_datalen = 0;
        rmemb = fread(rdata + lastnum, 1, nmemb, src_fp);
        if((lastnum + rmemb) % 3 == 0 || rmemb < nmemb) { //读取到的数据与上次余下来的数据总大小是3的倍数 或 文件已读完（或出错）
            base64_encode(rdata, lastnum + rmemb, encode_data, &encode_datalen);
            fwrite(encode_data, 1, encode_datalen, dst_fp);
            lastnum = 0;
        }
        else {
            num = (lastnum + rmemb) % 3; //余下来的字节数
            base64_encode(rdata, lastnum + rmemb - num, encode_data, &encode_datalen);
            fwrite(encode_data, 1, encode_datalen, dst_fp);
            //将余下来的数据移动至缓冲区最前面
            if(num == 1) {
                rdata[0] = rdata[lastnum + rmemb - 1];
            }
            else if(num == 2) {
                rdata[0] = rdata[lastnum + rmemb - 2];
                rdata[1] = rdata[lastnum + rmemb - 1];
            }
            lastnum = num;
        }
        if(rmemb < nmemb) { //文件已读完 或 出错
            break;
        }
    }
    fclose(src_fp);
    fclose(dst_fp);
    return 0;
}

/**
 * @brief base64_decode_file    base64解码文件
 * @param src                   需解码的文件路径
 * @param dst                   解码后输出的文件路径
 * @return  int                 0：成功    -1：无效参数     -2：文件操作失败
 */
int base64_decode_file(const char *src, const char *dst)
{
    if(src == NULL || dst == NULL) {
        return -1;
    }

    FILE *src_fp, *dst_fp;
    char rdata[128*4]; //存放读取到的文件数据
    size_t rmemb; //读文件数据返回值，读取到的块数
    size_t nmemb = sizeof(rdata); //每次读取文件数据的块数，最好是4的倍数
    char decode_data[nmemb/4*3+1]; //存放解码后的数据，大小计算
    int decode_datalen; //解码后的数据大小
    unsigned char num = 0, lastnum = 0;

    src_fp = fopen(src, "rb");
    if(NULL == src_fp) {
        perror("open src file failed");
        return -2;
    }
    dst_fp = fopen(dst, "wb");
    if(NULL == dst_fp) {
        fclose(src_fp);
        perror("open dst file failed");
        return -2;
    }
    while(1) {
        //memset(rdata, 0, sizeof(rdata));
        //memset(encode_data, 0, sizeof(encode_data));
        decode_datalen = 0;
        rmemb = fread(rdata + lastnum, 1, nmemb, src_fp);
        if((lastnum + rmemb) % 4 == 0 || rmemb < nmemb) { //读取到的数据与上次余下来的数据总大小是4的倍数 或 文件已读完（或出错）
            base64_decode(rdata, lastnum + rmemb, decode_data, &decode_datalen);
            fwrite(decode_data, 1, decode_datalen, dst_fp);
            lastnum = 0;
        }
        else {
            num = (lastnum + rmemb) % 4; //余下来的字节数
            base64_decode(rdata, lastnum + rmemb - num, decode_data, &decode_datalen);
            fwrite(decode_data, 1, decode_datalen, dst_fp);
            //将余下来的数据移动至缓冲区最前面
            if(num == 1) {
                rdata[0] = rdata[lastnum + rmemb - 1];
            }
            else if(num == 2) {
                rdata[0] = rdata[lastnum + rmemb - 2];
                rdata[1] = rdata[lastnum + rmemb - 1];
            }
            else if(num == 3) {
                rdata[0] = rdata[lastnum + rmemb - 3];
                rdata[1] = rdata[lastnum + rmemb - 2];
                rdata[2] = rdata[lastnum + rmemb - 1];
            }
            lastnum = num;
        }
        if(rmemb < nmemb) { //文件已读完 或 出错
            break;
        }
    }
    fclose(src_fp);
    fclose(dst_fp);
    return 0;
}

#endif
