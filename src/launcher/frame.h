/*
 * Copyright (c) 2020-2021 The reone project contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "../common/types.h"

namespace reone {

struct WindowID {
    static constexpr wxWindowID gameDir = wxID_HIGHEST + 1;
    static constexpr wxWindowID launch = wxID_HIGHEST + 2;
    static constexpr wxWindowID saveConfig = wxID_HIGHEST + 3;
};

class LauncherFrame : public wxFrame {
public:
    LauncherFrame();

private:
    struct Configuration {
        std::string gameDir;
        bool devMode {true};
        int width {1024};
        int height {768};
        bool fullscreen {false};
        bool vsync {false};
        bool fxaa {true};
        bool sharpen {true};
        bool grass {true};
        bool ssr {true};
        bool ssao {true};
        int texQuality {0};
        int shadowres {1};
        int drawdist {64};
        int musicvol {85};
        int voicevol {85};
        int soundvol {85};
        int movievol {85};
        int loglevel {static_cast<int>(LogLevel::Info)};
        int logch {LogChannels::general};
        bool logfile {false};
    } _config;

    wxTextCtrl *_textCtrlGameDir;
    wxCheckBox *_checkBoxDev;
    wxChoice *_choiceResolution;
    wxChoice *_choiceTextureQuality;
    wxChoice *_choiceShadowResolution;
    wxSlider *_sliderDrawDistance;
    wxCheckBox *_checkBoxFullscreen;
    wxCheckBox *_checkBoxVSync;
    wxCheckBox *_checkBoxFXAA;
    wxCheckBox *_checkBoxSharpen;
    wxCheckBox *_checkBoxGrass;
    wxCheckBox *_checkBoxSSR;
    wxCheckBox *_checkBoxSSAO;
    wxSlider *_sliderVolumeMusic;
    wxSlider *_sliderVolumeVoice;
    wxSlider *_sliderVolumeSound;
    wxSlider *_sliderVolumeMovie;
    wxChoice *_choiceLogLevel;
    wxCheckListBox *_checkListBoxLogChannels;
    wxCheckBox *_checkBoxLogFile;

    void OnLaunch(wxCommandEvent &event);
    void OnSaveConfig(wxCommandEvent &event);
    void OnGameDirLeftDown(wxMouseEvent &event);

    void LoadConfiguration();
    void SaveConfiguration();

    wxDECLARE_EVENT_TABLE();
};

} // namespace reone
