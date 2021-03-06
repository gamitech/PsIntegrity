#include "stdafx.h"

DWORD
DisplayHelp(
	VOID
);

DWORD
DisplayBanner(
	VOID
);

DWORD
ParseArguments(
	_In_ int argc,
	_In_ wchar_t **argv
);

HANDLE gProcHeap = NULL;
BOOLEAN gUseCurrentProcess = FALSE;
KA_PS_OPERATION gOperation = OperationNone;
DWORD gPid = 0;
KA_OBJECT_INTEGRITY gIntegrity = ObjectIntegrityNone;
PTCHAR gKeyPath = NULL;
PTCHAR gFilePath = NULL;
PTCHAR gProcessPath = NULL;
KA_OBJECT_INTEGRITY gProcessIntegrity = ObjectIntegrityNone;

int wmain(int argc, wchar_t **argv)
{
	TCHAR ProcessName[MAX_PATH] = { 0 };
	DWORD ProcessNameLength = sizeof(ProcessName);
	DWORD Integrity = 0;
	DWORD PolicyMask = 0;
	DWORD Error = 0;
	BOOL Result = FALSE;
	BOOL DefaultIntegrity = FALSE;

	DisplayBanner();

	gProcHeap = GetProcessHeap();
	if (!gProcHeap)
	{
		return GetLastError();
	}

	if (ParseArguments(argc, argv) != ERROR_SUCCESS)
	{
		return ERROR_INVALID_PARAMETER;
	}

	if (gUseCurrentProcess)
	{
		printf("PsIntegrity.exe - Displaying current process integrity level\r\n");
		KaPsGetProcessIntegrityLevelWithPolicyByPid(GetCurrentProcessId(), &Integrity, &PolicyMask);
		printf("Press enter display help...\r\n");
		getchar();
		printf("\r\n");
		DisplayHelp();
		return ERROR_SUCCESS;
	}

	if (gOperation == OperationNone)
	{
		printf("Error parsing command line arguments. Must specify an operation: -query | -set\r\n");
		DisplayHelp();
		return ERROR_INVALID_PARAMETER;
	}

	if (gOperation == OperationSet && (gIntegrity <= ObjectIntegrityNone || gIntegrity >= ObjectIntegrityMax))
	{
		printf("Error parsing command line arguments. Integrity level is invalid\r\n");
		DisplayHelp();
		return ERROR_INVALID_PARAMETER;
	}

	if (gPid == 0 && gKeyPath == NULL && gFilePath == NULL && (gOperation == OperationSet || gOperation == OperationQuery))
	{
		printf("Error parsing command line arguments. Must specify an object to query or set\r\n");
		DisplayHelp();
		return ERROR_INVALID_PARAMETER;
	}

	if (gOperation == OperationCreateProcess && gProcessPath == NULL)
	{
		printf("Error parsing command line arguments. Process start path argument missing\r\n");
		DisplayHelp();
		return ERROR_INVALID_PARAMETER;
	}

	if (gOperation == OperationCreateProcess && (gProcessIntegrity <= ObjectIntegrityNone || gProcessIntegrity >= ObjectIntegrityMax))
	{
		printf("Error parsing command line arguments. Process start integrity argument invalid\r\n");
		DisplayHelp();
		return ERROR_INVALID_PARAMETER;
	}

	switch (gOperation)
	{
		case OperationQuery:
		{
			if (gPid)
			{
				KaPsGetProcessNameById(gPid, ProcessName, ProcessNameLength);
				printf("PsIntegrity.exe - Displaying integrity level for pid: %ld\r\n", gPid);
				printf("Process name: %S\r\n", ProcessName);
				KaPsGetProcessIntegrityLevelWithPolicyByPid(gPid, &Integrity, &PolicyMask);
				printf("\r\n");
			}

			if (gFilePath)
			{
				printf("PsIntegrity.exe - Displaying integrity level for file: %S\r\n", gFilePath);
				KaPsGetObjectIntegrityLabelByName(gFilePath, SE_FILE_OBJECT, &DefaultIntegrity, &Integrity, &PolicyMask);
				if (DefaultIntegrity)
				{
					wprintf(L"Medium Integrity file\r\n");
					wprintf(L"Account sid name: %s(%s)\r\n", KA_DEFAULT_MANDATORY_LABEL_SID_ACCOUNT_NAME, PS_INTEGRITY_SID_MEDIUM);
				}
			}

			if (gKeyPath)
			{
				printf("PsIntegrity.exe - Displaying integrity level for registry key: %S\r\n", gKeyPath);
				KaPsGetObjectIntegrityLabelByName(gKeyPath, SE_REGISTRY_KEY, &DefaultIntegrity, &Integrity, &PolicyMask);
				if (DefaultIntegrity)
				{
					wprintf(L"Medium Integrity key\r\n");
					wprintf(L"Account sid name: %s(%s)\r\n", KA_DEFAULT_MANDATORY_LABEL_SID_ACCOUNT_NAME, PS_INTEGRITY_SID_MEDIUM);
				}
				
			}

		}
		break;

		case OperationSet:
		{
			if (gFilePath)
			{
				printf("PsIntegrity.exe - Setting integrity level for file: %S\r\n", gFilePath);
				Error = KaPsSetObjectIntegrityLabelWithPolicyMaskByName(gFilePath, SE_FILE_OBJECT, gIntegrity, KA_DEFAULT_MANDATORY_LABEL_POLICY_MASK);
			}

			if (gKeyPath)
			{
				printf("PsIntegrity.exe - Setting integrity level for registry key: %S\r\n", gKeyPath);
				Error = KaPsSetObjectIntegrityLabelWithPolicyMaskByName(gKeyPath, SE_REGISTRY_KEY, gIntegrity, KA_DEFAULT_MANDATORY_LABEL_POLICY_MASK);
			}

			if (gPid)
			{
				printf("Cannot set running process integrity. Start a new process with the desired integrity instead\r\n");
			}

			if (Error == ERROR_SUCCESS)
			{
				printf("Done. Succeded in settig new integrity level\r\n");
			}
			else
			{
				printf("Failed. Could not set new integrity level. Error: %ld\r\n", Error);
			}

		}
		break;

		case OperationCreateProcess:
		{
			if (gProcessPath && gProcessIntegrity)
			{
				printf("PsIntegrity.exe - Starting process with path: %S and integrity level: %ld\r\n", gProcessPath, gProcessIntegrity);
				Result = KaPsCreateProcessWithIntegrityLevel(gProcessPath, gProcessIntegrity, KA_DEFAULT_MANDATORY_LABEL_POLICY_MASK);
				if (!Result)
				{
					Error = GetLastError();
					printf("Failed. Could not start process with integrity level. Error: %ld\r\n", Error);
				}
				else
				{
					printf("Done. Succeded in starting process with integrity level\r\n");
				}
			}
		}
		break;

		default:
			printf("Invalid operation specified\r\n");
		break;
	}

	return 0;
}

