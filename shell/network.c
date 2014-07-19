//***********************************************************************/
//    Author                    : Garry
//    Original Date             : Jun 29,2014
//    Module Name               : network.h
//    Module Funciton           : 
//    Description               : 
//                                Network diagnostic application,common used
//                                network tools such as ping/tracert,are implemented
//                                in network.c file.
//    Last modified Author      :
//    Last modified Date        : 
//    Last modified Content     :
//                                1.
//                                2.
//    Lines number              :
//    Extra comment             : 
//***********************************************************************/

#ifndef __STDAFX_H__
#include "StdAfx.h"
#endif

#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/inet.h"

#include "kapi.h"
#include "shell.h"
#include "network.h"
#include "string.h"
#include "stdio.h"

//
//Pre-declare routines.
//
static DWORD CommandParser(LPSTR);
static DWORD help(__CMD_PARA_OBJ*);        //help sub-command's handler.
static DWORD exit(__CMD_PARA_OBJ*);        //exit sub-command's handler.
static DWORD iflist(__CMD_PARA_OBJ*);
static DWORD ping(__CMD_PARA_OBJ*);
//static DWORD telnet(__CMD_PARA_OBJ*);
//static DWORD tracert(__CMD_PARA_OBJ*);
static DWORD route(__CMD_PARA_OBJ*);

//
//The following is a map between command and it's handler.
//
static struct __FDISK_CMD_MAP{
	LPSTR                lpszCommand;
	DWORD                (*CommandHandler)(__CMD_PARA_OBJ*);
	LPSTR                lpszHelpInfo;
}SysDiagCmdMap[] = {
	{"iflist",     iflist,    "  iflist   : Show all network interface(s) in system."},
	{"ping",       ping,      "  ping     : Check a specified host's reachbility."},
	{"route",      route,     "  route    : List all route entry(ies) in system."},
	{"exit",       exit,      "  exit     : Exit the application."},
	{"help",       help,      "  help     : Print out this screen."},
	{NULL,		   NULL,      NULL}
};

//
//The following is a helper routine,it only prints out a "#" character as prompt.
//
static VOID PrintPound()
{
	WORD  wr = 0x0700;
	
	wr += '#';
	GotoHome();
	ChangeLine();
	PrintCh(wr);
}

//
//This is the application's entry point.
//
DWORD networkEntry(LPVOID p)
{
	CHAR                        strCmdBuffer[MAX_BUFFER_LEN];
	BYTE                        ucCurrentPtr                  = 0;
	BYTE                        bt;
	WORD                        wr                            = 0x0700;
	__KERNEL_THREAD_MESSAGE     Msg;
	DWORD                       dwRetVal;

	PrintPound();    //Print out the prompt.

	while(TRUE)
	{
		if(GetMessage(&Msg))
		{
			if(MSG_KEY_DOWN == Msg.wCommand)    //This is a key down message.
			{
				bt = (BYTE)Msg.dwParam;
				switch(bt)
				{
				case VK_RETURN:                //This is a return key.
					if(0 == ucCurrentPtr)      //There is not any character before this key.
					{
						PrintPound();
						break;
					}
					else
					{
						strCmdBuffer[ucCurrentPtr] = 0;    //Set the terminal flag.
						dwRetVal = CommandParser(strCmdBuffer);
						switch(dwRetVal)
						{
						case NET_CMD_TERMINAL: //Exit command is entered.
							goto __TERMINAL;
						case NET_CMD_INVALID:  //Can not parse the command.
							PrintLine("    Invalid command.");
							//PrintPound();
							break;
						case NET_CMD_FAILED:
							PrintLine("  Failed to process the command.");
							break;
						case NET_CMD_SUCCESS:      //Process the command successfully.
							//PrintPound();
							break;
						default:
							break;
						}
						ucCurrentPtr = 0;    //Re-initialize the buffer pointer.
						PrintPound();
					}
					break;
				case VK_BACKSPACE:
					if(ucCurrentPtr)
					{
						ucCurrentPtr --;
						GotoPrev();
					}
					break;
				default:
					if(ucCurrentPtr < MAX_BUFFER_LEN)    //The command buffer is not overflow.
					{
						strCmdBuffer[ucCurrentPtr] = bt;
						ucCurrentPtr ++;
						wr += bt;
						PrintCh(wr);
						wr  = 0x0700;
					}
					break;
				}
			}
			else
			{
				if(Msg.wCommand == KERNEL_MESSAGE_TIMER)
				{
					PrintLine("Timer message received.");
				}
			}
		}
	}

__TERMINAL:
	return 0;
}

