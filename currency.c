#include <stdio.h>
#include <curl/curl.h>
#include <expat.h>
#include <string.h>

int fload(char *url, char *filename)
{
	CURL *curl;
	FILE *fp;
	fp = fopen(filename,"wb");
	if(!fp)
		return -1;
	curl = curl_easy_init();
	if(!curl)
	{
		fclose(fp);
		return -1;
	}
	curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
	CURLcode res = curl_easy_perform(curl);	
	fclose(fp);
	curl_easy_cleanup(curl);	
	return res;
}

struct xmlData
{
	int numCode;
	float value;
	char isNumCode;
	char isValue;
	char isFound;
};

static void XMLCALL start(void *userData, const char *el, const char **attr) {
	struct xmlData *data = (struct xmlData*)userData;
	if(!strcmp(el, "NumCode"))
		data->isNumCode = 1;
	else if(!strcmp(el, "Value"))
		data->isValue = 1;
}

static void XMLCALL end(void *userData, const char *tag)
{
	struct xmlData *data = (struct xmlData*)userData;
	data->isValue = 0;
	data->isNumCode = 0;
}

static void XMLCALL data_handle(void *userData, const XML_Char *s, int len)
{
	struct xmlData *data = (struct xmlData*)userData;
	if(data->isNumCode)
	{
		char *tmp = malloc(len + 1);
		memcpy(tmp, s, len);
		tmp[len] = 0;
		data->isFound = (data->numCode == atoi(tmp));
		free(tmp);
	}
	if(data->isValue && data->isFound)
	{
		char *tmp = malloc(len + 1);
		memcpy(tmp, s, len);
		tmp[len] = 0;
		char *c = strchr(tmp, ',');
		if(c != NULL)
			*c = '.';
		data->value = atof(tmp);
		free(tmp);
		data->isFound = 0;
	}
}

float valueSearch(char *filename, int numCode, const XML_Char *encoding)
{
	FILE *fp;
	char *buf[BUFSIZ];
	size_t len;
	char done;
	fp = fopen(filename, "r");
	if(!fp)
		return -1;
	XML_Parser parser = XML_ParserCreate(encoding);
	struct xmlData data;
	data.numCode = numCode;
	data.value = 0;
	data.isValue = 0;
	data.isNumCode = 0;
	data.isFound = 0;
	XML_SetUserData(parser, &data);
	XML_SetElementHandler(parser, start, end);
	XML_SetCharacterDataHandler(parser, data_handle);
	do
	{
		len = fread(buf, 1, BUFSIZ, fp);
		done = len < BUFSIZ;
		if(XML_Parse(parser,(const char*) buf, (int)len, done) == XML_STATUS_ERROR)
		{
			fprintf(stderr, "%s at line %lu\n",
              XML_ErrorString(XML_GetErrorCode(parser)),
              XML_GetCurrentLineNumber(parser));
			fclose(fp);
			XML_ParserFree(parser);
			return -1;
		}
	}while(!done);
	XML_ParserFree(parser);
	fclose(fp);
	return data.value;
}



int main(int argc, char *argv[])
{
	int numCode;
	if((argc < 2) || !(numCode = atoi(argv[1])))
		return -1;
	char *url = "http://www.cbr.ru/scripts/XML_daily.asp";
	char *name = "daily.xml";
	if(fload(url, name) != CURLE_OK)
		return -1;
	float value = valueSearch(name, numCode, "ISO-8859-1");
	if(value > 0)
		printf("%f\n", value);
	else
		return -1;
	return 0;
}
