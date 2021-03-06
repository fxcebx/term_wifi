#include "net.h"
#include "wifi.h"
#include "runcommand.h"
#include "settings.h"
#include "interactive.h"

#define ACT_INTERACTIVE 0
#define ACT_ADD    1
#define ACT_JOIN   2
#define ACT_LEAVE  3
#define ACT_LIST   4
#define ACT_SCAN   5
#define ACT_FORGET 6
#define ACT_HELP   99

ListNode *Interfaces=NULL;


int ParseCommandLine(int argc, char *argv[], TNet *Conf)
{
CMDLINE *CL;
int Act=ACT_INTERACTIVE;
const char *ptr;

CL=CommandLineParserCreate(argc, argv);

ptr=CommandLineFirst(CL);
if (StrValid(ptr))
{
if (strcmp(ptr, "list")==0) Act=ACT_LIST;
else if (strcmp(ptr, "scan")==0) 
{
	Conf->Interface=CopyStr(Conf->Interface, CommandLineNext(CL));
	if (! ListFindNamedItem(Interfaces, Conf->Interface))
	{
			printf("ERROR: '%s' is not an interface\n", Conf->Interface);
			printf("usage: %s scan <interface>\n", argv[0]);
			exit(1);
	}
	Act=ACT_SCAN;
}
else if (strcmp(ptr, "add")==0) 
{
	Act=ACT_ADD;
	Conf->Flags |= NET_STORE;
	Conf->ESSID=CopyStr(Conf->ESSID, CommandLineNext(CL));
	Conf->Address=CopyStr(Conf->Address, CommandLineNext(CL));
	if (strcasecmp(Conf->Address, "dhcp") !=0)
	{
		Conf->Netmask=CopyStr(Conf->Netmask, CommandLineNext(CL));
		Conf->Gateway=CopyStr(Conf->Gateway, CommandLineNext(CL));
		Conf->DNSServer=CopyStr(Conf->DNSServer, CommandLineNext(CL));
	}
}
else if (strcmp(ptr, "join")==0) 
{
	Act=ACT_JOIN;
	Conf->Interface=CopyStr(Conf->Interface, CommandLineNext(CL));
	if (! ListFindNamedItem(Interfaces, Conf->Interface))
	{
			printf("ERROR: '%s' is not an interface\n", Conf->Interface);
			printf("usage: %s join <interface> <essid>\n", argv[0]);
			exit(1);
	}

	Conf->ESSID=CopyStr(Conf->ESSID, CommandLineNext(CL));
}
else if (strcmp(ptr, "leave")==0) 
{
	Act=ACT_LEAVE;
	Conf->Interface=CopyStr(Conf->Interface, CommandLineNext(CL));
	if (! ListFindNamedItem(Interfaces, Conf->Interface))
	{
			printf("ERROR: '%s' is not an interface\n", Conf->Interface);
			printf("usage: %s join <interface> <essid>\n", argv[0]);
			exit(1);
	}
}
else if (strcmp(ptr, "forget")==0) 
{
	Act=ACT_FORGET;
	Conf->ESSID=CopyStr(Conf->ESSID, CommandLineNext(CL));
}
else if (strcmp(ptr, "help")==0) Act=ACT_HELP;
else if (strcmp(ptr, "-?")==0) Act=ACT_HELP;
else if (strcmp(ptr, "-h")==0) Act=ACT_HELP;
else if (strcmp(ptr, "-help")==0) Act=ACT_HELP;
else if (strcmp(ptr, "--help")==0) Act=ACT_HELP;
	

ptr=CommandLineNext(CL);
while (ptr)
{
	if (strcmp(ptr, "-i")==0) Conf->Interface=CopyStr(Conf->Interface, CommandLineNext(CL));
	else if (strcmp(ptr, "-ap")==0) Conf->AccessPoint=CopyStr(Conf->AccessPoint, CommandLineNext(CL));
	else if (strcmp(ptr, "-k")==0) Conf->Key=CopyStr(Conf->Key, CommandLineNext(CL));
	else if (strcmp(ptr, "-?")==0) Act=ACT_HELP;
	else if (strcmp(ptr, "-h")==0) Act=ACT_HELP;
	else if (strcmp(ptr, "-help")==0) Act=ACT_HELP;
	else if (strcmp(ptr, "--help")==0) Act=ACT_HELP;
	
	ptr=CommandLineNext(CL);
}
}

return(Act);
}


