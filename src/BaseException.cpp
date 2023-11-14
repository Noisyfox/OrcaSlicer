
#include <backward.hpp>
#include "BaseException.h"
#include <iomanip>
#include <string>
#include <sstream>
#include <iostream>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/log/trivial.hpp>
#include <boost/format.hpp>
#include <mutex>
#include <tchar.h>


static std::string g_log_folder;
static std::atomic<int> g_crash_log_count = 0;
static std::mutex g_dump_mutex;

CBaseException::CBaseException(HANDLE hProcess, WORD wPID, LPCTSTR lpSymbolPath, PEXCEPTION_POINTERS pEp)
{
	if (NULL != pEp)
	{
		m_pEp = new EXCEPTION_POINTERS;
		CopyMemory(m_pEp, pEp, sizeof(EXCEPTION_POINTERS));
	}
	output_file = new boost::nowide::ofstream();
	std::time_t t = std::time(0);
	std::tm* now_time = std::localtime(&t);
	std::stringstream buf;

	if (!g_log_folder.empty()) {
		buf << std::put_time(now_time, "crash_%a_%b_%d_%H_%M_%S_") <<g_crash_log_count++ <<".log";
		auto log_folder = (boost::filesystem::path(g_log_folder) / "log").make_preferred();
		if (!boost::filesystem::exists(log_folder)) {
		    boost::filesystem::create_directory(log_folder);
	    }
		auto crash_log_path = boost::filesystem::path(log_folder / buf.str()).make_preferred();
		std::string log_filename = crash_log_path.string();
		output_file->open(log_filename, std::ios::out | std::ios::app);
	}
}

CBaseException::~CBaseException(void)
{
	if (output_file) {
		output_file->close();
		delete output_file;
	}
}


//BBS set crash log folder
void CBaseException::set_log_folder(std::string log_folder)
{
	g_log_folder = log_folder;
}

