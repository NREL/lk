/*
BSD 3-Clause License

Copyright (c) Alliance for Sustainable Energy, LLC. See also https://github.com/NREL/lk/blob/develop/LICENSE 

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __lk_stdlib_h
#define __lk_stdlib_h

#ifdef LK_USE_WXWIDGETS
#include <wx/wx.h>
#endif

#include <lk/env.h>

namespace lk {
    /* these stdlib_xxxx() functions
        return an array of fcall_t references.
        the end of the list is denoted by a null fcall_t */
    fcall_t *stdlib_basic();

    fcall_t *stdlib_sysio();

    fcall_t *stdlib_string();

    fcall_t *stdlib_math();

    fcall_t *stdlib_thread();

#ifdef LK_USE_WXWIDGETS

    fcall_t *stdlib_wxui();

#endif

    bool tex_doc(const lk_string &file,
                 const lk_string &title,
                 fcall_t *lib);

    lk_string html_doc(const lk_string &title, fcall_t *lib);

    lk_string html_doc(fcall_t f);

    lk_string json_write(const lk::vardata_t &x);

    bool json_read(const lk_string &json, lk::vardata_t &x, lk_string *err = 0);

    std::vector<lk_string> dir_list(const lk_string &dir, const lk_string &extlist, bool ret_dirs = false);

    std::vector<lk_string>
    split(const lk_string &str, const lk_string &delim, bool ret_empty = false, bool ret_delim = false);

    lk_string join(const std::vector<lk_string> &list, const lk_string &delim);

    bool to_integer(const lk_string &str, int *x);

    bool to_float(const lk_string &str, float *x);

    bool to_double(const lk_string &str, double *x);

    lk_string lower_case(const lk_string &in);

    lk_string upper_case(const lk_string &in);

    size_t replace(lk_string &s, const lk_string &old_text, const lk_string &new_text);

    lk_string read_file(const lk_string &file);

    bool read_line(FILE *fp, lk_string &text, int prealloc = 256);

    bool rename_file(const lk_string &f0, const lk_string &f1);

    bool file_exists(const char *file);

    bool dir_exists(const char *path);

    bool remove_file(const char *path);

    bool mkdir(const char *path, bool make_full = false);

    lk_string path_only(const lk_string &path);

    lk_string name_only(const lk_string &path);

    lk_string ext_only(const lk_string &path);

    char path_separator();

    lk_string get_cwd();

    bool set_cwd(const lk_string &path);

    class sync_piped_process {
    public:
        sync_piped_process() {}

        virtual ~sync_piped_process() {}

        int spawn(const lk_string &command, const lk_string &workdir = "");

        virtual void on_stdout(const lk_string &line_text) = 0;
    };

    lk_string trim_to_columns(const lk_string &str, int numcols);

    lk_string format_vl(const lk_string &fmt, const std::vector<vardata_t *> &args);

    lk_string format(const char *fmt, ...);

    size_t format_vn(char *buffer, int maxlen, const char *fmt, va_list arglist);

    double besj0(double x);

    double besj1(double x);

    double besy0(double x);

    double besy1(double x);

    double besi0(double x);

    double besk0(double x);

    double besi1(double x);

    double besk1(double x);

    double gammln(double xx);

    double betacf(double a, double b, double x);

    double betai(double a, double b, double x);

    double pearson(double *x, double *y, size_t len);

    void gser(double *gamser, double a, double x, double *gln);

    void gcf(double *gammcf, double a, double x, double *gln);

    double gammp(double a, double x);

    double gammq(double a, double x);

    double erf(double x);

    double erfc(double x);
};

#ifdef LK_USE_WXWIDGETS

class MyMessageDialog : public wxDialog {
public:
    MyMessageDialog(wxWindow *parent,
                    const wxString &message,
                    const wxString &title,
                    long buttons,
                    const wxPoint &pos = wxDefaultPosition,
                    const wxSize &size = wxDefaultSize,
                    bool addButtonClose = false)
            : wxDialog(parent, wxID_ANY, title, pos, size, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
        SetEscapeId(wxID_NONE);

        wxPanel *panel = new wxPanel(this);
        panel->SetBackgroundColour(*wxWHITE);

        wxBoxSizer *szpnl = new wxBoxSizer(wxVERTICAL);

        int wrap = 600;
        if (size != wxDefaultSize && size.x > 100)
            wrap = size.x - 40;

        int nlpos = message.Find('\n');
        if (nlpos > 0) {
            wxStaticText *label1 = new wxStaticText(panel, wxID_ANY, message.Left(nlpos), wxDefaultPosition,
                                                    wxDefaultSize, wxALIGN_LEFT);
            wxFont font(label1->GetFont());
            font.SetPointSize(font.GetPointSize() + 2);
            label1->SetFont(font);
            label1->SetForegroundColour(wxColour(0, 0, 120));
            label1->Wrap(wrap);

            wxStaticText *label2 = new wxStaticText(panel, wxID_ANY, message.Mid(nlpos + 1), wxDefaultPosition,
                                                    wxDefaultSize, wxALIGN_LEFT);
            label2->Wrap(wrap);

            szpnl->Add(label1, 0, wxTOP | wxLEFT | wxRIGHT | wxEXPAND, 20);
            szpnl->Add(label2, 1, wxALL | wxEXPAND, 20);
        } else {
            wxStaticText *label = new wxStaticText(panel, wxID_ANY, message, wxDefaultPosition, wxDefaultSize,
                                                   wxALIGN_LEFT);
            label->Wrap(wrap);

            szpnl->Add(label, 1, wxALL | wxEXPAND, 20);
        }

        if (addButtonClose) {
            wxButton *buttonClose = new wxButton(this, wxID_OK, wxT("OK"));
            szpnl->Add(buttonClose, 1, wxCENTER);
        }

        panel->SetSizer(szpnl);

        wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(panel, 1, wxALL | wxEXPAND, 0);
        sizer->Add(CreateButtonSizer(buttons), 0, wxALL | wxEXPAND, 11);

        SetSizerAndFit(sizer);

        if (size != wxDefaultSize)
            SetClientSize(size);
        else {
            wxSize sz = GetClientSize();
            if (sz.x < 340) sz.x = 340;
            if (sz.y < 120) sz.y = 120;
            SetClientSize(sz);
        }

        if (pos == wxDefaultPosition) {
            if (parent)
                CenterOnParent();
            else
                CenterOnScreen();
        }
    }

    void OnClose(wxCloseEvent &) {
        EndModal(wxID_CANCEL);
    }

    void OnCharHook(wxKeyEvent &evt) {
        if (evt.GetKeyCode() == WXK_ESCAPE)
            EndModal(wxID_CANCEL);
    }

    void OnCommand(wxCommandEvent &evt) {
        EndModal(evt.GetId());
    }

DECLARE_EVENT_TABLE();
};

wxWindow *GetCurrentTopLevelWindow();
#endif

#endif
