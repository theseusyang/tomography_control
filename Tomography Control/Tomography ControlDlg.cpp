
// Tomography ControlDlg.cpp : implementation file
//

#include "stdafx.h"

#include <errno.h>
#include <io.h>
#include <windows.h>
#include <string.h>

#include "Tomography Control.h"
#include "Tomography ControlDlg.h"
#include "DummyCamera.h"
#include "PerkinElmerXrd.h"
#include "ShadOCam.h"
#include "Tomography Control.h"
#include "RunProgressDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define ACCESS_MODE_READ_ONLY	4

#define FILE_BUFFER_SIZE		2048

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CTomographyControlDlg dialog




CTomographyControlDlg::CTomographyControlDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CTomographyControlDlg::IDD, pParent)
	, m_tableCommandOutput(_T(""))
	, m_tableCommand(_T(""))
	, m_directoryPath(_T(""))
	, m_exposureTimeSeconds(0.5f)
	, m_framesPerStop(10)
	, m_stopsPerRotation(100)
	, m_numberOfTurns(1)
	, m_delayBetweenTurnsSeconds(1)
	, m_tableInitialisationFile(_T(""))
	, m_manualCameraControl(_T(""))
	, m_cameraName(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTomographyControlDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_TABLE_COMMANDS, m_tableCommandOutput);
	DDX_Control(pDX, IDC_EDIT_TABLE_COMMAND, m_tableCommandControl);
	DDX_Text(pDX, IDC_EDIT_TABLE_COMMAND, m_tableCommand);
	DDX_Text(pDX, IDC_EDIT_EXPOSURE_TIME, m_exposureTimeSeconds);
	DDX_Text(pDX, IDC_EDIT_NUM_FRAMES_STOP, m_framesPerStop);
	DDX_Text(pDX, IDC_EDIT_NUM_STOPS_360, m_stopsPerRotation);
	DDX_Text(pDX, IDC_EDIT_NUM_STOPS_361, m_numberOfTurns);
	DDX_Text(pDX, IDC_EDIT_TURN_INTERVAL, m_delayBetweenTurnsSeconds);
	DDX_Control(pDX, IDC_BUTTON_RUN_LOOP, m_runLoopButton);
	DDX_Text(pDX, IDC_BROWSE_TABLE_INI, m_tableInitialisationFile);
	DDX_Text(pDX, IDC_BROWSE_CAMERA_INI, m_manualCameraControl);
	DDX_CBString(pDX, IDC_COMBO_CAMERA, m_cameraName);
	DDX_Control(pDX, IDC_COMBO_CAMERA, m_cameraComboBox);
	DDX_Text(pDX, IDC_MFCEDITBROWSE1, m_directoryPath);
}

BEGIN_MESSAGE_MAP(CTomographyControlDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_INITIALISE_TABLE, &CTomographyControlDlg::OnBnClickedButtonInitialiseTable)
	ON_BN_CLICKED(IDC_BUTTON_TABLE_NRESET, &CTomographyControlDlg::OnBnClickedButtonTableNreset)
	ON_BN_CLICKED(IDC_BUTTON_TABLE_NCAL, &CTomographyControlDlg::OnBnClickedButtonTableNcal)
	ON_BN_CLICKED(IDC_BUTTON_CLEAR_TABLE_DISPLAY, &CTomographyControlDlg::OnBnClickedButtonClearTableDisplay)
	ON_BN_CLICKED(IDC_BUTTON_RESET_TABLE, &CTomographyControlDlg::OnBnClickedButtonResetTable)
	ON_BN_CLICKED(IDC_BUTTON_RUN_LOOP, &CTomographyControlDlg::OnBnClickedButtonRunLoop)
	ON_BN_CLICKED(IDC_BUTTON_CAMERA_WRITE_INITIAL, &CTomographyControlDlg::OnBnClickedButtonCameraWriteInitial)
	ON_BN_CLICKED(IDC_BUTTON_CAMERA_TAKE_SINGLE, &CTomographyControlDlg::OnBnClickedButtonCameraTakeSingle)
	ON_BN_CLICKED(IDC_BUTTON_CAMERA_TAKE_DARK, &CTomographyControlDlg::OnBnClickedButtonCameraTakeDark)
	ON_BN_CLICKED(IDC_BUTTON_CAMERA_TAKE_FLAT, &CTomographyControlDlg::OnBnClickedButtonCameraTakeFlat)
	ON_MESSAGE(WM_USER_TABLE_MESSAGE_RECEIVED, &CTomographyControlDlg::OnTableMessageReceived)
END_MESSAGE_MAP()


// CTomographyControlDlg message handlers

BOOL CTomographyControlDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CTomographyControlDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CTomographyControlDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CTomographyControlDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// Catch return keypresses on table command field
BOOL CTomographyControlDlg::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN &&
        pMsg->wParam == VK_RETURN) /* &&
        this -> GetFocus() == &this -> m_tableCommandControl) */
    {
		this -> UpdateData(TRUE);

		// Take a copy of the command, then wipe the field
		DWORD commandLen = sizeof(char) * (this -> m_tableCommand.GetLength() + 3);
		char *command = (char*)alloca(commandLen);
		strcpy_s(command, commandLen, (LPCTSTR)m_tableCommand);
		strcat_s(command, commandLen, "\r\n");
		this -> m_tableCommand.Empty();

		this -> m_table -> SendTableCommand(command);

        return TRUE; // this doesn't need processing anymore
    }
    return FALSE; // all other cases still need default processing
}