// Convert a wide Unicode string to an UTF8 string
std::string utf8_encode(const std::wstring &wstr, UINT codePage = CP_ACP)
{
    if( wstr.empty() ) return std::string();
    int size_needed = WideCharToMultiByte(codePage, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo( size_needed, 0 );
    WideCharToMultiByte(codePage, 0, &wstr[0], (int) wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

void CBaseException::OutputString(LPCTSTR lpszFormat, ...)
{
	TCHAR szBuf[2048] = _T("");
	va_list args;
	va_start(args, lpszFormat);
	_vsntprintf_s(szBuf, 2048, lpszFormat, args);
	va_end(args);

	//WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), szBuf, _tcslen(szBuf), NULL, NULL);

	//output it to the current directory of binary
	std::string output_str = utf8_encode(szBuf);
	*output_file << output_str;
	output_file->flush();
}

void CBaseException::ShowCallstack(HANDLE hThread, CONTEXT* context)
{
	OutputString(_T("Show CallStack:\r\n"));

    backward::Printer printer;

    backward::StackTrace st;
    st.set_machine_type(printer.resolver().machine_type());
    st.set_thread_handle(hThread);
    st.load_here(32, context);
    st.skip_n_firsts(0);

    printer.address = true;
    printer.print(st, *output_file);
}

void CBaseException::ShowExceptionResoult(DWORD dwExceptionCode)
{
	OutputString(_T("Exception Code :%08x "), dwExceptionCode);
// BBS: to be checked
#if 1
	switch (dwExceptionCode)
	{
	case EXCEPTION_ACCESS_VIOLATION:
		{
			//OutputString(_T("ACCESS_VIOLATION(%s)\r\n"), _T("��д�Ƿ��ڴ�"));
			OutputString(_T("ACCESS_VIOLATION\r\n"));
		}
		return ;
	case EXCEPTION_DATATYPE_MISALIGNMENT:
		{
			//OutputString(_T("DATATYPE_MISALIGNMENT(%s)\r\n"), _T("�߳���ͼ�ڲ�֧�ֶ����Ӳ���϶�дδ���������"));
			OutputString(_T("DATATYPE_MISALIGNMENT\r\n"));
		}
		return ;
	case EXCEPTION_BREAKPOINT:
		{
			//OutputString(_T("BREAKPOINT(%s)\r\n"), _T("����һ���ϵ�"));
			OutputString(_T("BREAKPOINT\r\n"));
		}
		return ;
	case EXCEPTION_SINGLE_STEP:
		{
			//OutputString(_T("SINGLE_STEP(%s)\r\n"), _T("����")); //һ���Ƿ����ڵ����¼���
			OutputString(_T("SINGLE_STEP\r\n"));
		}
		return ;
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
		{
			//OutputString(_T("ARRAY_BOUNDS_EXCEEDED(%s)\r\n"), _T("�������Խ��"));
			OutputString(_T("ARRAY_BOUNDS_EXCEEDED\r\n"));
		}
		return ;
	case EXCEPTION_FLT_DENORMAL_OPERAND:
		{
			//OutputString(_T("FLT_DENORMAL_OPERAND(%s)\r\n"), _T("���������һ�������������棬�����ĸ������޷���ʾ")); //������������
			OutputString(_T("FLT_DENORMAL_OPERAND\r\n"));
		}
		return ;
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
		{
			//OutputString(_T("FLT_DIVIDE_BY_ZERO(%s)\r\n"), _T("��������0����"));
			OutputString(_T("FLT_DIVIDE_BY_ZERO\r\n"));
		}
		return ;
	case EXCEPTION_FLT_INEXACT_RESULT:
		{
			//OutputString(_T("FLT_INEXACT_RESULT(%s)\r\n"), _T("�����������Ľ���޷���ʾ")); //�޷���ʾһ��������̫С��������������ʾ�ķ�Χ, ����֮������Ľ���쳣
			OutputString(_T("FLT_INEXACT_RESULT\r\n"));
		}
		return ;
	case EXCEPTION_FLT_INVALID_OPERATION:
		{
			//OutputString(_T("FLT_INVALID_OPERATION(%s)\r\n"), _T("�����������쳣"));
			OutputString(_T("FLT_INVALID_OPERATION\r\n"));
		}
		return ;
	case EXCEPTION_FLT_OVERFLOW:
		{
			//OutputString(_T("FLT_OVERFLOW(%s)\r\n"), _T("���������ָ����������Ӧ���͵����ֵ"));
			OutputString(_T("FLT_OVERFLOW\r\n"));
		}
		return ;
	case EXCEPTION_FLT_STACK_CHECK:
		{
			//OutputString(_T("STACK_CHECK(%s)\r\n"), _T("ջԽ�����ջ�������"));
			OutputString(_T("STACK_CHECK\r\n"));
		}
		return ;
	case EXCEPTION_INT_DIVIDE_BY_ZERO:
		{
			//OutputString(_T("INT_DIVIDE_BY_ZERO(%s)\r\n"), _T("������0�쳣"));
			OutputString(_T("INT_DIVIDE_BY_ZERO\r\n"));
		}
		return ;
	case EXCEPTION_INVALID_HANDLE:
		{
			//OutputString(_T("INVALID_HANDLE(%s)\r\n"), _T("�����Ч"));
			OutputString(_T("INVALID_HANDLE\r\n"));
		}
		return ;
	case EXCEPTION_PRIV_INSTRUCTION:
		{
			//OutputString(_T("PRIV_INSTRUCTION(%s)\r\n"), _T("�߳���ͼִ�е�ǰ����ģʽ��֧�ֵ�ָ��"));
			OutputString(_T("PRIV_INSTRUCTION\r\n"));
		}
		return ;
	case EXCEPTION_IN_PAGE_ERROR:
		{
			//OutputString(_T("IN_PAGE_ERROR(%s)\r\n"), _T("�߳���ͼ����δ���ص������ڴ�ҳ���߲��ܼ��ص������ڴ�ҳ"));
			OutputString(_T("IN_PAGE_ERROR\r\n"));
		}
		return ;
	case EXCEPTION_ILLEGAL_INSTRUCTION:
		{
			//OutputString(_T("ILLEGAL_INSTRUCTION(%s)\r\n"), _T("�߳���ͼִ����Чָ��"));
			OutputString(_T("ILLEGAL_INSTRUCTION\r\n"));
		}
		return ;
	case EXCEPTION_NONCONTINUABLE_EXCEPTION:
		{
			//OutputString(_T("NONCONTINUABLE_EXCEPTION(%s)\r\n"), _T("�߳���ͼ��һ�����ɼ���ִ�е��쳣���������ִ��"));
			OutputString(_T("NONCONTINUABLE_EXCEPTION\r\n"));
		}
		return ;
	case EXCEPTION_STACK_OVERFLOW:
		{
			//OutputString(_T("STACK_OVERFLOW(%s)\r\n"), _T("ջ���"));
			OutputString(_T("STACK_OVERFLOW\r\n"));
		}
		return ;
	case EXCEPTION_INVALID_DISPOSITION:
		{
			//OutputString(_T("INVALID_DISPOSITION(%s)\r\n"), _T("�쳣���������쳣������������һ����Ч����")); //ʹ�ø߼����Ա�д�ĳ�����Զ������������쳣
			OutputString(_T("INVALID_DISPOSITION\r\n"));
		}
		return ;
	case EXCEPTION_FLT_UNDERFLOW:
		{
			//OutputString(_T("FLT_UNDERFLOW(%s)\r\n"), _T("������������ָ��С����Ӧ���͵���Сֵ"));
			OutputString(_T("FLT_UNDERFLOW\r\n"));
		}
		return ;
	case EXCEPTION_INT_OVERFLOW:
		{
			//OutputString(_T("INT_OVERFLOW(%s)\r\n"), _T("��������Խ��"));
			OutputString(_T("INT_OVERFLOW\r\n"));
		}
		return ;
	}

	TCHAR szBuffer[512] = { 0 };

	FormatMessage(  FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE,
		GetModuleHandle( _T("NTDLL.DLL") ),
		dwExceptionCode, 0, szBuffer, sizeof( szBuffer ), 0 );

	OutputString(_T("%s"), szBuffer);
	OutputString(_T("\r\n"));
#endif
}

LONG WINAPI CBaseException::UnhandledExceptionFilter(PEXCEPTION_POINTERS pExceptionInfo )
{
	if (pExceptionInfo->ExceptionRecord->ExceptionCode < 0x80000000
		//BBS: Load project on computers with SDC may trigger this exception (in ShowModal()),
		//     It's not fatal and should be ignored, or there will be lots of meaningless crash logs
		|| pExceptionInfo->ExceptionRecord->ExceptionCode==0xe0434352)
		//BBS: ignore the exception when copy preset
		//|| pExceptionInfo->ExceptionRecord->ExceptionCode==0xe06d7363)
	{
		//BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << boost::format(": got an ExceptionCode %1%, skip it!") % pExceptionInfo->ExceptionRecord->ExceptionCode;
		return EXCEPTION_CONTINUE_SEARCH;
	}
    g_dump_mutex.lock();
	CBaseException base(GetCurrentProcess(), GetCurrentProcessId(), NULL, pExceptionInfo);
	base.ShowExceptionInformation();
    g_dump_mutex.unlock();

	return EXCEPTION_CONTINUE_SEARCH;
}

LONG WINAPI CBaseException::UnhandledExceptionFilter2(PEXCEPTION_POINTERS pExceptionInfo )
{
	CBaseException base(GetCurrentProcess(), GetCurrentProcessId(), NULL, pExceptionInfo);
	base.ShowExceptionInformation();

	return EXCEPTION_CONTINUE_SEARCH;
}

BOOL CBaseException::GetLogicalAddress(
	PVOID addr, PTSTR szModule, DWORD len, DWORD& section, DWORD& offset )
{
	MEMORY_BASIC_INFORMATION mbi;

	if ( !VirtualQuery( addr, &mbi, sizeof(mbi) ) )
		return FALSE;

	DWORD_PTR hMod = (DWORD_PTR)mbi.AllocationBase;

	if ( !GetModuleFileName( (HMODULE)hMod, szModule, len ) )
		return FALSE;

	if (!hMod)
		return FALSE;

	PIMAGE_DOS_HEADER pDosHdr = (PIMAGE_DOS_HEADER)hMod;
	PIMAGE_NT_HEADERS pNtHdr = (PIMAGE_NT_HEADERS)(hMod + pDosHdr->e_lfanew);
	PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION( pNtHdr );

	DWORD_PTR rva = (DWORD_PTR)addr - hMod;

	//���㵱ǰ��ַ�ڵڼ�����
	for (unsigned i = 0; i < pNtHdr->FileHeader.NumberOfSections; i++, pSection++ )
	{
		DWORD sectionStart = pSection->VirtualAddress;
		DWORD sectionEnd = sectionStart + std::max(pSection->SizeOfRawData, pSection->Misc.VirtualSize);

		if ( (rva >= sectionStart) && (rva <= sectionEnd) )
		{
			section = i+1;
			offset = rva - sectionStart;
			return TRUE;
		}
	}

	return FALSE;   // Should never get here!
}

void CBaseException::STF(unsigned int ui,  PEXCEPTION_POINTERS pEp)
{
	CBaseException base(GetCurrentProcess(), GetCurrentProcessId(), NULL, pEp);
	throw base;
}

void CBaseException::ShowExceptionInformation()
{
	OutputString(_T("Exceptions:\r\n"));
	ShowExceptionResoult(m_pEp->ExceptionRecord->ExceptionCode);

	OutputString(_T("Exception Flag :0x%x "), m_pEp->ExceptionRecord->ExceptionFlags);
	OutputString(_T("NumberParameters :%ld \n"), m_pEp->ExceptionRecord->NumberParameters);
	for (int i = 0; i < m_pEp->ExceptionRecord->NumberParameters; i++)
	{
		OutputString(_T("Param %d :0x%x \n"), i, m_pEp->ExceptionRecord->ExceptionInformation[i]);
	}
	OutputString(_T("Context :%p \n"), m_pEp->ContextRecord);
    OutputString(_T("ContextFlag : 0x%x, EFlags: 0x%x \n"), m_pEp->ContextRecord->ContextFlags, m_pEp->ContextRecord->EFlags);

	TCHAR szFaultingModule[MAX_PATH];
	DWORD section, offset;
	GetLogicalAddress(m_pEp->ExceptionRecord->ExceptionAddress, szFaultingModule, sizeof(szFaultingModule), section, offset );
	OutputString( _T("Fault address:  0x%X 0x%X:0x%X %s\r\n"), m_pEp->ExceptionRecord->ExceptionAddress, section, offset, szFaultingModule );

	ShowCallstack(GetCurrentThread(), m_pEp->ContextRecord);
}