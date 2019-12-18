#include <stdio.h>
#include <windows.h>
#include <iostream>

void main(int num, char* Buff[])
{
	setlocale(LC_ALL, "Russian");

	char* command;
	if (Buff[1] == NULL)
	{
	//	 Если аргументов нет, то получаем директорию текущей папки. Максимальный размер пути в Виндовс составляет 255 символов.
		char CurrentDir[256];
		GetCurrentDirectory(256, CurrentDir);
		command = new char[strlen(CurrentDir)];
		command[0] = '\0';
		strcat(command, CurrentDir);
	}
	else if(strlen(Buff[1]) < 4 && strstr(Buff[1],"-h"))
	{
	//	 Если пользователь запросил справку
		std::cout << "Это клиентская часть приложения — аналога утилиты dir.\nПрограмма принимает на вход каталог и возвращает список файлов и каталогов внутри него. Пустой путь равносилен указанию текущей директории, где лежит клиентский исполняемый файл. Для работы программы необходим одновременный запуск серверного приложения.\nПрограмма написана Кузнецовым Григорием и Попониным Остапом, КЭ-351.";
		return;
	}
	else
	{
	//	Если в кавычках аргумент, то убираем кавычки
		if (Buff[1][0] == '\"')
		{
			int size = strlen(Buff[1]);
			command = new char[size - 2];
			for (int i = 1; i < size - 1; i++)
				command[i-1] = Buff[1][i];
			command[size-2] = '\0';
		}
		else
		{
			command = new char[strlen(Buff[1])];
			strcpy(command, Buff[1]);
		}
	}

	// Если аргумент без закрывающего символа, то добавляем
	if (command[strlen(command) - 1] != '\\')
	{
		int size = strlen(command);
		char* temp = new char[size + 1];
		strcpy(temp,command);
		temp[size] = '\\';
		temp[size + 1] = '\0';
		command = new char[size + 1];
		strcpy(command, temp);
	}

	// Создаем Пайп
	HANDLE hPipe;
	const int buff_size = 256;

	hPipe = CreateFile(
		TEXT("\\\\.\\pipe\\MyPipe"),	      // имя канала
		GENERIC_READ |  // чтение и запись в канал
		GENERIC_WRITE,
		0,              // нет разделяемых операций
		NULL,           // защита по умолчанию
		CREATE_ALWAYS, // открытие всегда нового канала
		0,              // атрибуты по умолчанию
		NULL);          // нет дополнительных атрибутов

	if (GetLastError() == ERROR_FILE_NOT_FOUND)
	{
		std::cout << "Сервер недоступен. Проверьте работоспособность серверного приложения.";
		return;
	}

	DWORD NumBytesToWrite;
	WriteFile(hPipe, command, strlen(command), &NumBytesToWrite, NULL);

	DWORD iNumBytesToRead;
	bool fSuccess = false;
	do
	{
		char NewBuff[buff_size];
		iNumBytesToRead = buff_size;
		fSuccess	=
			ReadFile(hPipe, NewBuff, iNumBytesToRead, &iNumBytesToRead, NULL);

			int counter = 0;
			for (int i = buff_size - 1; i >= 0; i--)
			{
				if ((NewBuff[i] == '\n') || (NewBuff[i] != NewBuff[i - 1] && NewBuff[i] != NewBuff[i - 2]))
				{
					counter = i;
					break;
				}
			}

			char* response = new char[counter + 2];
			for (int i = 0; i < counter; i++)
				response[i] = NewBuff[i];
			response[counter] = '\0';
			std::cout << response;

			// Проверка на ошибку каталога
			if (strlen(response) == 27 && response[0] == 'E' && response[1] == 'r')
				break;
			memset(NewBuff,'\0', buff_size);

			if (!fSuccess && GetLastError() != ERROR_MORE_DATA) // Если считано все, то заканчивать
				break;

	} while (true);
	CloseHandle(hPipe);
	return;
}