DWORD
DisplayBanner(
	VOID
	)
{
	printf("PsIntegrity v1.0 - Display or sets objects integrity levels:\r\nCopyright(C) 2018 - Gabriel Bercea\r\nKasardia Util Tools - www.kasardia.com\r\n\r\n");
	return 0;
}

DWORD
DisplayHelp(
	VOID
	)
{
	printf("Usage : PsIntegrity.exe [[-query] [-set <IntegrityLevel>] [[-pid <ProcessId>] [-key <RegKeyPath>] [-file <FilePath>]]] [-start_process <IntegrityLevel> <ProcessPath>]\r\n");
	printf("Running with no parameters will display the integrity level of the current instance of PsIntegrity.exe\r\n");
	printf("Integrity Levels:\r\n1 - Untrusted integrity level\r\n2 - Low integrity level\r\n3 - Medium Integrity Level\r\n4 - High Integrity Level\r\n5 - System Integrity Level\r\n");
	printf("<RegKeypath>:\r\nA registry key object can be in the local registry, such as CLASSES_ROOT\\SomePath or in a remote registry, such as \\ComputerName\\CLASSES_ROOT\\SomePath. The names of registry keys must use the following literal strings to identify the predefined registry keys : \"CLASSES_ROOT\", \"CURRENT_USER\", \"MACHINE\", and \"USERS\".\r\n");
	printf("<RegKeypath> examples:\r\nMACHINE\\Software\\Adobe\r\nUSERS\\S-1-5-18\\Software\r\n\r\n");

	return ERROR_SUCCESS;
}

