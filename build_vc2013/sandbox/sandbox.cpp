#include <wx/wx.h>
#include <wx/frame.h>
#include <wx/stc/stc.h>
#include <wx/webview.h>
#include <wx/statbmp.h>
#include <wx/numformatter.h>
#include <wx/grid.h>
#include <wx/zstream.h>
#include <wx/app.h>

#include <lk_absyn.h>
#include <lk_env.h>
#include <lk_stdlib.h>
#include <lk_eval.h>
#include <lk_lex.h>
#include <lk_invoke.h>
#include <lk_math.h>
#include <lk_parse.h>

#define LK_USE_WXWIDGETS 1



#include <vector>
#include <string>
#include <algorithm>
#include <allocators>

typedef struct DIR DIR;
struct dirent
{
	char *d_name;
};
DIR           *opendir(const char *);
int           closedir(DIR *);
struct dirent *readdir(DIR *);
void          rewinddir(DIR *);

class TestApp : public wxApp
{
public:
	bool OnInit()
	{
		wxFrame *frm = new wxFrame(NULL, wxID_ANY, "Test", wxDefaultPosition, wxSize(300, 200));
		frm->SetBackgroundColour(*wxWHITE);

		std::vector< lk_string  > x;
		//lk::dir_exists("c:/test_csv");
		lk_string path = "c:/test_csv";
		lk_string extlist = "csv";

		//x.push_back(d);
		//x=lk::dir_list(d, e, false);
		//bool b=lk::file_exists("c:\tmp");



		// test dir_list
		std::vector< lk_string > list;
		std::vector< lk_string > extensions;// = split(lower_case(extlist), ",");
		extensions.push_back("csv");
		DIR *dir;
		struct dirent *ent;

		dir = ::opendir((const char*)path.c_str());
//		dir = ::opendir((const wchar_t*)path.c_str());
		 
		while ((ent = readdir(dir)))
		{
			lk_string item(ent->d_name);
			if (item == "." || item == "..")
				continue;

			//if (extlist.empty()
			//	|| extlist == "*"
			//	|| (ret_dirs && dir_exists((const char*)lk_string(path + "/" + item).c_str())))
			//{
			//	list.push_back(item);
			//}
			//else
			//{
				bool found = false;
				lk_string ext = item;
				for (size_t i = 0; !found && i<extensions.size(); i++)
				{
					if (ext == extensions[i])
					{
						list.push_back(item);
						found = true;
					}
				}
//			}
		}

		closedir(dir);


		/* test opendir and readdir x64 and win32 - both working
		struct DIR *dir;
		struct dirent *ent;

		dir = opendir((const char*)d.c_str());

		ent = readdir(dir);
		*/
		
		frm->Show();
		return true;
	}
};


IMPLEMENT_APP(TestApp);
