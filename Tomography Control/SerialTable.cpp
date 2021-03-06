#include "stdafx.h"

#include "Tomography Control.h"
#include "Tomography ControlDlg.h"
#include "Exceptions.h"

#define BUFFER_SIZE 2000
#define TABLE_MIN_WRITE_INTERVAL_TICKS 200

SerialTable::SerialTable(CWnd* wnd, LPCSTR gszPort) : Table(wnd)
{
	this -> m_inputToSendToTableEvent.ResetEvent();
	
    FillMemory(&this -> m_dcb, sizeof(this -> m_dcb), 0);
    FillMemory(&this -> m_commTimeouts, sizeof(this -> m_commTimeouts), 0);
    this -> m_dcb.DCBlength = sizeof(this -> m_dcb);

    this -> m_hComm = CreateFile( gszPort,  
                    GENERIC_READ | GENERIC_WRITE, 
                    0, 
                    NULL, 
                    OPEN_EXISTING,
                    0,
                    NULL);

    if (this -> m_hComm == INVALID_HANDLE_VALUE) {
		throw bad_serial_port_error("Could not open serial port.");
    }

	// Set up the port details
    if (!BuildCommDCB("baud=19200 parity=N data=8 stop=1", &this -> m_dcb)) {   
        // Couldn't build the DCB. Usually a problem
        // with the communications specification string.
        throw bad_serial_port_error("Could not construct serial port configuration.");
    }
    else
	{
		// DCB is ready for use.
	}

	if (SetCommState(this -> m_hComm, &this -> m_dcb) == 0)
	{
        throw bad_serial_port_error("Could not set port state.");
	}
	
	this -> m_commTimeouts.ReadIntervalTimeout = MAXDWORD;
	this -> m_commTimeouts.ReadTotalTimeoutMultiplier = 0;
	this -> m_commTimeouts.ReadTotalTimeoutConstant = 0;
	this -> m_commTimeouts.WriteTotalTimeoutMultiplier = 0;
	this -> m_commTimeouts.WriteTotalTimeoutConstant = 0;

	SetCommTimeouts(this -> m_hComm, &this -> m_commTimeouts);

	this -> m_lastWriteTicks = GetTickCount();

	this -> Start();
}

SerialTable::~SerialTable() 
{
	this -> Stop();
    CloseHandle(this -> m_hComm);
}

/* Get pending input from the table, and write new input out to it. */
void SerialTable::DoIO()
{
	DoWrite();
	DoRead();
}
	
void SerialTable::DoRead()
{
	DWORD bytesRead = 0;
	char tempBuffer[BUFFER_SIZE];
	
	tempBuffer[0] = NULL;
	
	if (!ReadFile(this -> m_hComm, tempBuffer, BUFFER_SIZE - 1, &bytesRead, NULL))
	{
		DWORD dw = GetLastError();
		char errorBuffer[ERROR_BUFFER_SIZE];

		FormatMessage(FORMAT_MESSAGE_FROM_HMODULE,
			this -> m_hComm,
			dw,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)errorBuffer,
			ERROR_BUFFER_SIZE - 1, NULL);

		MessageBox(NULL, errorBuffer, "Tomography Control", MB_ICONERROR);
		return;
	}

	// Copy the incoming string to the output buffer
	if (bytesRead > 0)
	{
		// Lock the IO buffers while we exchange data with them
		this -> m_bufferLock.Lock();

		tempBuffer[bytesRead] = NULL;
		this -> m_displayBuffer += tempBuffer;
		
		this -> PumpInputReceived();

		this -> m_bufferLock.Unlock();

		this -> m_outputReceivedFromTableEvent.PulseEvent();
	}
}

void SerialTable::DoWrite()
{
	// Lock the IO buffers while we exchange data with them
	this -> m_bufferLock.Lock();
	
	if (!this -> m_sendBuffer.IsEmpty())
	{
		DWORD bytesWritten;
		char linefeedsAtEndOfLine = strlen("\r\n");
		// Only send whole lines
		int lineEnding = this -> m_sendBuffer.Find("\r\n");

		// If we have input to be sent, take a copy then release the locks on the buffers
		if (lineEnding >= 0)
		{
			// Ensure there's a pacing delay between writing to the table, as it does
			// not do its own flow control
			DWORD timeSinceLastWrite = GetTickCount() - this -> m_lastWriteTicks;

			if (timeSinceLastWrite < TABLE_MIN_WRITE_INTERVAL_TICKS)
			{
				Sleep(TABLE_MIN_WRITE_INTERVAL_TICKS - timeSinceLastWrite);
			}

			// Write the contents of the temp buffer out to serial
			WriteFile(this -> m_hComm, this -> m_sendBuffer,
				lineEnding + linefeedsAtEndOfLine, &bytesWritten, NULL);

			// Copy the input to the output buffer as if we'd just read it in
			this -> m_displayBuffer += this -> m_sendBuffer.Left(bytesWritten);

			// Clear the written bytes from the output buffer
			this -> m_sendBuffer.Delete(0, bytesWritten);

			// Notify the window to refresh the display as we've copied
			// the output to the display
			this -> PumpInputReceived();
		}

		if (this -> m_sendBuffer.IsEmpty())
		{
			// Notify the window to refresh the display as we've copied
			// the output to the display
			this -> PumpOutputFinished();
		}
	}

	this -> m_bufferLock.Unlock();

	this -> m_lastWriteTicks = GetTickCount();
}