DWORD 
ParseArguments(
	_In_ int argc,
	_In_ wchar_t **argv
	)
{
	int i;

	if (argc == 0 || !argv)
	{
		return ERROR_INVALID_PARAMETER;
	}

	if (argc == 1)
	{
		gUseCurrentProcess = TRUE;
		return ERROR_SUCCESS;
	}

	for (i = 1; i < argc; i++)
	{
		if (_wcsnicmp(L"-start_process", argv[i], wcslen(L"-start_process")) == 0)
		{
			if (gOperation == OperationNone)
			{
				gOperation = OperationCreateProcess;
			}
			else
			{
				printf("Parsing error, cannot specify more than one operation in the same command line\r\n");
				return ERROR_INVALID_PARAMETER;
			}

			if (argc > i + 2)
			{
				gProcessIntegrity = (KA_OBJECT_INTEGRITY)_wtoi(argv[i + 1]);
				if (gProcessIntegrity == 0)
				{
					printf("Parsing error, invalid Integrity value provided as parameter\r\n");
					return ERROR_INVALID_PARAMETER;
				}

				if (gProcessIntegrity <= ObjectIntegrityNone || gProcessIntegrity >= ObjectIntegrityMax)
				{
					printf("Parsing error, Integrity value provided out of the supported limits\r\n");
					return ERROR_INVALID_PARAMETER;
				}

				gProcessPath = argv[i + 2];
				i += 2;
			}
			else
			{
				printf("Parsing error, expected Integrity value and process path\r\n");
				return ERROR_INVALID_PARAMETER;
			}
		}
		else if (_wcsnicmp(L"-query", argv[i], wcslen(L"-query")) == 0)
		{
			if (gOperation == OperationNone)
			{
				gOperation = OperationQuery;
			}
			else
			{
				printf("Parsing error, cannot specify more than one operation in the same command line\r\n");
				return ERROR_INVALID_PARAMETER;
			}
		}
		else if (_wcsnicmp(L"-set", argv[i], wcslen(L"-set")) == 0)
		{
			if (gOperation == OperationNone)
			{
				gOperation = OperationSet;
			}
			else
			{
				printf("Parsing error, cannot specify more than one operation in the same command line\r\n");
				return ERROR_INVALID_PARAMETER;
			}

			if (argc > i + 1)
			{
				gIntegrity = (KA_OBJECT_INTEGRITY)_wtoi(argv[i + 1]);
				if (gIntegrity == 0)
				{
					printf("Parsing error, invalid Integrity value provided as parameter\r\n");
					return ERROR_INVALID_PARAMETER;
				}

				if (gIntegrity <= ObjectIntegrityNone || gIntegrity >= ObjectIntegrityMax)
				{
					printf("Parsing error, Integrity value provided out of the supported limits\r\n");
					return ERROR_INVALID_PARAMETER;
				}

				i++;
			}
			else
			{
				printf("Parsing error, expected Integrity value\r\n");
				return ERROR_INVALID_PARAMETER;
			}
		}
		else if (_wcsnicmp(L"-pid", argv[i], wcslen(L"-pid")) == 0)
		{
			if (gPid)
			{
				printf("Parsing error, cannot specify more than one PID\r\n");
				return ERROR_INVALID_PARAMETER;
			}

			if (argc > i + 1)
			{
				gPid = _wtoi(argv[i + 1]);
				if (gPid == 0)
				{
					printf("Parsing error, invalid PID value provided as parameter\r\n");
					return ERROR_INVALID_PARAMETER;
				}
			}
			else
			{
				printf("Parsing error, expected PID value\r\n");
				return ERROR_INVALID_PARAMETER;
			}

			i++;
		}
		else if (_wcsnicmp(L"-key", argv[i], wcslen(L"-key")) == 0)
		{
			if (gKeyPath)
			{
				printf("Parsing error, cannot specify more than one key path\r\n");
				return ERROR_INVALID_PARAMETER;
			}

			if (argc > i + 1)
			{
				gKeyPath = argv[i + 1];
			}
			else
			{
				printf("Parsing error, expected key path\r\n");
				return ERROR_INVALID_PARAMETER;
			}

			i++;
		}
		else if (_wcsnicmp(L"-file", argv[i], wcslen(L"-file")) == 0)
		{
			if (gFilePath)
			{
				printf("Parsing error, cannot specify more than one file path\r\n");
				return ERROR_INVALID_PARAMETER;
			}

			if (argc > i + 1)
			{
				gFilePath = argv[i + 1];
			}
			else
			{
				printf("Parsing error, expected key path\r\n");
				return ERROR_INVALID_PARAMETER;
			}

			i++;
		}
		else
		{
			printf("Unknown argument: %S\r\n", argv[i]);
		}
	}

	return ERROR_SUCCESS;
}

DWORD KaPsGetAccountSidName(
	_In_  PSID Sid,
	_In_  BOOLEAN IncludeDomain,
	_Out_ PTCHAR *AccountName,
	_Out_ PDWORD AccountNameLength
	)
{
	BOOL Result = TRUE;

	PTCHAR TotalBuffer = NULL;
	DWORD  TotalSize = 0;

	PTCHAR AcctName = NULL;
	PTCHAR DomainName = NULL;
	DWORD AcctNameLength = 0, DomainNameLength = 0;

	SID_NAME_USE SidNameUse = SidTypeUnknown;

	if (!Sid || !AccountName || !AccountNameLength)
	{
		return ERROR_INVALID_PARAMETER;
	}

	// First call to LookupAccountSid to get the buffer sizes.
	Result = LookupAccountSidW(
		NULL,           // local computer
		Sid,
		AcctName,
		(LPDWORD)&AcctNameLength,
		DomainName,
		(LPDWORD)&DomainNameLength,
		&SidNameUse);

	TotalSize = (AcctNameLength + DomainNameLength + 2) * sizeof(WCHAR);
	TotalBuffer = (PTCHAR)HeapAlloc(gProcHeap, 0, TotalSize);
	if (!TotalBuffer)
	{
		return GetLastError();
	}

	DomainName = TotalBuffer;
	AcctName = &TotalBuffer[DomainNameLength];
	RtlZeroMemory(TotalBuffer, TotalSize);

	// Second call to LookupAccountSid to get the account name.
	Result = LookupAccountSid(
		NULL,                   // name of local or remote computer
		Sid,					// security identifier
		AcctName,               // account name buffer
		(LPDWORD)&AcctNameLength,   // size of account name buffer 
		DomainName,             // domain name
		(LPDWORD)&DomainNameLength, // size of domain name buffer
		&SidNameUse);                 // SID type

									  // Check GetLastError for LookupAccountSid error condition.
	if (Result == FALSE)
	{
		HeapFree(gProcHeap, 0, TotalBuffer);
		return GetLastError();
	}

	if (IncludeDomain)
	{
		TotalBuffer[DomainNameLength] = L'\\';
		*AccountNameLength = AcctNameLength + DomainNameLength + 2 * sizeof(L'\\');
	}
	else
	{
		RtlMoveMemory(TotalBuffer, AcctName, AcctNameLength);
		*AccountNameLength = AcctNameLength;
	}

	*AccountName = TotalBuffer;

	return ERROR_SUCCESS;
}

