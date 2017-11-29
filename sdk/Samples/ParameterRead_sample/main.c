/*
 * Copyright (c) 2017 Quantum.inc. All rights reserved.
 * This software is released under the MIT License, see LICENSE.txt.
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <error.h>
#include <system.h>
#include <hids.h>
#include <fs.h>

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
typedef void (*parameterCallback)(char *name, int16_t val);

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static int _initialize_user(void);
static void _parameter_parse_cb(char *name, int16_t val);

static uint8_t aiMini4WdParametersParseFromText(char *line, parameterCallback cb);


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static uint8_t sLED0 = 0;
static uint8_t sLED1 = 0;
static uint8_t sLED2 = 0;

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
int main(void)
{
	int ret = 0;
	
	ret = aiMini4WdSystemInitialize(AI_SYSTEM_FUNCTION_BASE | AI_SYSTEM_FUNCTION_FS | AI_SYSTEM_FUNCTION_ADNS9800);
	if (ret != AI_OK) {
		
	}
	
	ret = _initialize_user();
	if (ret != AI_OK) {
		
	}

	//J Parameter �t�@�C���̓W�J
	FIL param_file;
	ret = f_open (&param_file, "PARAM.TXT", FA_READ);
	if (ret == FR_OK) {
		char line[128];
		while (line == f_gets(line, sizeof(line), &param_file)) {
			aiMini4WdParametersParseFromText(line, _parameter_parse_cb);
		}
		
		f_close(&param_file);
	}

	if (sLED0) {
		aiMini4WdHidsSetLed(AI_MINI_4WD_LED0);
	}
	if (sLED1) {
		aiMini4WdHidsSetLed(AI_MINI_4WD_LED1);
	}
	if (sLED2) {
		aiMini4WdHidsSetLed(AI_MINI_4WD_LED2);
	}

	
	while (1) {
		;
	}	

	return 0;
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
static int _initialize_user(void)
{
	return AI_OK;
}

/*--------------------------------------------------------------------------*/
static void _parameter_parse_cb(char *name, int16_t val)
{
	if (strcmp("LED0", name) == 0) {
		sLED0 = (val & 0xff);
	}
	else if (strcmp("LED1", name) == 0) {
		sLED1 = (val & 0xff);
	}
	else if (strcmp("LED2", name) == 0) {
		sLED2 = (val & 0xff);
	}
}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
static char *_sSkipFrontSpace(char *str);
static char *_sRemoveNextSpace(char *str);
static uint8_t _sParseParamNameValuePair(char *str, char **name, int16_t *val);

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
//J ParameterName = Value
//J �Ƃ����`���ł��肢���܂��B
static uint8_t aiMini4WdParametersParseFromText(char *line, parameterCallback cb)
{
	if (line[0] == '#') {
		return 0;
	}
	
	if (cb == NULL) {
		return -1; //TODO
	}

	int16_t val = 0;
	char *name = NULL;
	uint8_t ret = _sParseParamNameValuePair(line, &name, &val);
	if (ret == 0) {
		cb(name, val);
	}

	return 0;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
static char *_sSkipFrontSpace(char *str)
{
	//J ���p�X�y�[�X�ƃ^�u�̊ԃ|�C���^��i�߂�
	while (*str == ' ' || *str == '\t') {
		str++;
	}
	
	return str;
}

/*---------------------------------------------------------------------------*/
static char *_sRemoveNextSpace(char *str)
{
	//J ���p�X�y�[�X�ƃ^�u�Ŗ����ԃ|�C���^��i�߂�
	while (*str != ' ' && *str != '\t') {
		str++;
	}
	
	//J �k�������ɒu������
	*str = '\0';
	
	//J �k��������1���Ƃ�Ԃ�
	return str + 1;
}

/*---------------------------------------------------------------------------*/
static uint8_t _sParseParamNameValuePair(char *str, char **name, int16_t *val)
{
	char *ptr = str;

	//J �擪�̋󔒂�����
	ptr = _sSkipFrontSpace(ptr);
	
	//J �p�����[�^����n��
	*name = str;
	
	//J ���̋󔒂��k�������ɒu��������
	ptr = _sRemoveNextSpace(ptr);
	ptr = _sSkipFrontSpace(ptr);
	
	//J �C�R�[�����΂�
	while (*ptr++ == '=') {
	}
	ptr = _sSkipFrontSpace(ptr);

	//J �l�������OK
	if (ptr[0] != '\0') {
		*val = strtol(ptr, NULL, 10);
		return 0;
		} else {
		*name = NULL;
		*val  = 0;
		return -1;
	}

	return -1;
}

