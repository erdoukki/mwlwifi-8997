/*************************************************************************
**CopyRight 2018, Marvell Semiconductors,Inc.
**History
**Author:Ramkumar
**Last Update:07/10/18
*************************************************************************/


#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#define TX_PWR_LMT_MAX_BUFFER 3072
#define SUBBAND_HDR_LEN 6
unsigned char subBandTRPCInfo[1024];

char subBandName[5][30] = {"txpwrlimit_2g_cfg_set",
                        "txpwrlimit_5g_cfg_set_sub0",
                        "txpwrlimit_5g_cfg_set_sub1",
                        "txpwrlimit_5g_cfg_set_sub2",
                        "txpwrlimit_5g_cfg_set_sub3"};

char hexc2bin(char chr)
{
        if (chr >= '0' && chr <= '9')
                chr -= '0';
        else if (chr >= 'A' && chr <= 'F')
                chr -= ('A' - 10);
        else if (chr >= 'a' && chr <= 'f')
                chr -= ('a' - 10);

        return chr;
}

int a2hex(char *s)
{
        int val = 0;

        if (!strncasecmp("0x", s, 2)) {
                s += 2;
        }

        while (*s && isxdigit(*s)) {
                val = (val << 4) + hexc2bin(*s++);
        }

        return val;
}

int a2hex_or_atoi(char *value)
{
        if (value[0] == '0' && (value[1] == 'X' || value[1] == 'x')) {
                return a2hex(value + 2);
        } else if (isdigit(*value)) {
                return atoi(value);
        } else {
                return *value;
        }
}

char * mlan_config_get_line(FILE * fp, char *str, int size, int *lineno)
{
        char *start, *end;
        int out, next_line;

        if (!fp || !str)
                return NULL;

        do {
read_line:
                if (!fgets(str, size, fp))
                        break;
                start = str;
                start[size - 1] = '\0';
                end = start + strlen(str);
                (*lineno)++;

                out = 1;
                while (out && (start < end)) {
                        next_line = 0;
                        /* Remove empty lines and lines starting with # */
                        switch (start[0]) {
                        case ' ':       /* White space */
                        case '\t':      /* Tab */
                                start++;
                                break;
                        case '#':
                        case '\n':
                        case '\0':
                                next_line = 1;
                                break;
                        case '\r':
                                if (start[1] == '\n')
                                        next_line = 1;
                                else
                                        start++;
                                break;
                        default:
                                out = 0;
                                break;
                        }
                        if (next_line)
                                goto read_line;
                }

                /* Remove # comments unless they are within a double quoted
                 * string. Remove trailing white space. */
                if ((end = strstr(start, "\""))) {
                        if (!(end = strstr(end + 1, "\"")))
                                end = start;
                } else
                        end = start;

                if ((end = strstr(end + 1, "#")))
                        *end-- = '\0';
                else
                        end = start + strlen(start) - 1;

                out = 1;
                while (out && (start < end)) {
                        switch (*end) {
                        case ' ':       /* White space */
                        case '\t':      /* Tab */
                        case '\n':
                        case '\r':
                                *end = '\0';
                                end--;
                                break;
                        default:
                                out = 0;
                                break;
                        }
                }

                if (start == '\0')
                        continue;

                return start;
        } while (1);

        return NULL;
}

bool parseDateInfo(char *DateInfo)
{
	char date[20], *pos, *pos1;
	unsigned char val;
	
        snprintf(date, sizeof(date), "%s", DateInfo);
	pos = date;

	pos1 = strchr(pos, '-');
	if(pos1 == NULL) {
		return false;
	}

	*pos1++ = '\0';
        val = (unsigned char)a2hex_or_atoi(pos);
	printf("%02x ", val);

	pos = pos1;

        pos1 = strchr(pos, '-');
        if(pos1 == NULL) {
                return false;
        }

        *pos1++ = '\0';
        val = (unsigned char)a2hex_or_atoi(pos);
        printf("%02x ", val);
	pos = pos1;
        val = (unsigned char)a2hex_or_atoi(pos);
        printf("%02x \n", val);
	return true;
}