BOOL
KaPsCreateProcessWithIntegrityLevel(
	_In_ PTCHAR Path,
	_In_ KA_OBJECT_INTEGRITY Integrity,
	_In_ DWORD PolicyMask
	)
{
	BOOL                  Result;

	HANDLE                hToken = NULL;
	HANDLE                hNewToken = NULL;
	TOKEN_MANDATORY_LABEL TokenIntegrity = { 0 };
	TOKEN_MANDATORY_POLICY TokenPolicy = { 0 };

	PSID                  IntegritySid = NULL;
	PTCHAR				  StringSidToUse = NULL;

	PROCESS_INFORMATION   ProcInfo = { 0 };
	STARTUPINFO           StartupInfo = { 0 };

	if (!Path)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return ERROR_INVALID_PARAMETER;
	}

	if (Integrity <= ObjectIntegrityNone || Integrity >= ObjectIntegrityMax)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return ERROR_INVALID_PARAMETER;
	}

	if (!(PolicyMask & ( SYSTEM_MANDATORY_LABEL_NO_WRITE_UP | SYSTEM_MANDATORY_LABEL_NO_READ_UP | SYSTEM_MANDATORY_LABEL_NO_EXECUTE_UP)))
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return ERROR_INVALID_PARAMETER;
	}

	do
	{
		Result = OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE | TOKEN_ADJUST_DEFAULT | TOKEN_QUERY | TOKEN_ASSIGN_PRIMARY, &hToken);
		if (!Result)
		{
			break;
		}

		Result = DuplicateTokenEx(hToken, 0, NULL, SecurityImpersonation, TokenPrimary, &hNewToken);
		if (!Result)
		{
			break;
		}

		switch (Integrity)
		{
			case ObjectIntegrityUntrusted:
				StringSidToUse = (PTCHAR)PS_INTEGRITY_SID_UNTRUSTED;
				break;

			case ObjectIntegrityLow:
				StringSidToUse = (PTCHAR)PS_INTEGRITY_SID_LOW;
				break;

			case ObjectIntegrityMedium:
				StringSidToUse = (PTCHAR)PS_INTEGRITY_SID_MEDIUM;
				break;

			case ObjectIntegrityHigh:
				StringSidToUse = (PTCHAR)PS_INTEGRITY_SID_HIGH;
				break;

			case ObjectIntegritySystem:
				StringSidToUse = (PTCHAR)PS_INTEGRITY_SID_SYSTEM;
				break;

			default:
				StringSidToUse = (PTCHAR)PS_INTEGRITY_SID_LOW;
				break;
		}

		Result = ConvertStringSidToSid(StringSidToUse, &IntegritySid);
		if (!Result)
		{
			break;
		}

		TokenIntegrity.Label.Attributes = SE_GROUP_INTEGRITY;
		TokenIntegrity.Label.Sid = IntegritySid;

		//
		// Set the process integrity level
		//

		Result = SetTokenInformation(hNewToken, TokenIntegrityLevel, &TokenIntegrity, sizeof(TOKEN_MANDATORY_LABEL));
		if (!Result)
		{
			break;
		}

		//
		// Set the process policy mask
		//

		TokenPolicy.Policy = PolicyMask;

		Result = SetTokenInformation(hNewToken, TokenMandatoryPolicy, &TokenPolicy, sizeof(TOKEN_MANDATORY_POLICY));

		//
		// Create the new process at the specified integrity level with the specified policy
		//

		ZeroMemory(&StartupInfo, sizeof(STARTUPINFO));
		StartupInfo.cb = sizeof(STARTUPINFO);

		Result = CreateProcessAsUser(hNewToken, NULL, Path, NULL, NULL, FALSE, 
			NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE, NULL, NULL, &StartupInfo, &ProcInfo);
	} while (0);

	if (ProcInfo.hProcess != NULL)
	{
		CloseHandle(ProcInfo.hProcess);
	}

	if (ProcInfo.hThread != NULL)
	{
		CloseHandle(ProcInfo.hThread);
	}

	if (IntegritySid)
	{
		LocalFree(IntegritySid);
	}

	if (hNewToken != NULL)
	{
		CloseHandle(hNewToken);
	}

	if (hToken != NULL)
	{
		CloseHandle(hToken);
	}

	return Result;
}

