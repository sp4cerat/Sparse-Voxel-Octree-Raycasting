#include <Commdlg.h>
#include <windows.h>

char* win_getopenfilename(char* filter="All Files(*.*)\0*.*\0", uint flags=OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY)
{
	OPENFILENAME of;
	char szFileName[1000] = "";

	ZeroMemory(&of, sizeof(of));

	of.lStructSize = sizeof(of);
	of.hwndOwner = 0;
	of.lpstrFilter = filter;
	of.lpstrFile = szFileName;
	of.nMaxFile = 1000;
	of.Flags = flags;
	of.lpstrDefExt = "*.*";

	if(GetOpenFileName(&of))
	{
		return szFileName;
	}

	return 0;
}

char* win_getsavefilename(char* filter="All Files(*.*)\0*.*\0", uint flags=OFN_EXPLORER | OFN_HIDEREADONLY)
{
	OPENFILENAME of;
	char szFileName[1000] = "";

	ZeroMemory(&of, sizeof(of));

	of.lStructSize = sizeof(of);
	of.hwndOwner = 0;
	of.lpstrFilter = filter;
	of.lpstrFile = szFileName;
	of.nMaxFile = 1000;
	of.Flags = flags;
	of.lpstrDefExt = "*.*";

	if(GetSaveFileName(&of))
	{
		return szFileName;
	}

	return 0;
}

bool win_messagebox_confirm(char* title,char* text)
{
	int a=MessageBox(0,text,title,MB_OKCANCEL|MB_ICONINFORMATION);
	return (a==IDOK);
}