/**
 * Send the contents of the table initialisation file to the table.
 */
void CTomographyControlDlg::OnBnClickedButtonInitialiseTable()
{
	this -> UpdateData(TRUE);

	if (this -> m_tableInitialisationFile.GetLength() == 0)
	{
		MessageBox("You must specify a table initialisation file.", "Tomography Control", MB_ICONERROR);
		return;
	}

	switch (_access(m_tableInitialisationFile, ACCESS_MODE_READ_ONLY))
	{
	case 0:
		// File exists and we can read it
		break;
	case EACCES:
		MessageBox("You do not have permission to read the table initialisation file.", "Tomography Control", MB_ICONERROR);
		return;
	case ENOENT:
		MessageBox("Table initialisation file does not exist.", "Tomography Control", MB_ICONERROR);
		return;
	default:
		// Leave further diagnosis to opening the file
		break;
	}

	FILE* initialisationFileHandle = fopen(m_tableInitialisationFile, "r");
	if (NULL == initialisationFileHandle)
	{
		MessageBox(strerror(errno), "Tomography Control", MB_ICONERROR);
		return;
	}
	
	// Load file from disk, line by line
	char buffer[FILE_BUFFER_SIZE];
	// Leave two free characters, one for injecting a '\r' if needed, one for the NULL terminator
	char* line = fgets(buffer, FILE_BUFFER_SIZE - 2, initialisationFileHandle);

	while (NULL != line
		&& !feof(initialisationFileHandle))
	{
		// If the line ends with just "\n", change it to "\r\n"
		char *lastChar = line + strlen(line) - 1;

		if (*(lastChar - 1) != '\r')
		{
			*lastChar = '\r';
			*(lastChar + 1) = '\n';
			*(lastChar + 2) = NULL;
		}

		this -> m_table -> SendTableCommand(line);
		line = fgets(buffer, FILE_BUFFER_SIZE - 1, initialisationFileHandle);
	}

	fclose(initialisationFileHandle);
}


void CTomographyControlDlg::OnBnClickedButtonClearTableDisplay()
{
	this -> m_table -> m_bufferLock.Lock();
	this -> m_table -> m_outputBuffer[0] = NULL;
	this -> m_table -> m_bufferLock.Unlock();
	
	UpdateData(TRUE);
	this -> m_tableCommandOutput.Empty();
	UpdateData(FALSE);
}


void CTomographyControlDlg::OnBnClickedButtonResetTable()
{
	// TODO: Add your control notification handler code here
	this -> m_tableCommandOutput.Empty();
	this -> UpdateData(FALSE);
}


void CTomographyControlDlg::OnBnClickedButtonTableNreset()
{
	this -> m_table -> SendTableCommand("nreset\r\n");
}


void CTomographyControlDlg::OnBnClickedButtonTableNcal()
{
	this -> m_table -> SendTableCommand("ncal\r\n");
}

LRESULT CTomographyControlDlg::OnTableMessageReceived(WPARAM wParam, LPARAM tablePtr)
{
	Table* table = (Table*)tablePtr;

	UpdateData(TRUE);
	table -> m_bufferLock.Lock();
	this -> m_tableCommandOutput += table -> m_outputBuffer.data();
	table -> m_outputBuffer.clear();
	table -> m_bufferLock.Unlock();
	UpdateData(FALSE);

	return TRUE;
}

/* Get a camera object, depending on the currently selected
 * type.
 * Throws a string with an error message in case of a problem.
 */
ICamera* CTomographyControlDlg::BuildSelectedCamera()
{
	ICamera *camera;

	if (strcmp(this -> m_cameraName, "Dummy") == 0)
	{
		camera = new DummyCamera();
	}
	else if (strcmp(this -> m_cameraName, "Shad-o-cam") == 0)
	{
		camera = new ShadOCam("C:\\ShadoCam\\IniFile.txt");
	}
	else if (strcmp(this -> m_cameraName, "Perkin-Elmer XRD") == 0)
	{
		camera = new PerkinElmerXrd();
	}
	else
	{
		throw "Unrecognised camera type.";
	}

	camera -> SetupCamera(this -> m_exposureTimeSeconds);
}