void ListNetworks()
{
ListNode *Curr, *Networks;
TNet *Net;

printf("Configured networks: \n");
Networks=SettingsLoadNets(NULL);
Curr=ListGetNext(Networks);
while (Curr)
{
Net=(TNet *) Curr->Item;
printf("% 15s ip:%s netmask:%s gateway:%s dns:%s\n", Net->ESSID, Net->Address, Net->Netmask, Net->Gateway, Net->DNSServer);
Curr=ListGetNext(Curr);
}

}


void ScanForNetworks(TNetDev *Dev)
{
ListNode *Networks, *Curr;
char *Tempstr=NULL, *Output=NULL;
TNet *Net;

ConfiguredNets=SettingsLoadNets(NULL);
Networks=WifiGetNetworks(Dev);
Curr=ListGetNext(Networks);
while (Curr)
{
Net=(TNet *) Curr->Item;

Output=OutputFormatNet(Output, Net);
Output=CatStr(Output, "\n");
TerminalPutStr(Output, StdIO);
Curr=ListGetNext(Curr);
}

ListDestroy(ConfiguredNets, NetDestroy);

Destroy(Tempstr);
Destroy(Output);
}


void DisplayHelp()
{
printf("version: %s\n", VERSION);
printf("usage:\n");
printf("  term_wifi scan <interface>                                          scan networks and output details\n");
printf("  term_wifi add <essid> <address> <netmask> <gateway>  <dns server>   add a config for a network\n");
printf("  term_wifi add <essid> dhcp                                          add a config for a network using dhcp\n");
printf("  term_wifi forget <essid>                                            delete (forget) network\n");
printf("  term_wifi list                                                      list configured networks\n");
printf("  term_wifi join <interface> <essid>                                  join a configured network\n");
printf("  term_wifi leave <interface>                                         leave current network\n");
}


int main(int argc, char *argv[])
{
ListNode *Networks, *Curr;
char *Tempstr=NULL;
TNet *Conf, *Net;
int Action;
TNetDev *Dev;

StdIO=STREAMFromDualFD(0, 1);
SettingsInit();
CommandsInit();

Interfaces=ListCreate();
NetGetInterfaces(Interfaces);

Conf=NetCreate();
Action=ParseCommandLine(argc, argv, Conf);

if (StrValid(Conf->Interface)) Curr=ListFindNamedItem(Interfaces, Conf->Interface);
else Curr=ListFindNamedItem(Interfaces, "wlan0");

if (Curr)
{
	Dev=(TNetDev *) Curr->Item;
	switch (Action)
	{
	case ACT_ADD:
		if (! StrValid(Conf->Key)) Conf->Key=TerminalReadPrompt(Conf->Key, "Key/password: ", 0, StdIO);
		SettingsConfigureNet(Conf);
		SettingsSaveNet(Conf);
	break;

	case ACT_JOIN:
		SettingsConfigureNet(Conf);
		if (Dev->Flags & DEV_WIFI) 
		{
			if (! StrValid(Conf->Key)) Conf->Key=TerminalReadPrompt(Conf->Key, "Key/password: ", 0, StdIO);
			printf("Configure wifi: dev:%s essid:%s\n", Conf->Interface, Conf->ESSID);
			WifiSetup(Dev, Conf);
		}

		Net=(TNet *) calloc(1, sizeof(TNet));
		while (1)
		{
		WifiGetStatus(Dev, Net);
		if (Net->Flags & NET_ASSOCIATED) break;
		}
		NetDestroy(Net);

		printf("Configure IPv4: ip:%s netmask:%s gw:%s dns:%s\n", Conf->Address, Conf->Netmask, Conf->Gateway, Conf->DNSServer);
		NetSetupInterface(Dev, Conf->Address, Conf->Netmask, Conf->Gateway, Conf->DNSServer);
		if (Conf->Flags & NET_STORE) SettingsSaveNet(Conf);
	break;

	case ACT_FORGET:
	SettingsForgetNet(Conf->ESSID);
	break;

	case ACT_LIST:
		ListNetworks();
	break;

	case ACT_SCAN:
		ScanForNetworks(Dev);
	break;

	case ACT_LEAVE:
		NetDown(Dev);
	break;

	case ACT_INTERACTIVE:
		Interactive(Dev);
	break;

	case ACT_HELP:
		DisplayHelp();
	break;
	}
}

Destroy(Tempstr);

return(0);
}