//
//The following routine processes the input command string.
//
static DWORD CommandParser(LPSTR lpszCmdLine)
{
	DWORD                  dwRetVal          = NET_CMD_INVALID;
	DWORD                  dwIndex           = 0;
	__CMD_PARA_OBJ*        lpCmdParamObj     = NULL;

	if((NULL == lpszCmdLine) || (0 == lpszCmdLine[0]))    //Parameter check
	{
		return NET_CMD_INVALID;
	}

	lpCmdParamObj = FormParameterObj(lpszCmdLine);
	if(NULL == lpCmdParamObj)    //Can not form a valid command parameter object.
	{
		return NET_CMD_FAILED;
	}

	if(0 == lpCmdParamObj->byParameterNum)  //There is not any parameter.
	{
		return NET_CMD_FAILED;
	}

	//
	//The following code looks up the command map,to find the correct handler that handle
	//the current command.Calls the corresponding command handler if found,otherwise SYS_DIAG_CMD_PARSER_INVALID
	//will be returned to indicate this case.
	//
	while(TRUE)
	{
		if(NULL == SysDiagCmdMap[dwIndex].lpszCommand)
		{
			dwRetVal = NET_CMD_INVALID;
			break;
		}
		if(StrCmp(SysDiagCmdMap[dwIndex].lpszCommand,lpCmdParamObj->Parameter[0]))  //Find the handler.
		{
			dwRetVal = SysDiagCmdMap[dwIndex].CommandHandler(lpCmdParamObj);
			break;
		}
		else
		{
			dwIndex ++;
		}
	}

	//Release parameter object.
	if(NULL != lpCmdParamObj)
	{
		ReleaseParameterObj(lpCmdParamObj);
	}

	return dwRetVal;
}

//
//The exit command's handler.
//
static DWORD exit(__CMD_PARA_OBJ* lpCmdObj)
{
	return NET_CMD_TERMINAL;
}

//
//The help command's handler.
//
static DWORD help(__CMD_PARA_OBJ* lpCmdObj)
{
	DWORD               dwIndex = 0;

	while(TRUE)
	{
		if(NULL == SysDiagCmdMap[dwIndex].lpszHelpInfo)
			break;

		PrintLine(SysDiagCmdMap[dwIndex].lpszHelpInfo);
		dwIndex ++;
	}
	return NET_CMD_SUCCESS;
}

//route command's implementation.
static DWORD route(__CMD_PARA_OBJ* lpCmdObj)
{
	return NET_CMD_FAILED;
}

//ping command's implementation.
static DWORD ping(__CMD_PARA_OBJ* lpCmdObj)
{
	__PING_PARAM     PingParam;
	int              count    = 3;    //Ping counter.
	int              size     = 64;   //Ping packet size.
	ip_addr_t        ipAddr;
	DWORD            dwRetVal = NET_CMD_FAILED;
	//__CMD_PARA_OBJ*  pNext  = NULL;

	//Set default IP address if user does not specified.
	//ipAddr.addr = inet_addr("127.0.0.1");

	lpCmdObj = lpCmdObj->pNext;  //Skip the first parameter,since it is the command string itself.

	while(lpCmdObj)
	{
		if(lpCmdObj->byFunctionLabel == 0)  //Default parameter as IP address.
		{
			ipAddr.addr = inet_addr(lpCmdObj->Parameter[0]);
			//printf(" Ping target address is : %s\r\n",lpCmdObj->Parameter[0]);
		}
		if(lpCmdObj->byFunctionLabel == 'c')  //Ping counter.
		{
			if(NULL == lpCmdObj->pNext)
			{
				printf("  Invalid parameter.\r\n");
				goto __TERMINAL;
			}
			lpCmdObj = lpCmdObj->pNext;
			count = atoi(lpCmdObj->Parameter[0]);
		}
		if(lpCmdObj->byFunctionLabel == 'l')  //Packet size.
		{
			if(NULL == lpCmdObj->pNext)
			{
				printf("  Invalid parameter.\r\n");
				goto __TERMINAL;
			}
			lpCmdObj = lpCmdObj->pNext;
			size = atoi(lpCmdObj->Parameter[0]);
		}
		lpCmdObj = lpCmdObj->pNext;
	}

	PingParam.count      = count;
	PingParam.targetAddr = ipAddr;
	PingParam.size       = size;

	//Call ping entry routine.
	ping_Entry((void*)&PingParam);
	dwRetVal = NET_CMD_SUCCESS;

__TERMINAL:
	return dwRetVal;
}

//A helper routine used to dumpout a network interface.
static void ShowIf(struct netif* pIf)
{
	char    buff[128];
	
	//Print out all information about this interface.
	PrintLine("  --------------------------------------");
	sprintf(buff,"  Inetface name : %c%c",pIf->name[0],pIf->name[1]);
	PrintLine(buff);
	sprintf(buff,"      IPv4 address   : %s",inet_ntoa(pIf->ip_addr));
	PrintLine(buff);
	sprintf(buff,"      IPv4 mask      : %s",inet_ntoa(pIf->netmask));
	PrintLine(buff);
	sprintf(buff,"      IPv4 gateway   : %s",inet_ntoa(pIf->gw));
	PrintLine(buff);
	sprintf(buff,"      Interface MTU  : %d",pIf->mtu);
	PrintLine(buff);
}

//iflist command's implementation.
static DWORD iflist(__CMD_PARA_OBJ* lpCmdObj)
{
	struct netif* pIfList = netif_list;
	//char   buff[128];

	while(pIfList)  //Travel the whole list and dumpout everyone.
	{
		ShowIf(pIfList);
		pIfList = pIfList->next;
	}
	return NET_CMD_SUCCESS;
}