DWORD
KaPsSetProcessIntegrityLevelWithPolicyMaskByPid(
	_In_ DWORD Pid,
	_In_ KA_OBJECT_INTEGRITY NewIntegrity,
	_In_ DWORD PolicyMask
	)
{
	DWORD Error = 0;
	BOOL Result;

	BOOLEAN PriviledgeEnabled = FALSE;

	HANDLE hToken = NULL;
	HANDLE hProcess = NULL;
	BOOLEAN IsCurrentProcess = FALSE;	

	TOKEN_MANDATORY_LABEL TokenLabel = { 0 };
	TOKEN_MANDATORY_POLICY TokenPolicy = { 0 };
	PTCHAR StringSidToUse = NULL;

	if (NewIntegrity <= ObjectIntegrityNone || NewIntegrity >= ObjectIntegrityMax)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return ERROR_INVALID_PARAMETER;
	}

	if (!(PolicyMask & (SYSTEM_MANDATORY_LABEL_NO_WRITE_UP | SYSTEM_MANDATORY_LABEL_NO_READ_UP | SYSTEM_MANDATORY_LABEL_NO_EXECUTE_UP)))
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return ERROR_INVALID_PARAMETER;
	}

	if (Pid == 0)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return ERROR_INVALID_PARAMETER;
	}

	if (Pid == GetCurrentProcessId())
	{
		IsCurrentProcess = TRUE;
		hProcess = GetCurrentProcess();
	}
	else
	{
		IsCurrentProcess = FALSE;
		hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, Pid);
	}

	if (!hProcess)
	{
		return GetLastError();
	}

	do
	{
		Result = OpenProcessToken(hProcess, TOKEN_QUERY | TOKEN_ADJUST_DEFAULT | TOKEN_ASSIGN_PRIMARY, &hToken);
		if (Result == FALSE)
		{
			break;
		}

		switch (NewIntegrity)
		{
		case ObjectIntegrityUntrusted:
			StringSidToUse = (PTCHAR)PS_INTEGRITY_SID_UNTRUSTED;
		break;

		case ObjectIntegrityLow:
			StringSidToUse = (PTCHAR)PS_INTEGRITY_SID_LOW;
			break;

		case ObjectIntegrityMedium:
			StringSidToUse = (PTCHAR)PS_INTEGRITY_SID_MEDIUM;
			break;

		case ObjectIntegrityHigh:
			StringSidToUse = (PTCHAR)PS_INTEGRITY_SID_HIGH;
			break;

		case ObjectIntegritySystem:
			StringSidToUse = (PTCHAR)PS_INTEGRITY_SID_SYSTEM;
			break;

		default:
			StringSidToUse = (PTCHAR)PS_INTEGRITY_SID_LOW;
			break;
		}

		Result = ConvertStringSidToSid(StringSidToUse, &TokenLabel.Label.Sid);
		if (!Result)
		{
			Error = GetLastError();
			break;
		}

		Result = KaPsSetProcessPrivilege(GetCurrentProcess(), (PTCHAR)SE_TCB_NAME, TRUE);
		if (!Result)
		{
			Error = GetLastError();
			break;
		}

		PriviledgeEnabled = TRUE;


		TokenLabel.Label.Attributes = SE_GROUP_INTEGRITY | SE_GROUP_INTEGRITY_ENABLED;
		Result =  SetTokenInformation(hToken, TokenIntegrityLevel, &TokenLabel, sizeof(TOKEN_MANDATORY_LABEL) + GetLengthSid(TokenLabel.Label.Sid));
		if (!Result)
		{
			Error = GetLastError();
			break;
		}

		TokenPolicy.Policy = PolicyMask;

		Result = SetTokenInformation(hToken, TokenMandatoryPolicy, &TokenPolicy, sizeof(TOKEN_MANDATORY_POLICY));
		if (!Result)
		{
			Error = GetLastError();
			break;
		}

		KaPsSetProcessPrivilege(GetCurrentProcess(), (PTCHAR)SE_TCB_NAME, FALSE);
		PriviledgeEnabled = FALSE;

	} while (0);

	if (TokenLabel.Label.Sid)
	{
		LocalFree(TokenLabel.Label.Sid);
		TokenLabel.Label.Sid = NULL;
	}

	if (PriviledgeEnabled)
	{
		KaPsSetProcessPrivilege(GetCurrentProcess(), (PTCHAR)SE_TCB_NAME, FALSE);
	}

	return Error;
}

DWORD 
KaPsGetProcessIntegrityLevelWithPolicyByPid(
	DWORD Pid,
	_Out_ PDWORD IntegrityRID,
	_Out_opt_ PDWORD PolicyMask
	)
{
	HANDLE hToken = NULL;
	HANDLE hProcess = NULL;
	BOOLEAN IsCurrentProcess = FALSE;

	BOOL Result;
	DWORD Error = ERROR_SUCCESS;

	PTOKEN_MANDATORY_LABEL TokenIntegrity = NULL;
	TOKEN_MANDATORY_POLICY TokenPolicy = { 0 };
	DWORD LengthNeeded;
	DWORD IntegrityLevel;

	PTCHAR AccountSidName = NULL;
	DWORD AccountSidNameLength = 0;
	PTCHAR SidString = NULL;
	

	if (!IntegrityRID)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return ERROR_INVALID_PARAMETER;
	}

	if (Pid == 0)
	{ 
		SetLastError(ERROR_INVALID_PARAMETER);
		return ERROR_INVALID_PARAMETER;
	}

	if (Pid == GetCurrentProcessId())
	{
		IsCurrentProcess = TRUE;
		hProcess = GetCurrentProcess();
	}
	else
	{
		IsCurrentProcess = FALSE;
		hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, Pid);
	}

	if (!hProcess)
	{
		return GetLastError();
	}

	do
	{
		Result = OpenProcessToken(hProcess, TOKEN_QUERY , &hToken);
		if (Result == FALSE)
		{
			break;
		}

		Result = GetTokenInformation(hToken, TokenIntegrityLevel, NULL, 0, &LengthNeeded);
		if (!Result)
		{
			Error = GetLastError();
			if (Error == ERROR_INSUFFICIENT_BUFFER)
			{
				TokenIntegrity = (PTOKEN_MANDATORY_LABEL)HeapAlloc(gProcHeap, 0, LengthNeeded);
				if (TokenIntegrity == NULL)
				{
					break;
				}

				Result = GetTokenInformation(hToken, TokenIntegrityLevel, TokenIntegrity, LengthNeeded, &LengthNeeded);
				if (!Result)
				{
					break;
				}
			}
			else
			{
				break;
			}
		}

		if (PolicyMask)
		{
			Result =  GetTokenInformation(hToken, TokenMandatoryPolicy, &TokenPolicy, sizeof(TokenPolicy), &LengthNeeded);
			if (Result)
			{
				*PolicyMask = TokenPolicy.Policy;
			}
		}

		Error = KaPsGetAccountSidName(TokenIntegrity->Label.Sid, TRUE, &AccountSidName, &AccountSidNameLength);
		if (Error != ERROR_SUCCESS)
		{
			break;
		}

		Result = ConvertSidToStringSid(TokenIntegrity->Label.Sid, &SidString);
		if (!Result)
		{
			break;
		}

		IntegrityLevel = *GetSidSubAuthority(TokenIntegrity->Label.Sid, (DWORD)(UCHAR)(*GetSidSubAuthorityCount(TokenIntegrity->Label.Sid) - 1));
		if (IntegrityLevel == SECURITY_MANDATORY_LOW_RID)
		{
			// Low Integrity
			wprintf(L"Low Integrity Process\r\n");
		}
		else if (IntegrityLevel >= SECURITY_MANDATORY_MEDIUM_RID &&
			IntegrityLevel < SECURITY_MANDATORY_HIGH_RID)
		{
			// Medium Integrity
			wprintf(L"Medium Integrity Process\r\n");
		}
		else if (IntegrityLevel >= SECURITY_MANDATORY_HIGH_RID)
		{
			// High Integrity
			wprintf(L"High Integrity Process\r\n");
		}
		else if (IntegrityLevel >= SECURITY_MANDATORY_SYSTEM_RID)
		{
			// System Integrity
			wprintf(L"System Integrity Process\r\n");
		}

		wprintf(L"Account sid name: %s(%s)\r\n", AccountSidName, SidString);

		Error = ERROR_SUCCESS;

	} while (0);
	
	if (!IsCurrentProcess && hProcess)
	{
		CloseHandle(hProcess);
		hProcess = NULL;
	}

	if (hToken)
	{
		CloseHandle(hToken);
		hToken = NULL;
	}

	if (TokenIntegrity)
	{
		HeapFree(gProcHeap, 0, TokenIntegrity);
		TokenIntegrity = NULL;
	}

	if (AccountSidName)
	{
		HeapFree(gProcHeap, 0, AccountSidName);
		AccountSidName = NULL;
	}

	if (SidString)
	{
		LocalFree(SidString);
		SidString = NULL;
	}

	return Error;
}