void  mlan_get_hostcmd_data(FILE * fp, int *ln, unsigned char *buf, int *size)
{
        int errors = 0, i, len;
        char line[256], *pos, *pos1, *pos2, *pos3;
	unsigned char val;

        while ((pos = mlan_config_get_line(fp, line, sizeof(line), ln))) {
                (*ln)++;
                if (strcmp(pos, "}") == 0) {
                        break;
                }

                pos1 = strchr(pos, ':');
                if (pos1 == NULL) {
                        printf("Line %d: Invalid hostcmd line '%s'\n", *ln,
                               pos);
                        errors++;
                        continue;
                }
                *pos1++ = '\0';

                pos2 = strchr(pos1, '=');
                if (pos2 == NULL) {
                        printf("Line %d: Invalid hostcmd line '%s'\n", *ln,
                               pos);
                        errors++;
                        continue;
                }
                *pos2++ = '\0';

                len = a2hex_or_atoi(pos1);
                if (len < 1 || len > TX_PWR_LMT_MAX_BUFFER) {
                        printf("Line %d: Invalid hostcmd line '%s'\n", *ln,
                               pos);
                        errors++;
                        continue;
                }

               *size += len;

                if (*pos2 == '\'') {
                        pos2++;
                        if ((pos3 = strchr(pos2, '\'')) == NULL) {
                                printf("Line %d: invalid quotation '%s'\n", *ln,
                                       pos);
                                errors++;
                                continue;
                        }
                        *pos3 = ',';
                        for (i = 0; i < len; i++) {
                                if ((pos3 = strchr(pos2, ',')) != NULL) {
                                        *pos3 = '\0';
                                        *buf++ = (unsigned char)a2hex_or_atoi(pos2);
                                        pos2 = pos3 + 1;
                                } else {
                                        *buf++ = 0;
				}
                        }
                } else {
                        int value = a2hex_or_atoi(pos2);
                        while (len--) {
                                *buf++ = (unsigned char)(value & 0xff);
                                value >>= 8;
                        }
                }
        }
}

void parseConfFile(FILE * fp)
{
        char line[256], blockname[128], cmdname[128], *pos;
        int ln = 0, idx, temp, temp_len, subband_len, trpc_entry_count, trpc_entry_len;
	bool SubBandStart = false;

        snprintf(cmdname, sizeof(cmdname), "ChanTRPC.TlvLength:2={");
	for(idx = 0; idx < 5; idx++)
	{
        	snprintf(blockname, sizeof(blockname), "%s={", subBandName[idx]);
		while(pos = mlan_config_get_line(fp, line, sizeof(line), &ln)) {
			if(strcmp(pos, blockname) == 0) {
				SubBandStart = true;
				subband_len = SUBBAND_HDR_LEN;
				trpc_entry_count = 0;
				memset(subBandTRPCInfo,0x0,sizeof(subBandTRPCInfo));
				while(pos = mlan_config_get_line(fp, line, sizeof(line), &ln)) {
					if(strcmp(pos, cmdname) == 0) {
						trpc_entry_count++;
                                		mlan_get_hostcmd_data(fp, &ln, subBandTRPCInfo + subband_len , &subband_len);
					} else if(*pos == '}') {
						subBandTRPCInfo[0] = idx;
						subBandTRPCInfo[2] = (subband_len & 0xFF);
						subBandTRPCInfo[3] = ((subband_len >> 8) & 0xFF);
						subBandTRPCInfo[4] = (trpc_entry_count & 0xFF);
						subBandTRPCInfo[5] = ((trpc_entry_count >> 8) & 0xFF);
						trpc_entry_len = (subband_len - SUBBAND_HDR_LEN)/trpc_entry_count ;
						temp = 0;
						temp_len = 0;
						while(temp < subband_len) {
                                                        printf("%02x ", subBandTRPCInfo[temp]);
							if(temp == (SUBBAND_HDR_LEN - 1)) {
								printf("\n");							
							} else if (temp >= SUBBAND_HDR_LEN) {
								if((++temp_len % trpc_entry_len) == 0) {
									printf("\n");
									temp_len = 0;
								}
							}
							temp++;
						}
						break;
					}
				}
			}
			if ((SubBandStart && (*pos == '}'))) {
				SubBandStart = false;
				break;
			}
		}
	}
}

int main(int argc, char *argv[])
{
        FILE *fp = NULL;
	char versionInfo;

        if (argc != 3) {
                printf("Error: invalid no of arguments \n");
                printf("Syntax: ./txpwrcfgParser DateInfo txpwrlmt_cfg.conf \n");
                printf("Example: ./txpwrcfgParser 07-02-18 txpwrlmt_cfg.conf \n");
                exit(1);
        }

	//For future. If the format changes then parsing needs to be done based on version number
	versionInfo = 0x00; 
    //Adding Signature. Driver checks for this signature before downloading
    //the conf file to device.
    printf ("01 0E 24 A1 \n");
	printf ("%02x ",versionInfo);

	parseDateInfo(argv[1]);

        if ((fp = fopen(argv[2], "r")) == NULL) {
                fprintf(stderr, "Cannot open file %s\n", argv[1]);
                exit(1);
        }

        parseConfFile(fp);
        fclose(fp);
}