void CTomographyControlDlg::OnBnClickedButtonRunLoop()
{
	// TODO: Validate inputs

	// TODO: Calculate end time

	this -> UpdateData(TRUE);

	RunTask task;
	CRunProgressDlg runProgressDlg;
	
	try {
		task.m_camera = BuildSelectedCamera();
	}
	catch(char* message)
	{
		MessageBox(message, "Tomography Control", MB_ICONERROR);
		return;
	}

	task.m_dialog = &runProgressDlg;
	task.m_table = this -> m_table;
	task.m_directoryPath = this -> m_directoryPath;
	task.m_turnsTotal = this -> m_numberOfTurns;
	task.m_stopsPerTurn = this -> m_stopsPerRotation;
	task.m_framesPerStop = this -> m_framesPerStop;
	task.m_exposureTimeSeconds = this -> m_exposureTimeSeconds;
	task.m_running = TRUE;
	
	runProgressDlg.m_exposureTimeSeconds = this -> m_exposureTimeSeconds;
	runProgressDlg.m_framesPerStop = this -> m_framesPerStop;
	runProgressDlg.m_turnsTotal = this -> m_numberOfTurns;
	runProgressDlg.m_stopsPerRotation = this -> m_stopsPerRotation;

	CWinThread* workerThread = AfxBeginThread(captureRunFrames, &task, THREAD_PRIORITY_NORMAL, 
		0, CREATE_SUSPENDED);
	workerThread -> m_bAutoDelete = FALSE;
	workerThread -> ResumeThread();

	runProgressDlg.DoModal();

	// TODO: The dialog itself should set this, so the dialog is still
	// valid at every point where this value is true
	task.m_running = FALSE;

	delete task.m_camera;

	// Give the thread 60 seconds to exit
	::WaitForSingleObject(workerThread -> m_hThread, 60000);
	delete workerThread;
}


void CTomographyControlDlg::OnBnClickedButtonCameraWriteInitial()
{
	// TODO: Add your control notification handler code here
}

/* The following methods deal with manually taking images from the camera, on
 * request.
 */
void CTomographyControlDlg::OnBnClickedButtonCameraTakeSingle()
{
	this -> UpdateData(TRUE);

	CameraTask task(CameraTask::SINGLE);

	try {
		task.m_camera = BuildSelectedCamera();
	}
	catch(char* message)
	{
		MessageBox(message, "Tomography Control", MB_ICONERROR);
		return;
	}

	this -> RunManualImageTask(&task);
	delete task.m_camera;
}

void CTomographyControlDlg::OnBnClickedButtonCameraTakeDark()
{
	this -> UpdateData(TRUE);

	CameraTask task(CameraTask::DARK);

	try {
		task.m_camera = BuildSelectedCamera();
	}
	catch(char* message)
	{
		MessageBox(message, "Tomography Control", MB_ICONERROR);
		return;
	}
	this -> RunManualImageTask(&task);
	delete task.m_camera;
}

void CTomographyControlDlg::OnBnClickedButtonCameraTakeFlat()
{
	this -> UpdateData(TRUE);

	CameraTask task(CameraTask::FLAT_FIELD);

	try {
		task.m_camera = BuildSelectedCamera();
	}
	catch(char* message)
	{
		MessageBox(message, "Tomography Control", MB_ICONERROR);
		return;
	}
	this -> RunManualImageTask(&task);
	delete task.m_camera;
}

void CTomographyControlDlg::RunManualImageTask(CameraTask* task)
{
	CTakingPhotosDlg takingPhotosDlg;

	task -> m_dialog = &takingPhotosDlg;

	CWinThread* workerThread = AfxBeginThread(takeManualImages, task, THREAD_PRIORITY_NORMAL, 
		0, CREATE_SUSPENDED);
	workerThread -> m_bAutoDelete = FALSE;
	workerThread -> ResumeThread();

	takingPhotosDlg.DoModal();

	task -> m_running = false;
	// Give the thread 5 seconds to exit
	::WaitForSingleObject(workerThread -> m_hThread, 5000);
	delete workerThread;
}

// Worker function for taking a set of manual images from an X-ray camera
UINT takeManualImages( LPVOID pParam )
{
	char filenameBuffer[FILENAME_BUFFER_SIZE];
	CameraTask* task = (CameraTask*)pParam;
	
	for (short i = 0; i < task -> m_totalImages; i++) 
	{
		// TODO Handle dark images
		switch (task -> m_taskType)
		{
		case CameraTask::DARK:
			sprintf_s(filenameBuffer, FILENAME_BUFFER_SIZE, "%s\\DC%04d.raw", task -> m_directoryPath, (i + 1));
			task -> m_camera -> CaptureDarkImage(filenameBuffer);
			break;
		case CameraTask::FLAT_FIELD:
			sprintf_s(filenameBuffer, FILENAME_BUFFER_SIZE, "%s\\FF%04d.raw", task -> m_directoryPath, (i + 1));
			task -> m_camera -> CaptureFlatField(filenameBuffer);
			break;
		default:
			sprintf_s(filenameBuffer, FILENAME_BUFFER_SIZE, "%s\\IMAGE%04d.raw", task -> m_directoryPath, (i + 1));
			task -> m_camera -> CaptureFrame(filenameBuffer);
			break;
		}
		if(!(task -> m_running))
		{
			break;
		}

		if (task -> m_dialog -> m_IsInit)
		{
			task -> m_dialog -> m_progress.SetRange(0, task -> m_totalImages - 1);
			task -> m_dialog -> m_progress.SetPos(i + 1);
		}
	}

	if (task -> m_running)
	{
		task -> m_dialog -> PostMessage(WM_CLOSE);
	}

	return 0;   // thread completed successfully
}