DWORD
KaPsGetObjectIntegrityLabelByName(
	_In_  PTCHAR ObjectName,
	_In_ SE_OBJECT_TYPE ObjectType, 
	_Out_ PBOOL DefaultIntegrity,
	_Out_ PDWORD IntegrityRID,
	_Out_opt_ PDWORD PolicyMask
	)
{
	DWORD Error = 0;
	BOOL Result;

	PACL Sacl = NULL;
	PSECURITY_DESCRIPTOR SecDesc = NULL;
	ACL_SIZE_INFORMATION AclSizeInfo = { 0 };
	
	PTCHAR AccountSidName = NULL;
	DWORD AccountSidNameLength;
	PTCHAR SidString = NULL;
	PSID Sid = NULL;
	
	union {
		PVOID Ace;
		PSYSTEM_MANDATORY_LABEL_ACE AceLabel;
		PACE_HEADER AceHeader;
	};

	union {
		PUCHAR pc;
		PULONG pl;
	};

	if (!ObjectName || !IntegrityRID || (ObjectType <= SE_UNKNOWN_OBJECT_TYPE || ObjectType > SE_REGISTRY_WOW64_64KEY) || !DefaultIntegrity)
	{
		return ERROR_INVALID_PARAMETER;
	}

	//
	// according to documentation if not label security ACE is found in the SACL then the default medium
	// integrity label is applied to the object
	// 
	*IntegrityRID = SECURITY_MANDATORY_MEDIUM_RID;// default LABEL
	if (PolicyMask)
	{
		*PolicyMask = SYSTEM_MANDATORY_LABEL_NO_WRITE_UP;
	}

	do
	{
		Error = GetNamedSecurityInfo(ObjectName, ObjectType, LABEL_SECURITY_INFORMATION, 0, 0, 0, &Sacl, &SecDesc);
		if (Error != ERROR_SUCCESS)
		{
			assert(SecDesc == NULL);
			return Error;
		}

		if (!Sacl)
		{
			//
			// no Sacl her = default medium label for this object
			// 
			Error = ERROR_SUCCESS;

			*DefaultIntegrity = TRUE;
			break;
		}

		Result = GetAclInformation(Sacl, &AclSizeInfo, sizeof(AclSizeInfo), AclSizeInformation);
		if (!Result)
		{
			Error = GetLastError();
			break;
		}

		if (AclSizeInfo.AceCount == 0)
		{
			//
			// Medium integrity should the Sacl have 0 ACE in it's list when querying for LABEL_SECURITY_INFORMATION
			// 
			Error = ERROR_SUCCESS;

			*DefaultIntegrity = TRUE;
			break;
		}

		if (AclSizeInfo.AceCount > 1)
		{
			//
			// There is no case where the file has 2 integrity labels. This must be an error.
			// 
			
			SetLastError(ERROR_GEN_FAILURE);
			Error = ERROR_GEN_FAILURE;
			break;
		}

		Result = GetAce(Sacl, 0, &Ace);
		if (!Result)
		{
			Error = GetLastError();
			break;
		}

		assert(AceHeader->AceType == SYSTEM_MANDATORY_LABEL_ACE_TYPE);
		if (AceHeader->AceType != SYSTEM_MANDATORY_LABEL_ACE_TYPE)
		{
			SetLastError(ERROR_GEN_FAILURE);
			Error = ERROR_GEN_FAILURE;
			break;
		}

		if (PolicyMask)
		{
			*PolicyMask = AceLabel->Mask;
		}

		Sid = &AceLabel->SidStart;
		pc = GetSidSubAuthorityCount(Sid);
		if (pc)
		{
			if (*pc == 1 && !memcmp(&LabelAuth, GetSidIdentifierAuthority(Sid), sizeof(SID_IDENTIFIER_AUTHORITY)))
			{
				pl = GetSidSubAuthority(Sid, 0);
				if (pl)
				{
					*IntegrityRID = *pl;
				}
			}
		}

		Error = KaPsGetAccountSidName(Sid, TRUE, &AccountSidName, &AccountSidNameLength);
		if (Error != ERROR_SUCCESS)
		{
			break;
		}

		Result = ConvertSidToStringSid(Sid, &SidString);
		if (!Result)
		{
			LocalFree(SecDesc);
			Error = GetLastError();
			break;
		}

		if (*IntegrityRID == SECURITY_MANDATORY_LOW_RID)
		{
			// Low Integrity
			wprintf(L"Low Integrity object\r\n");
		}
		else if (*IntegrityRID >= SECURITY_MANDATORY_MEDIUM_RID &&
			*IntegrityRID < SECURITY_MANDATORY_HIGH_RID)
		{
			// Medium Integrity
			wprintf(L"Medium Integrity object\r\n");
		}
		else if (*IntegrityRID >= SECURITY_MANDATORY_HIGH_RID)
		{
			// High Integrity
			wprintf(L"High Integrity object\r\n");
		}
		else if (*IntegrityRID >= SECURITY_MANDATORY_SYSTEM_RID)
		{
			// System Integrity
			wprintf(L"System Integrity object\r\n");
		}

		wprintf(L"Account sid name: %s(%s)\r\n", AccountSidName, SidString);

		Error = ERROR_SUCCESS;

	} while (0);

	if (SecDesc)
	{
		LocalFree(SecDesc);
		SecDesc = NULL;
	}

	if (SidString)
	{
		LocalFree(SidString);
		SidString = NULL;
	}

	if (AccountSidName)
	{
		HeapFree(gProcHeap, 0, AccountSidName);
		AccountSidName = NULL;
	}

	return Error;
}

