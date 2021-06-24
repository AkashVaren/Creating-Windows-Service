#include <Windows.h>
#include <tchar.h>
#include<iostream>
using namespace std;
/*
    A Main Entry point
    A Service Entry point
    A Service Control Handler
*/

SERVICE_STATUS        g_ServiceStatus = { 0 }; //Service Status Handle for Register the Service
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;// Event Handle for Service
HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv);// Service Control Handler
VOID WINAPI ServiceCtrlHandler(DWORD);
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);

#define SERVICE_NAME  _T("My Sample Service")


int _tmain(int argc, TCHAR* argv[])
{
    OutputDebugString(_T("My Sample Service: Main: Entry"));

    //-->Reading the registry key value
    HKEY hKey = NULL;
    DWORD cType = REG_SZ;
    TCHAR lpData[1024] = { 0 };
    DWORD size = sizeof(lpData);

    LONG lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        _T("SOFTWARE\\Test\\Product\\NewOne"),
        NULL, KEY_QUERY_VALUE, &hKey);
    if (ERROR_SUCCESS != lRet)
    {
        std::cout << _T("RegOpenKeyEx failed. Error: ") << lRet << std::endl;
    }

    lRet = RegQueryValueEx(hKey, _T("Windows Service Created"), NULL, &cType, (LPBYTE)lpData, &size);
    RegCloseKey(hKey);

    if (ERROR_SUCCESS != lRet)
    {
        std::cout << _T("RegQueryValueEx failed. Error: ") << lRet << std::endl;;
    }

    if (REG_SZ != cType)
    {
        std::cout << _T("Value has not expected type REG_SZ") << std::endl;
   
    }

    std::cout << "Data: " << lpData << std::endl;

    //-->End of registry read function
    //This will have the structure for each service that it can start

   


    SERVICE_TABLE_ENTRY ServiceTable[] =
    {
        {(LPWSTR)SERVICE_NAME,(LPSERVICE_MAIN_FUNCTION)ServiceMain},
        {NULL, NULL}
    };

    if (StartServiceCtrlDispatcher(ServiceTable) == FALSE)
    {
        OutputDebugString(_T("My Sample Service: Main: StartServiceCtrlDispatcher returned error"));
        return GetLastError();
    }

    OutputDebugString(_T("My Sample Service: Main: Exit"));

    return 0;
}


VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv)
{
    DWORD Status = E_FAIL;

    OutputDebugString(_T("My Sample Service: ServiceMain: Entry"));

    g_StatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);

    // Start the thread that will perform the main task of the service
    HANDLE hThread;

    if (g_StatusHandle == NULL)
    {
        OutputDebugString(_T("My Sample Service: ServiceMain: RegisterServiceCtrlHandler returned error"));
        goto EXIT;
    }

    // Tell the service controller we are starting
    ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
    {
        OutputDebugString(_T("My Sample Service: ServiceMain: SetServiceStatus returned error"));
    }

    /*
     * Perform tasks neccesary to start the service here
     */
    OutputDebugString(_T("My Sample Service: ServiceMain: Performing Service Start Operations"));

    // Create stop event to wait on later.
    g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);



    if (g_ServiceStopEvent == NULL)
    {
        OutputDebugString(_T("My Sample Service: ServiceMain: CreateEvent(g_ServiceStopEvent) returned error"));

        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        g_ServiceStatus.dwWin32ExitCode = GetLastError();
        g_ServiceStatus.dwCheckPoint = 1;

        if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
        {
            OutputDebugString(_T("My Sample Service: ServiceMain: SetServiceStatus returned error"));
        }
        goto EXIT;
    }

    // Tell the service controller we are started
    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
    {
        OutputDebugString(_T("My Sample Service: ServiceMain: SetServiceStatus returned error"));
    }
    // Start the thread that will perform the main task of the service
    hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);

    OutputDebugString(_T("My Sample Service: ServiceMain: Waiting for Worker Thread to complete"));

    // Wait until our worker thread exits effectively signaling that the service needs to stop
    WaitForSingleObject(hThread, INFINITE);

    OutputDebugString(_T("My Sample Service: ServiceMain: Worker Thread Stop Event signaled"));


    /*
     * Perform any cleanup tasks
     */
    OutputDebugString(_T("My Sample Service: ServiceMain: Performing Cleanup Operations"));

    CloseHandle(g_ServiceStopEvent);

    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 3;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
    {
        OutputDebugString(_T("My Sample Service: ServiceMain: SetServiceStatus returned error"));
    }

EXIT:
    OutputDebugString(_T("My Sample Service: ServiceMain: Exit"));

    return;
}


VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode)
{
    OutputDebugString(_T("My Sample Service: ServiceCtrlHandler: Entry"));

    switch (CtrlCode)
    {
    case SERVICE_CONTROL_STOP:

        OutputDebugString(_T("My Sample Service: ServiceCtrlHandler: SERVICE_CONTROL_STOP Request"));

        if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
            break;

        /*
         * Perform tasks neccesary to stop the service here
         */

        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        g_ServiceStatus.dwWin32ExitCode = 0;
        g_ServiceStatus.dwCheckPoint = 4;

        if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
        {
            OutputDebugString(_T("My Sample Service: ServiceCtrlHandler: SetServiceStatus returned error"));
        }

        // This will signal the worker thread to start shutting down
        SetEvent(g_ServiceStopEvent);

        break;
    case SERVICE_CONTROL_PAUSE:
        /* Pause service. */
        break;
    case SERVICE_CONTROL_CONTINUE:
        /* Continue from Paused state. */
        break;
    default:
        break;
    }

    OutputDebugString(_T("My Sample Service: ServiceCtrlHandler: Exit"));
}


DWORD WINAPI ServiceWorkerThread(LPVOID lpParam)
{
    OutputDebugString(_T("My Sample Service: ServiceWorkerThread: Entry"));

    //  Periodically check if the service has been requested to stop
    while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)
    {
        /*
         * Perform main service function here
         */
        /*
         //-->Executing a new exe file
        
        TCHAR* path = (TCHAR*)L"C:\\Users\\akash\\source\\repos\\RegistryMonitoring\\Debug\\RegistryMonitoring.exe";

        STARTUPINFO info;
        PROCESS_INFORMATION processInfo;

        ZeroMemory(&info, sizeof(info));
        info.cb = sizeof(info);
        ZeroMemory(&processInfo, sizeof(processInfo));


        if (CreateProcess(path, NULL, NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo))
        {
            ::WaitForSingleObject(processInfo.hProcess, INFINITE);
            CloseHandle(processInfo.hProcess);
            CloseHandle(processInfo.hThread);
        }

        //-->Ending of reading exe file
        */
        //ShellExecute(NULL, L"open", L"C:\\Users\\akash\\source\\repos\\RegistryMonitoring\\Debug\\RegistryMonitoring.exe", NULL, NULL, SW_SHOWDEFAULT);
         //  Simulate some work by sleeping
        Sleep(3000);
    }

    OutputDebugString(_T("My Sample Service: ServiceWorkerThread: Exit"));

    return ERROR_SUCCESS;
}