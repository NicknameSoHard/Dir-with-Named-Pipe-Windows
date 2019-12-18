#include <stdio.h>
#include <windows.h>
#include <iostream>

void main(int num, char* Buff[])
{
	setlocale(LC_ALL, "Russian");
	if (Buff[1] != NULL)
	{
		if (strlen(Buff[1]) < 4 && strstr(Buff[1], "-h"))
			std::cout << "Это серверная часть приложения — аналога утилиты dir. После запуска сервер переходит в бесконечное ожидание запроса от клиента. Выключить сервер можно, если с клиента будет отправлен сигнал Q или q.\nЗапрос осуществляется через именованный пайп и содержит в себе директорию. Сервер отвечает содержимым этой директории.\nПрограмма написана Кузнецовым Григорием и Попониным Остапом, КЭ-351.\n\0";
	}

	const int buff_size = 256;
	char PipeBuff[buff_size];

	PROCESS_INFORMATION piProcInfo;
	STARTUPINFO SI;

	HANDLE hPipe;
	DWORD NumBytesToWrite;
	DWORD iNumBytesToRead;

	ZeroMemory(&SI, sizeof(STARTUPINFO));
	SI.cb = sizeof(STARTUPINFO);
	ZeroMemory(&piProcInfo, sizeof(piProcInfo));

	std::cout << "Start server.\n";

	bool fSuccess = false;
	while (true)
	{
		hPipe = CreateNamedPipe(
			TEXT("\\\\.\\pipe\\MyPipe"),       // имя канала
			PIPE_ACCESS_DUPLEX,       // чтение и запись из канала
			PIPE_TYPE_MESSAGE |       // передача сообщений по каналу
			PIPE_READMODE_MESSAGE |   // режим чтения сообщений
			PIPE_WAIT,                // синхронная передача сообщений
			PIPE_UNLIMITED_INSTANCES, // число экземпляров канала
			4096,			   // размер выходного буфера
			4096,			   // размер входного буфера
			0,			   // тайм-аут клиента
			NULL);             // защита по умолчанию

		if (hPipe == INVALID_HANDLE_VALUE)
		{
			std::cout << "CreatePipe failed: error code %d\n" << (int)GetLastError();
			return;
		}

		if ((ConnectNamedPipe(hPipe, NULL)) == 0)
		{
			std::cout << "Client could not connect\n";
			return;
		}

		iNumBytesToRead = buff_size;
		ReadFile(hPipe, PipeBuff, iNumBytesToRead, &iNumBytesToRead, NULL);

		int counter = 0;
		int size = sizeof(PipeBuff) - 1;
		if (PipeBuff[size] == PipeBuff[size - 1])
		{
			for (int i = size; i > -1; i--)
				if (PipeBuff[i] != PipeBuff[size])
					counter++;
		}
		else
			counter = strlen(PipeBuff);

		char* command = new char[counter + 2];
		for (int i = 0; i < counter; i++)
			command[i] = PipeBuff[i];
		command[counter] = '*';
		command[counter + 1] = '\0';

		if (command[counter - 1] == '\"')
			command[counter - 1] = '\\';
		memset(PipeBuff, '\0', buff_size);

		// Проверяем не поступило ли комманд
		if (strlen(command) < 4 && (command[0] == 'q' || command[0] == 'Q'))
		{
			WriteFile(hPipe, "Work end.", 28, &NumBytesToWrite, NULL);
			break;
		}

		WIN32_FIND_DATA FindFileData;
		HANDLE hf = FindFirstFile(command, &FindFileData);
		if (hf != INVALID_HANDLE_VALUE)
		{
			do {
				// Получаем имя
				char* temp = NULL;
				char* FileInfo = new char[strlen(FindFileData.cFileName)];
				strcpy(FileInfo, FindFileData.cFileName);

				unsigned __int64 iFileLen;
				char * FileLen;
				if (( (strlen(FileInfo) != 2) || (strlen(FileInfo) != 3) ) && FileInfo[0] != '.')
				{
					// Проверяем папка ли
					if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
					{
						FileLen = new char[8];
						FileLen[0] = '\0';
						FileLen = strcat(FileLen, "Folder");
					}
					else
					{
						// Получаем данные о размере, если файл
						iFileLen = ((FindFileData.nFileSizeHigh * ((unsigned __int64)MAXDWORD + 1)) + FindFileData.nFileSizeLow) / 1024; // Размер в Мегабайтах
						FileLen = new char[sizeof(iFileLen) + 11];
						itoa(iFileLen, FileLen, 10);
						strcat(FileLen, " kilobyte");
					}

					// Склеиваем название и размер
					temp = new char[strlen(FileInfo) + 4];
					strcpy(temp, FileInfo);
					strcat(temp, " | ");
					FileInfo = new char[strlen(temp)];
					strcpy(FileInfo, temp);

					temp = new char[strlen(FileInfo)];
					strcpy(temp, FileInfo);
					FileInfo = new char[strlen(temp) + strlen(FileLen)];
					strcpy(FileInfo, temp);
					strcat(FileInfo, FileLen);

					// Получаем время последнего изменения
					FILETIME ft = FindFileData.ftLastWriteTime;
					SYSTEMTIME stime;
					FileTimeToSystemTime(&ft, &stime);

					char FileTime[14];
					sprintf(FileTime, " | %u %u %u\0", stime.wYear, stime.wMonth, stime.wDay);

					// Склеиваем все куски
					temp = new char[strlen(FileInfo)];
					strcpy(temp, FileInfo);

					FileInfo = new char[strlen(temp) + 13 + 1];
					strcpy(FileInfo, temp);

					strcat(FileInfo, FileTime);
					strcat(FileInfo, "\n");

					WriteFile(hPipe, FileInfo, strlen(FileInfo), &NumBytesToWrite, NULL);
				}

			} while (FindNextFile(hf, &FindFileData) != 0);
			FindClose(hf);
			CloseHandle(hPipe);
		}
		else
			WriteFile(hPipe, "Error. Incorrect Directory\n", 27, &NumBytesToWrite, NULL);


		delete[] command;
		std::cout << "Responce done.\n";
		if (GetLastError() == ERROR_BROKEN_PIPE)
		{
			DisconnectNamedPipe(hPipe);
			CloseHandle(hPipe);
		}
	}
}