DWORD
KaPsSetObjectIntegrityLabelByName(
	_In_ PTCHAR ObjectName,
	_In_ SE_OBJECT_TYPE ObjectType,
	_In_ KA_OBJECT_INTEGRITY NewIntegrity	
	)
{
	DWORD Error = ERROR_SUCCESS;

	PSECURITY_DESCRIPTOR SecDesc = NULL;

	PACL Sacl = NULL; // not allocated
	BOOL IsSaclPresent = FALSE;
	BOOL IsSaclDefaulted = FALSE;
	PTCHAR SDDLToUse = NULL;

	if (NewIntegrity <= ObjectIntegrityNone ||
		NewIntegrity >= ObjectIntegrityMax)
	{
		return ERROR_INVALID_PARAMETER;
	}

	switch (NewIntegrity)
	{
	case ObjectIntegrityUntrusted:
		SDDLToUse = (PTCHAR)PS_INTEGRITY_SID_UNTRUSTED;
		break;

	case ObjectIntegrityLow:
		SDDLToUse = (PTCHAR)LOW_INTEGRITY_SDDL_SACL_W;
		break;

	case ObjectIntegrityMedium:
		SDDLToUse = (PTCHAR)MEDIUM_INTEGRITY_SDDL_SACL_W;
		break;

	case ObjectIntegrityHigh:
		SDDLToUse = (PTCHAR)HIGH_INTEGRITY_SDDL_SACL_W;
		break;

	case ObjectIntegritySystem:
		SDDLToUse = (PTCHAR)SYSTEM_INTEGRITY_SDDL_SACL_W;
		break;

	default:
		SDDLToUse = (PTCHAR)LOW_INTEGRITY_SDDL_SACL_W;
		break;
	}

	if (ConvertStringSecurityDescriptorToSecurityDescriptorW( SDDLToUse, SDDL_REVISION_1, &SecDesc, NULL))
	{
		if (GetSecurityDescriptorSacl(SecDesc, &IsSaclPresent, &Sacl, &IsSaclDefaulted))
		{
			// Note that psidOwner, psidGroup, and pDacl are 
			// all NULL and set the new LABEL_SECURITY_INFORMATION
			Error = SetNamedSecurityInfoW(ObjectName, ObjectType, LABEL_SECURITY_INFORMATION, NULL, NULL, NULL, Sacl);
		}
		LocalFree(SecDesc);
	}

	return Error;
}

