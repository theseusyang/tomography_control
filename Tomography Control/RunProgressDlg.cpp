// RunProgressDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Tomography Control.h"
#include "RunProgressDlg.h"
#include "afxdialogex.h"


// CRunProgressDlg dialog

IMPLEMENT_DYNAMIC(CRunProgressDlg, CDialogEx)

CRunProgressDlg::CRunProgressDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CRunProgressDlg::IDD, pParent)
	, m_currentPosition(0)
	, m_imageFilename(_T(""))
	, m_stopsMade(_T(0))
	, m_stopsPerRotation(0)
	, m_turnsMade(0)
	, m_turnsTotal(0)
	, m_calculatedAngle(_T(""))
	, m_startTime(_T(""))
	, m_estEndTime(_T(""))
	, m_estRunTime(_T(""))
{

}

CRunProgressDlg::~CRunProgressDlg()
{
}

void CRunProgressDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_DISPLAY_CURRENT_POSITION, m_currentPosition);
	DDX_Text(pDX, IDC_DISPLAY_PROCEEDED_IMAGE, m_imageFilename);
	DDX_Text(pDX, IDC_DISPLAY_STOPS_MADE, m_stopsMade);
	DDX_Text(pDX, IDC_DISPLAY_STOPS_TOTAL, m_stopsPerRotation);
	DDX_Text(pDX, IDC_DISPLAY_TURNS_MADE, m_turnsMade);
	DDX_Text(pDX, IDC_DISPLAY_TURNS_TOTAL, m_turnsTotal);
	DDX_Text(pDX, IDC_DISPLAY_CALC_ANGLE, m_calculatedAngle);
	DDX_Text(pDX, IDC_DISPLAY_START_TIME, m_startTime);
	DDX_Text(pDX, IDC_DISPLAY_EST_END_TIME, m_estEndTime);
	DDX_Text(pDX, IDC_DISPLAY_EST_RUN_TIME, m_estRunTime);
}

BOOL CRunProgressDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	
	CString startTime = CTime::GetCurrentTime().Format("%H:%M:%S");
	this -> m_startTime.Append(startTime);

	return TRUE;  // return TRUE  unless you set the focus to a control
}


BEGIN_MESSAGE_MAP(CRunProgressDlg, CDialogEx)
	ON_MESSAGE(WM_USER_TURN_COMPLETED, &CRunProgressDlg::OnTurnCompleted)
	ON_MESSAGE(WM_USER_TABLE_ANGLE_CHANGED, &CRunProgressDlg::OnTableAngleChanged)
	ON_MESSAGE(WM_USER_STOP_COMPLETED, &CRunProgressDlg::OnStopCompleted)
	ON_MESSAGE(WM_USER_IMAGE_CAPTURED, &CRunProgressDlg::OnImageCaptured)
END_MESSAGE_MAP()


// CRunProgressDlg message handlers


afx_msg LRESULT CRunProgressDlg::OnTurnCompleted(WPARAM wParam, LPARAM lParam)
{
	int* turnCount = (int*)lParam;

	this -> m_turnsMade = *turnCount;
	this -> UpdateData(FALSE);

	return TRUE;
}

afx_msg LRESULT CRunProgressDlg::OnTableAngleChanged(WPARAM wParam, LPARAM lParam)
{
	float* angle = (float*)lParam;

	this -> m_calculatedAngle.Format("%.2f", angle);
	this -> UpdateData(FALSE);

	return TRUE;
}

afx_msg LRESULT CRunProgressDlg::OnStopCompleted(WPARAM wParam, LPARAM lParam)
{
	int* stopCount = (int*)lParam;

	this -> m_stopsMade = *stopCount;
	this -> UpdateData(FALSE);

	return TRUE;
}

afx_msg LRESULT CRunProgressDlg::OnImageCaptured(WPARAM wParam, LPARAM lParam)
{
	return TRUE;
}


// Worker functions
UINT takeRunImages( LPVOID pParam )
{
	RunTask* task = (RunTask*)pParam;
	CWnd* dialog = (CRunProgressDlg*)task -> m_dialog;

	const float tableResolution = 0.0005f; // Degrees
	int stepsPerStop = (360.0f / task -> m_stopsPerTurn) / tableResolution;
	
	for (int turnCount = 0; turnCount < task -> m_turnsTotal; turnCount++)
	{
		for (int stopCount = 0; stopCount < task -> m_stopsPerTurn; stopCount++)
		{
			float calculatedAngle = stopCount * stepsPerStop * tableResolution;

			// TODO: Turn table here
			if (::IsWindow(dialog -> m_hWnd))
			{
				dialog -> PostMessage(WM_USER_TABLE_ANGLE_CHANGED, 0, (LPARAM)&calculatedAngle);
			}

			for (int frameCount = 0; frameCount < task -> m_framesPerStop; frameCount++)
			{
				Sleep(100); // task -> m_exposureTimeSeconds * 1000);

				if (task -> m_running)
				{
					if (::IsWindow(dialog -> m_hWnd))
					{
						// Should input filename here, too
						dialog -> PostMessage(WM_USER_IMAGE_CAPTURED, 0, (LPARAM)&frameCount);
					}
				}
				else
				{
					break;
				}
			}

			if (task -> m_running)
			{
				if (::IsWindow(dialog -> m_hWnd))
				{
					dialog -> PostMessage(WM_USER_STOP_COMPLETED, 0, (LPARAM)&stopCount);
				}
			}
			else
			{
				break;
			}

		}

		if (task -> m_running)
		{
			if (::IsWindow(dialog -> m_hWnd))
			{
				dialog -> PostMessage(WM_USER_TURN_COMPLETED, 0, (LPARAM)&turnCount);
			}
		}
		else
		{
			break;
		}
	}

	if (task -> m_running == TRUE
		&& ::IsWindow(dialog -> m_hWnd))
	{
		dialog -> PostMessage(WM_CLOSE);
	}

	return 0;   // thread completed successfully
}