DWORD
KaPsInitializeMandatoryLabelSacl(
	_Out_ PACL *Sacl,
	_In_ KA_OBJECT_INTEGRITY Integrity,
	_In_ DWORD PolicyMask
	)
{
	DWORD Error = ERROR_SUCCESS;
	BOOL Result = FALSE;

	PSYSTEM_MANDATORY_LABEL_ACE LabelAce = NULL;

	PSID IntegritySid = NULL;
	PTCHAR StringSidToUse = NULL;
	DWORD SidLength = 0;

	if (!Sacl || Integrity <= ObjectIntegrityNone || Integrity >= ObjectIntegrityMax ||
		(!(PolicyMask & (SYSTEM_MANDATORY_LABEL_NO_WRITE_UP | SYSTEM_MANDATORY_LABEL_NO_READ_UP | SYSTEM_MANDATORY_LABEL_NO_EXECUTE_UP))))
	{
		return ERROR_INVALID_PARAMETER;
	}

	do
	{
		switch (Integrity)
		{
		case ObjectIntegrityUntrusted:
			StringSidToUse = (PTCHAR)PS_INTEGRITY_SID_UNTRUSTED;
			break;

			case ObjectIntegrityLow:
				StringSidToUse = (PTCHAR)PS_INTEGRITY_SID_LOW;
			break;

			case ObjectIntegrityMedium:
				StringSidToUse = (PTCHAR)PS_INTEGRITY_SID_MEDIUM;
			break;

			case ObjectIntegrityHigh:
				StringSidToUse = (PTCHAR)PS_INTEGRITY_SID_HIGH;
			break;

			case ObjectIntegritySystem:
				StringSidToUse = (PTCHAR)PS_INTEGRITY_SID_SYSTEM;
			break;

			default:
				StringSidToUse = (PTCHAR)PS_INTEGRITY_SID_LOW;
			break;
		}

		Result = ConvertStringSidToSid(StringSidToUse, &IntegritySid);
		if (!Result)
		{
			Error = GetLastError();
			break;
		}

		SidLength = GetLengthSid(IntegritySid);

		LabelAce = (PSYSTEM_MANDATORY_LABEL_ACE)HeapAlloc(gProcHeap, 0, sizeof(SYSTEM_MANDATORY_LABEL_ACE) + SidLength);
		if (!LabelAce)
		{
			Error = GetLastError();
			break;
		}

		LabelAce->Header.AceSize = (WORD)(sizeof(SYSTEM_MANDATORY_LABEL_ACE) + SidLength);
		LabelAce->Header.AceType = SYSTEM_MANDATORY_LABEL_ACE_TYPE;
		LabelAce->Header.AceFlags = 0;
		LabelAce->Mask = PolicyMask;
		RtlCopyMemory(&LabelAce->SidStart, IntegritySid, SidLength);

		*Sacl = (PACL)HeapAlloc(gProcHeap, 0, sizeof(ACL) + LabelAce->Header.AceSize);
		if (!Sacl)
		{
			Error = GetLastError();
			break;
		}


		Result = InitializeAcl(*Sacl, sizeof(ACL) + LabelAce->Header.AceSize, ACL_REVISION);
		if (!Result)
		{
			Error = GetLastError();
			break;
		}

		Result = AddAce(*Sacl, ACL_REVISION, 0, LabelAce, LabelAce->Header.AceSize);
		if (!Result)
		{
			Error = GetLastError();
			break;
		}

	} while (0);

	if (IntegritySid)
	{
		LocalFree(IntegritySid);
		IntegritySid = NULL;
	}

	if (LabelAce)
	{
		HeapFree(gProcHeap, 0, LabelAce);
		LabelAce = NULL;
	}

	if (*Sacl && Error != ERROR_SUCCESS)
	{
		HeapFree(gProcHeap, 0, *Sacl);
		*Sacl = NULL;
	}

	return Error;
}

DWORD
KaPsSetObjectIntegrityLabelWithPolicyMaskByName(
	_In_ PTCHAR ObjectName,
	_In_ SE_OBJECT_TYPE ObjectType,
	_In_ KA_OBJECT_INTEGRITY NewIntegrity,
	_In_ DWORD PolicyMask
	)
{
	DWORD Error = ERROR_SUCCESS;
	PACL Sacl = NULL;
	//PSECURITY_DESCRIPTOR SecDescr = NULL;
	
	if (NewIntegrity <= ObjectIntegrityNone ||
		NewIntegrity >= ObjectIntegrityMax)
	{
		return ERROR_INVALID_PARAMETER;
	}
	
	do
	{
		/*SecDescr = (PSECURITY_DESCRIPTOR)HeapAlloc(gProcHeap, 0, SECURITY_DESCRIPTOR_MIN_LENGTH);
		if (!SecDescr)
		{
			Error = GetLastError();
			break;
		}

		Result = InitializeSecurityDescriptor(SecDescr, SECURITY_DESCRIPTOR_REVISION );
		if (!Result)
		{
			Error = GetLastError();
			break;
		}

		Result = SetSecurityDescriptorSacl(SecDescr, TRUE, Sacl, FALSE);
		if (!Result)
		{
			Error = GetLastError();
			break;
		}*/

		Error = KaPsInitializeMandatoryLabelSacl(&Sacl, NewIntegrity, PolicyMask);
		if (Error != ERROR_SUCCESS)
		{
			break;
		}

		Error = SetNamedSecurityInfoW(ObjectName, ObjectType, LABEL_SECURITY_INFORMATION, NULL, NULL, NULL, Sacl);

	} while (0);

	/*if (SecDescr)
	{
		HeapFree(gProcHeap, 0, SecDescr);
		SecDescr = NULL;
	}*/

	if (Sacl)
	{
		HeapFree(gProcHeap, 0, Sacl);
		Sacl = NULL;
	}

	return Error;
}